.SILENT: link compile clean run

all: clean compile link

link:
	gcc -g -m32 -Wall -o out myshell.c LineParser.o
	gcc -g -m32 -Wall -o looper looper.o

compile:
	gcc -g -m32 -Wall -c -o myshell.o myshell.c
	gcc -g -m32 -Wall -c -o LineParser.o LineParser.c
	gcc -g -m32 -Wall -c -o looper.o looper.c

clean:
	rm -f *.o out looper

run:
	out -d