main: main.c tesi.o
	gcc -g -o main main.c tesi.o -lncurses -lutil
#`pkg-config --cflags --libs glib-1.2`

tesi.o: tesi.c tesi.h
	gcc -g -c tesi.c
