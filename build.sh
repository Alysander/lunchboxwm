gcc basin.c -lX11 `pkg-config --cflags --libs cairo` -std=c99 -g -o basin 
