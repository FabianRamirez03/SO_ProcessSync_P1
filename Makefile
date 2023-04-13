all:
	gcc -c src/inicializador.c
	gcc inicializador.o -lpthread -lncurses -lrt -lm -o out/inicializador
	rm inicializador.o

	gcc -c src/emisor.c
	gcc emisor.o -lpthread -lrt -lncurses -lm -o out/emisor
	rm emisor.o

	find /dev/shm/ -mindepth 1 -delete