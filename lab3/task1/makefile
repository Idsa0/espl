.SILENT: link compile clean run
.PHONY: all link compile clean run

all: clean assemble compile link

link:
	ld -m elf_i386 start.o util.o -o task1

compile:
	gcc -m32 -Wall -ansi -c -nostdlib -fno-stack-protector util.c -o util.o

assemble:
	nasm -f elf32 start.s -o start.o

clean:
	rm -f *.o task1

run:
	task1