.SILENT: link compile clean run rm_o
.PHONY: link compile clean run rm_o

all: clean compile link rm_o

rm_o:
	rm -f *.o

link:
	gcc -g -m32 -Wall -o hexeditplus hexeditplus.o
	gcc -g -m32 -Wall -o myntsc ntsc.o

compile:
	gcc -g -m32 -Wall -c -o hexeditplus.o task1.c
	gcc -g -m32 -Wall -fno-stack-protector -fno-pie -c -o ntsc.o task4.c

clean:
	rm -f *.o hexeditplus myntsc

run:
	hexeditplus