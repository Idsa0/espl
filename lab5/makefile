.SILENT: link assemble compile clean run rm_o
.PHONY: link assemble compile clean run rm_o

FLG_GCC = -m32 -c -g -Wall
FLG_LD = -L/usr/lib32 -lc -T linking_script -dynamic-linker /lib32/ld-linux.so.2
FLG_NASM = -f elf32 -g
LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS), 32)
	FLG_GCC = -c -g -Wall
	FLG_LD = -lc -T linking_script -dynamic-linker /lib/ld-linux.so.2
endif

all: assemble compile link

rm_o:
	rm -f *.o

link:
	ld $(FLG_LD) -o out loader.o startup.o start.o

compile:
	gcc $(FLG_GCC) -o loader.o loader.c

assemble:
	nasm $(FLG_NASM) -o start.o start.s

clean:
	rm -f *.o out

run:
	out