a = 100
if a > 100:
    print('大于')
else:
    print('小于')

#0x0000_0000
#\转义 r'\\\n\\' = \\\n\\

print(r'\\\n\\')

print('''line1
line2
line3''')

print(r'''hello,\n
world''')

#布尔 and or not
print(True and True)
print(True and False)
print(False and False)

#动态语言

a = 'ABC'#解释器创建‘ABC’，变量a指向‘ABC’
b = a#b指向a
a = 'XYZ'#解释器创建‘XYZ’，变量a指向‘XYZ’
print(b)

#大写表示常量

#浮点/与地板除//
print(10/3)
print(10//3)

#对变量赋值x = y是把变量x指向真正的对象，该对象是变量y所指向的。随后对变量y的赋值不影响变量x的指向

