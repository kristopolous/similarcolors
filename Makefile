CC=cc -Wall
CFLAGS=`pkg-config --cflags opencv` -g3 -gdwarf-2
LDFLAGS= `pkg-config --libs opencv` -lm -ldb
colors: colors.o server.o db.o rb/red_black_tree.o rb/misc.o rb/stack.o 
install:
	make
	install colors /usr/local/bin/
clean:
	rm -f colors *.o */*.o
