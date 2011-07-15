CC=c++ -Wall
CFLAGS=`pkg-config --cflags opencv` -O2
LDFLAGS= `pkg-config --libs opencv` -lm
colors: colors.o
install:
	make
	install colors /usr/local/bin/
clean:
	rm -f colors colors.o
