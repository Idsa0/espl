.SILENT: link compile clean run rm_o
.PHONY: link compile clean run rm_o

all: clean compile link rm_o

rm_o:
	rm -f *.o

link:
	gcc -g -m32 -Wall -o main main.o

compile:
	gcc -g -m32 -Wall -c -o main.o main.c

clean:
	rm -f *.o main

run:
	main