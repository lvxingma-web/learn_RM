"""Expose the three raw YOLO head convolutions, bypassing OpenCV 4.5's 5D bug.

The removed ONNX tail only reshapes/transposes and applies a fixed affine
transform to the first eight corner coordinates. The decoder must reproduce
that transform. Use only with the specific supplied yolov5.onnx and verify
against a capable ONNX Runtime before relying on results.
"""

import argparse
from pathlib import Path
from convert_fp16_onnx import parse, encode, rewrite


HEADS = [
    b"/m/model.24/m.0/Conv_output_0",
    b"/m/model.24/m.1/Conv_output_0",
    b"/m/model.24/m.2/Conv_output_0",
]


def proto_field(number, wire, value):
    return (number, wire, value)


def value_info(name, side):
    dimensions = (1, 66, side, side)
    shape = encode([proto_field(1, 2, encode([proto_field(1, 0, d)]))
                    for d in dimensions])
    tensor_type = encode([proto_field(1, 0, 1), proto_field(2, 2, shape)])
    type_proto = encode([proto_field(1, 2, tensor_type)])
    return encode([proto_field(1, 2, name), proto_field(2, 2, type_proto)])


def graph_heads(data):
    fields = parse(data)
    nodes = [(n,w,v) for n,w,v in fields if n == 1]
    keeps = []
    seen = set()
    for n,w,v in nodes:
        node_fields = parse(v)
        outputs = [x for field,wire,x in node_fields if field == 2]
        if any(x in HEADS for x in outputs):
            seen.update(x for x in outputs if x in HEADS)
        # Tail nodes use five-dimensional tensors unsupported by this OpenCV.
        if not any(x.startswith(b"/m/model.24/") or x == b"output"
                   for x in outputs) or any(x in HEADS for x in outputs):
            keeps.append((n,w,v))
    if seen != set(HEADS):
        raise ValueError("This is not the expected three-head YOLO model")
    new_fields = [(n,w,v) for n,w,v in fields if n not in (1,12)]
    new_fields.extend(keeps)
    new_fields.extend((12,2,value_info(name,side))
                      for name,side in zip(HEADS,(80,40,20)))
    return encode(new_fields)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("source",type=Path)
    ap.add_argument("destination",type=Path)
    args = ap.parse_args()
    if args.source.resolve() == args.destination.resolve():
        raise SystemExit("Destination must differ from source")
    model,count = rewrite(args.source.read_bytes(),"model")
    result = []
    for n,w,v in parse(model):
        result.append((n,w,graph_heads(v) if n == 7 and w == 2 else v))
    args.destination.parent.mkdir(parents=True,exist_ok=True)
    args.destination.write_bytes(encode(result))
    print(f"Wrote {args.destination}; converted {count} FP16 tensors/types")


if __name__ == "__main__":
    main()
