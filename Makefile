all:
	gcc basin.c -lX11 -lXcursor `pkg-config --cflags --libs cairo` -std=c99 -g -o basin 
