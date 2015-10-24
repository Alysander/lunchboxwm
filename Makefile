SHELL=/bin/sh

CC=gcc
LDFLAGS+=-lX11 -lXcursor -lXext -lcairo
CFLAGS= -g -Wall -Wextra -pedantic -std=c99 `pkg-config --cflags cairo`
OBJS=bin/
COMPILE=${CC} ${CFLAGS} -c $< -o $@
# Some variables useful for building debian packages
VERSION=0.5
#remember to change the version in the debian changelog as well.
bindir=/usr/bin
datadir=/usr/local

#make runs the first one by default

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

install:
	mkdir -p $(DESTDIR)$(datadir)/lunchbox
	cp -r themes $(DESTDIR)$(datadir)/lunchbox
	#Allow anyone to read the themes
	chmod -R a+r $(DESTDIR)$(datadir)/lunchbox
	chmod -R a+X $(DESTDIR)$(datadir)/lunchbox
	mkdir -p $(DESTDIR)/usr/share/xsessions
	cp -r ./lunchbox.desktop $(DESTDIR)/usr/share/xsessions
	cp lunchbox $(DESTDIR)$(bindir)

uninstall:
	rm -f $(DESTDIR)$(bindir)/lunchbox
	rm -Rf $(DESTDIR)$(datadir)/lunchbox/themes
	rm -f $(DESTDIR)/usr/share/xsessions/lunchbox.desktop

distprep:
	make clean
	tar cvzf lunchbox-${VERSION}.tgz *.c *.h lunchbox gpl-3.0.txt lunchbox.desktop TODO EWMH_SUPPORT PLAN INSTALL Makefile themes bin debian