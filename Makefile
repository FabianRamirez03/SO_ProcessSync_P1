all: build inicializador

KEY = 25
ID = "prueba"
BLOQUES = 10


build:
	gcc -c src/inicializador.c
	gcc inicializador.o -lpthread -lncurses -lrt -lm -o out/inicializador
	rm inicializador.o

	gcc -c src/emisor.c
	gcc emisor.o -lpthread -lrt -lncurses -lm -o out/emisor
	rm emisor.o

	gcc -c src/receptor.c
	gcc receptor.o -lpthread -lrt -lncurses -lm -o out/receptor
	rm receptor.o

	gcc -c src/finalizador.c
	gcc finalizador.o -lpthread -lrt -lncurses -lm -o out/finalizador
	rm finalizador.o

	find /dev/shm/ -mindepth 1 -delete

inicializador:
	./out/inicializador -n $(ID) -b $(BLOQUES) -k $(KEY)

emisor: 
	./out/emisor -n $(ID) -k $(KEY) 

receptor:
	./out/receptor -n $(ID) -k $(KEY) 

finalizador:
	./out/finalizador -n $(ID)