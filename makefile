CFLAGS = -Wall
LIBS = -lm

objects = mosaic.o photomosaic.o

all: mosaico

mosaico: $(objects)
	gcc $(objects) -o mosaico $(LIBS)

mosaic.o: mosaic.c
	gcc -c mosaic.c $(CFLAGS)

photomosaic.o:	photomosaic.c photomosaic.h
	gcc -c photomosaic.c $(CFLAGS)

clean:
	rm -f *.o

purge:	clean
	rm -f mosaico