all: libcrt.a

libcrt.a: crt.o
	ar rcs libcrt.a crt.o

crt.o: crt.c
	gcc -c -nostdlib -nostartfiles crt.c -o crt.o

clean:
	rm -f crt.o libcrt.a
