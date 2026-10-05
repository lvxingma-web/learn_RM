"""Convert FLOAT16 tensors in this ONNX model to FLOAT32 for OpenCV 4.5 DNN.

Uses only numpy and the ONNX protobuf wire format.  The original model is
never modified.  This is deliberately narrow: unsupported tensor encodings
raise an error rather than silently changing model semantics.
"""

import argparse
from pathlib import Path
import numpy as np


def read_varint(data, pos):
    value = 0
    shift = 0
    while True:
        byte = data[pos]
        pos += 1
        value |= (byte & 0x7F) << shift
        if byte < 128:
            return value, pos
        shift += 7


def varint(value):
    result = bytearray()
    while value >= 128:
        result.append((value & 0x7F) | 0x80)
        value >>= 7
    result.append(value)
    return bytes(result)


def parse(data):
    pos = 0
    fields = []
    while pos < len(data):
        tag, pos = read_varint(data, pos)
        wire = tag & 7
        if wire == 0:
            value, pos = read_varint(data, pos)
        elif wire == 2:
            length, pos = read_varint(data, pos)
            value = data[pos : pos + length]
            pos += length
        elif wire in (1, 5):
            length = 8 if wire == 1 else 4
            value = data[pos : pos + length]
            pos += length
        else:
            raise ValueError(f"Unsupported protobuf wire type: {wire}")
        fields.append((tag >> 3, wire, value))
    if pos != len(data):
        raise ValueError("Malformed protobuf")
    return fields


def encode(fields):
    out = bytearray()
    for number, wire, value in fields:
        out.extend(varint((number << 3) | wire))
        if wire == 0:
            out.extend(varint(value))
        elif wire == 2:
            out.extend(varint(len(value)))
            out.extend(value)
        else:
            out.extend(value)
    return bytes(out)


def convert_tensor(data):
    fields = parse(data)
    dtype = next((v for n, w, v in fields if n == 2 and w == 0), None)
    if dtype != 10:  # TensorProto.FLOAT16
        return data, 0
    raw = next((v for n, w, v in fields if n == 9 and w == 2), None)
    if raw is None:
        raise ValueError("FLOAT16 tensor without raw_data is unsupported")
    if len(raw) % 2:
        raise ValueError("Malformed FLOAT16 raw_data")
    converted = np.frombuffer(raw, dtype="<f2").astype("<f4").tobytes()
    fields = [(n, w, 1 if n == 2 else converted if n == 9 else v)
              for n, w, v in fields]
    return encode(fields), 1


def rewrite(data, kind):
    count = 0
    result = []
    for number, wire, value in parse(data):
        if wire == 2:
            nested = None
            if kind == "model" and number == 7:
                nested = "graph"
            elif kind == "graph" and number == 5:
                value, added = convert_tensor(value)
                count += added
            elif kind == "graph" and number in (11, 12, 13):
                nested = "value_info"
            elif kind == "graph" and number == 1:
                nested = "node"
            elif kind == "node" and number == 5:
                nested = "attribute"
            elif kind == "attribute" and number == 5:
                value, added = convert_tensor(value)
                count += added
            elif kind == "value_info" and number == 2:
                nested = "type"
            elif kind == "type" and number == 1:
                nested = "tensor_type"
            if nested:
                value, added = rewrite(value, nested)
                count += added
        elif kind == "tensor_type" and number == 1 and wire == 0 and value == 10:
            value = 1
            count += 1
        result.append((number, wire, value))
    return encode(result), count


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    if args.source.resolve() == args.destination.resolve():
        raise SystemExit("Destination must differ from source")
    model, count = rewrite(args.source.read_bytes(), "model")
    if count == 0:
        raise SystemExit("No FLOAT16 tensors or graph inputs found")
    args.destination.parent.mkdir(parents=True, exist_ok=True)
    args.destination.write_bytes(model)
    print(f"Converted {count} FLOAT16 tensors/types: {args.destination}")


if __name__ == "__main__":
    main()
