CC = gcc
CFLAGS = -I $(IDIR) `sdl2-config --cflags --libs` -lm -l sqlite3 -l pthread -D CLIENT_DEBUG

IDIR = ./include/
SRCDIR = ./src/

SOURCES = $(SRCDIR)*.c \
		  $(SRCDIR)utils/*.c \
		  $(SRCDIR)ui/*.c \
		  $(SRCDIR)cerver/*.c

all: blackrock #run #clean

blackrock: $(SOURCES)
	$(CC) $(SOURCES) $(CFLAGS) -o ./bin/blackrock

run:
	./bin/blackrock

clean:
	rm ./bin/blackrock      