CC = gcc
CFLAGS = -I $(IDIR) `sdl2-config --cflags --libs`

IDIR = ./include/
SRCDIR = ./src/

SOURCES = $(SRCDIR)*.c
		  # \ $(SRCDIR)utils/*.c

all: blackrock #run #clean

blackrock: $(SOURCES)
	$(CC) $(SOURCES) $(CFLAGS) -o ./bin/blackrock

run:
	./bin/blackrock

clean:
	rm ./bin/blackrock      