src = 	$(wildcard src/*.c) \
		$(wildcard src/ui/*.c) \
		$(wildcard src/cerver/*.c) \
		$(wildcard src/utils/*.c)

objs = $(src:.c=.o)

IDIR = ./include/

CC = gcc
CFLAGS = -I $(IDIR) $(SDL2) $(MATH) $(SQLITE) $(PTHREAD) $(BLACK_DEBUG) $(DEFINES)

SDL2 = `sdl2-config --cflags --libs`
MATH = -lm 
SQLITE = -l sqlite3 
PTHREAD = -l pthread 

# for debugging...
DEBUG = -g

# additional blackrock info
BLACK_DEBUG = -D BLACK_DEBUG

# print additional client information
DEFINES = -D CLIENT_DEBUG

OUTPUT = -o ./bin/blackrock

all: blackrock #run #clean

blackrock: $(objs)
	$(CC) $^ $(CFLAGS) $(OUTPUT)
run:
	./bin/blackrock

clean:
	rm ./bin/blackrock    