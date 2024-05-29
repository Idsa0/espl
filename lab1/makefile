.SILENT: link compile clean run

all: clean compile link

link:
	gcc -g -m32 -Wall -o out base.o menu.o
#	gcc -g -m32 -Wall -o bin/addresses bin/addresses.o
#	gcc -g -m32 -Wall -o bin/cw bin/cw.o

compile:
	gcc -g -m32 -Wall -c -o menu.o menu_map.c
	gcc -g -m32 -Wall -c -o base.o base.c
#	gcc -g -m32 -Wall -c -o bin/addresses.o src/addresses.c
#	gcc -g -m32 -Wall -c -o bin/cw.o src/count-words.c

clean:
	rm -f *.o out

run:
	out