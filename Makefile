all:
	gcc -c src/inicializador.c
	gcc inicializador.o -lpthread -lncurses -lrt -lm -o out/inicializador
	rm inicializador.o

	gcc -c src/emisor.c
	gcc emisor.o -lpthread -lrt -lncurses -lm -o out/emisor
	rm emisor.o

	gcc -c src/receptor.c
	gcc receptor.o -lpthread -lrt -lncurses -lm -o out/receptor
	rm receptor.o

	find /dev/shm/ -mindepth 1 -delete

	./out/inicializador -n "prueba" -b 10 -k 25
	./out/emisor -n "prueba" -k 25 -m