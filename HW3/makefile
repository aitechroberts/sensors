#
# target entry to build program executable from program and mylib 
# object files 
#
# 	gcc -o tpms tpms.c -lm
all:
	#
	# object files for shared libraries need to be compiled as position independent
	# code (-fPIC) because they are mapped to any position in the address space.
	#
	cp submission/*.c build/
	gcc -c -fPIC -lm tpms.c -o tpms.o
	gcc tpms.o -shared -o libtpms.so
