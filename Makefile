CC=gcc 
LDFLAGS+=-lX11 -lXcursor -lXext -lcairo
CFLAGS= -g -Werror -Wall -Wextra -pedantic -std=c99 `pkg-config --cflags cairo`

#make runs the first one by default

all:basin

basin: basin.c focus.o workspace.o frame.o frame-actions.o theme.o xcheck.o space.o menus.o *.h Makefile
	${CC}  focus.o workspace.o frame.o frame-actions.o theme.o xcheck.o space.o menus.o basin.c ${CFLAGS} ${LDFLAGS} -o basin

focus.o: focus.c frame.o xcheck.o *.h
	${CC} ${CFLAGS} -c $<

workspace.o: workspace.c frame-actions.o frame.o *.h
	${CC} ${CFLAGS} -c $<

frame.o: frame.c xcheck.o *.h
	${CC} ${CFLAGS} -c $<

frame-actions.o: frame-actions.c frame.o xcheck.o *.h
	${CC} ${CFLAGS} -c $<

theme.o: theme.c xcheck.o *.h
	${CC} ${CFLAGS} -c $<

xcheck.o: xcheck.c *.h
	${CC} ${CFLAGS} -c $<

space.o: space.c *.h
	${CC} ${CFLAGS} -c $<

menus.o: menus.c
	${CC} ${CFLAGS} -c $<

	
clean:
	rm -f *.o
	rm -f basin

install:
	cp basin /usr/bin     

#debug:
#	gcc basin.c -lX11 -lXcursor -lXext `pkg-config --cflags --libs cairo` -std=c99 -g -o basin > compile 2>&1
