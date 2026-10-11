#ASCII Unicode UTF-8

#ord()chr()

#encode('utf-8') decode('ascii') len(b'...')

#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#'Hello, %s' % 'world'
#'Hello, world'
#'Hi, %s, you have $%d.' % ('Michael', 1000000)
#'Hi, Michael, you have $1000000.'


print('%2d-%02d' % (3, 1))
print('%.2f' % 3.1415926)

'Hello, {0}, 成绩提升了 {1:.1f}%'.format('小明', 17.125)
#'Hello, 小明, 成绩提升了 17.1%'

r = 2.5
s = 3.14 * r ** 2
print(f'The area of a circle with radius {r} is {s:.2f}')
#The area of a circle with radius 2.5 is 19.62

s1 = 72
s2 = 85
r = (s2 - s1) / s1
print(f'{r:.1f}')
