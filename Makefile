all:
	gcc -c src/inicializador.c
	gcc inicializador.o -lpthread -lrt -lm -o out/inicializador
	rm inicializador.o
	find /dev/shm/ -mindepth 1 -delete
