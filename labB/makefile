.SILENT: link compile clean run

all: clean compile link

link:
	gcc -g -m32 -Wall -o virusDetector vd.o
	gcc -g -m32 -Wall -o out vd.o

compile:
	gcc -g -m32 -Wall -c -o vd.o AntiVirus.c

clean:
	rm -f *.o out virusDetector

run:
	out