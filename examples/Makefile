all: gtk-terminal fltk-terminal

gtk-terminal: gtk-terminal.c tesi.o
	gcc -Wall -g gtk-terminal.c -o gtk-terminal tesi.o `pkg-config --cflags gtk+-2.0` `pkg-config --libs gtk+-2.0`

fltk-terminal: fltk-terminal.cpp tesi.o
	g++ -Wall -g fltk-terminal.cpp -o fltk-terminal tesi.o `fltk-config --cxxflags` -L/usr/local/lib -lfltk -lXext -lX11 -lm

tesi.o: tesi.h tesi.c
	gcc -c tesi.o tesi.c
