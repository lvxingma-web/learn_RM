#list
classmates = ['Michael', 'Bob', 'Tracy'] #classmates就是一个list变量
print(len(classmates))
print(classmates[1])
print(classmates[-1])

#有序列表
classmates.append('A')
classmates.insert(1,'Jack')
#pop()删尾 pop(i)指定位置
classmates.pop(1)

classmates[1] = 'Sarah'

s = ['python', 'java', ['asp', 'php'], 'scheme']

#tuple
classmates = ('Michael', 'Bob', 'Tracy')
classmates[0]
t = (1,)

#t->('a', 'b', ['A', 'B'])
#     |    |     | 
#    str  str   list
#tuple中元素指向不能变
t = ('a', 'b', ['A', 'B'])
t[2][0] = 'X'
t[2][1] = 'Y'








