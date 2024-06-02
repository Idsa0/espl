all: clean compile link
	
link:
	gcc -g -m32 -Wall -o bin/a bin/*.o

compile: bin/add.o bin/numbers.o bin/main.o

bin/add.o: src/add.s
	nasm -g -f elf -w+all -o bin/add.o src/add.s

bin/numbers.o: src/numbers.c
	gcc -g -m32 -Wall -c -o bin/numbers.o src/numbers.c

bin/main.o: src/main.c
	gcc -g -m32 -Wall -c -o bin/main.o src/main.c

clean:
	rm -f bin/*