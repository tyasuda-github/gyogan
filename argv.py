#
# -*- coding: utf-8 -*-
# $Date: 2021/10/08 00:17:54 $
#
import sys



print('sys.argv             : ', sys.argv)
print('type(sys.argv)       : ', type(sys.argv))
print()

print('sys.argv[0])         : ', sys.argv[0])
print('type(sys.argv[0])    : ', type(sys.argv[0]))
print()

print('len(sys.argv)        : ', len(sys.argv))
print('type(len(sys.argv))  : ', type(len(sys.argv)))
print()



def type_file(file):
	print(file)
	i = 0
	fh = open(file)
	line = fh.readline()

	while line:
		i += 1
		print(i, "\t:", line, end='')
		line = fh.readline()

	fh.close()
	print()



argv = sys.argv
argc = len(argv)

for i in range(argc):
	type_file(argv[i])

quit()
