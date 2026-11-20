.PHONY: all doc clean install uninstall

all: main.o mbr.o gpt.o
	gcc -o listpart main.o mbr.o gpt.o -lm

main.o: main.c mbr.h gpt.h
	gcc -c -o main.o main.c

mbr.o: mbr.c mbr.h
	gcc -c -o mbr.o mbr.c

gpt.o: gpt.c gpt.h mbr.h
	gcc -c -o gpt.o gpt.c

doc:
	doxygen

clean:
	rm -f main.o mbr.o gpt.o listpart
	rm -rf docs

install: all
	sudo cp listpart /usr/local/bin

uninstall:
	sudo rm -f /usr/local/bin/listpart