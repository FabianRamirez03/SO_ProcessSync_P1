all:
	gcc -c src/inicializador.c
	gcc inicializador.o -lpthread -lrt -lm -o out/inicializador
	rm inicializador.o

	gcc -c src/emisor.c
	gcc emisor.o -lpthread -lrt -lm -o out/emisor
	rm emisor.o

	find /dev/shm/ -mindepth 1 -delete