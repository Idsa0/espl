.SILENT: link compile clean run

all: clean compile link

link:
	gcc -g -m32 -Wall -o out bubble.o

compile:
	gcc -g -m32 -Wall -c -o bubble.o bubblesort.c

clean:
	rm -f *.o out

run:
	out