CC=gcc 
LDFLAGS+=-lX11 -lXcursor -lXext -lcairo
CFLAGS= -g -Wall -Wextra -pedantic -std=c99 `pkg-config --cflags cairo`
OBJS=bin/
COMPILE=${CC} ${CFLAGS} -c $< -o $@ 
#make runs the first one by default

all:lunchbox

lunchbox: main.c ${OBJS}focus.o ${OBJS}workspace.o ${OBJS}frame.o ${OBJS}frame-actions.o ${OBJS}theme.o ${OBJS}xcheck.o ${OBJS}space.o ${OBJS}menus.o *.h Makefile
	${CC}  ${OBJS}focus.o ${OBJS}workspace.o ${OBJS}frame.o ${OBJS}frame-actions.o ${OBJS}theme.o ${OBJS}xcheck.o ${OBJS}space.o ${OBJS}menus.o main.c ${CFLAGS} ${LDFLAGS} -o lunchbox

${OBJS}focus.o: focus.c ${OBJS}frame.o ${OBJS}xcheck.o *.h
	${COMPILE}

${OBJS}workspace.o: workspace.c ${OBJS}frame-actions.o ${OBJS}frame.o *.h
	${COMPILE}

${OBJS}frame.o: frame.c ${OBJS}xcheck.o *.h
	${COMPILE}

${OBJS}frame-actions.o: frame-actions.c ${OBJS}frame.o ${OBJS}xcheck.o *.h
	${COMPILE}

${OBJS}theme.o: theme.c ${OBJS}xcheck.o *.h
	${COMPILE}

${OBJS}xcheck.o: xcheck.c *.h
	${COMPILE}

${OBJS}space.o: space.c *.h
	${COMPILE}

${OBJS}menus.o: menus.c
	${COMPILE}


install:
	cp lunchbox /usr/bin     

#debug:
#	gcc basin.c -lX11 -lXcursor -lXext `pkg-config --cflags --libs cairo` -std=c99 -g -o basin > compile 2>&1
