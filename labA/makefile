.PHONY: encoder
.SILENT: link compile clean run

all: encoder

encoder: clean compile link

link:
	gcc -g -m32 -Wall -o encoder encoder.o

compile:
	gcc -g -m32 -Wall -c -o encoder.o encoder.c

clean:
	rm -f encoder encoder.o

run:
	encoder +e123 -Iin.txt -Oout.txt