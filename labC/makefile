.SILENT: link compile clean run

all: clean compile link

link:
	gcc -g -m32 -Wall -o out myshell.c

compile:
	gcc -g -m32 -Wall -c -o myshell.o myshell.c

clean:
	rm -f *.o out

run:
	out -d