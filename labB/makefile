.SILENT: link compile clean run

all: clean compile link

link:
	gcc -g -m32 -Wall -o out xp.o

compile:
	gcc -g -m32 -Wall -c -o xp.o hexaPrint.c

clean:
	rm -f *.o out

run:
	out