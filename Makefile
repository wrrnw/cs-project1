CC = gcc
CFLAGS = -std=c99 -O3 -Wall -Wpedantic
EXE = server
CFILE = server.c

all: $(EXE)

$(EXE): $(CFILE)
	$(CC) $(CFLAGS) -o $(EXE) $<

clean:
	rm $(EXE)
