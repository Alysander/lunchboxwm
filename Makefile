SHELL=/bin/sh

CC=gcc
LDFLAGS+=-lX11 -lXcursor -lXext -lcairo
CFLAGS= -g -Wall -Wextra -pedantic -std=c99 `pkg-config --cflags cairo`
OBJS=bin/
COMPILE=${CC} ${CFLAGS} -c $< -o $@
bindir=/usr/bin
datadir=/usr/local

all:folder lunchbox
folder:
	mkdir -p bin
lunchbox: main.c  ${OBJS}util.o ${OBJS}focus.o ${OBJS}workspace.o ${OBJS}frame.o ${OBJS}frame-actions.o ${OBJS}theme.o ${OBJS}xcheck.o ${OBJS}space.o ${OBJS}menus.o *.h Makefile
	${CC}   ${OBJS}util.o ${OBJS}focus.o ${OBJS}workspace.o ${OBJS}frame.o ${OBJS}frame-actions.o ${OBJS}theme.o ${OBJS}xcheck.o ${OBJS}space.o ${OBJS}menus.o main.c ${CFLAGS} ${LDFLAGS} -o lunchbox

${OBJS}focus.o: focus.c ${OBJS}frame.o ${OBJS}xcheck.o *.h
	${COMPILE}

${OBJS}workspace.o: workspace.c ${OBJS}frame-actions.o ${OBJS}frame.o *.h
	${COMPILE}

${OBJS}frame.o: frame.c ${OBJS}xcheck.o *.h
	${COMPILE}

${OBJS}frame-actions.o: frame-actions.c ${OBJS}frame.o ${OBJS}xcheck.o *.h
	${COMPILE}

${OBJS}theme.o: theme.c ${OBJS}xcheck.o ${OBJS}util.o *.h
	${COMPILE}

${OBJS}xcheck.o: xcheck.c *.h
	${COMPILE}

${OBJS}space.o: space.c *.h
	${COMPILE}

${OBJS}menus.o: menus.c  *.h
	${COMPILE}

${OBJS}util.o: util.c  *.h
	${COMPILE}

clean:
	rm -f ${OBJS}*.o

#This install rule only installs a .desktop file
#which allows most login managagers to show a different session.
install: lunchbox.desktop
	mkdir -p $(DESTDIR)/usr/share/xsessions
	cp -r ./lunchbox.desktop $(DESTDIR)/usr/share/xsessions

uninstall:
	rm -f $(DESTDIR)/usr/share/xsessions/lunchbox.desktop
