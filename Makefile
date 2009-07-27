CC=gcc 
LDFLAGS+=-lX11 -lXcursor -lXext -lcairo
CFLAGS= -g -std=c99 `pkg-config --cflags cairo`

#make runs the first one by default

all:basin

#basin.c  frame-actions.c  menus.c  theme.c      xcheck.c
#focus.c  frame.c          space.c  workspace.c 

basin: basin.c focus.o workspace.o frame.o frame-actions.o theme.o xcheck.o space.o menus.o *.h Makefile
	${CC}  focus.o workspace.o frame.o frame-actions.o theme.o xcheck.o space.o menus.o basin.c ${CFLAGS} ${LDFLAGS} -o basin

focus.o: focus.c frame.o xcheck.o
	${CC} ${CFLAGS} -c $<

workspace.o: workspace.c frame-actions.o frame.o
	${CC} ${CFLAGS} -c $<

frame.o: frame.c xcheck.o
	${CC} ${CFLAGS} -c $<

frame-actions.o: frame-actions.c frame.o xcheck.o
	${CC} ${CFLAGS} -c $<

theme.o: theme.c xcheck.o
	${CC} ${CFLAGS} -c $<

xcheck.o: xcheck.c
	${CC} ${CFLAGS} -c $<

space.o: space.c
	${CC} ${CFLAGS} -c $<

menus.o: menus.c
	${CC} ${CFLAGS} -c $<

	
clean:
	rm -f *.o
	rm -f basin
     
#debug:
#	gcc basin.c -lX11 -lXcursor -lXext `pkg-config --cflags --libs cairo` -std=c99 -g -o basin > compile 2>&1
