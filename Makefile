CC = gcc
CFLAGS = -std=c99 -O3 -Wall -Wpedantic
EXE = image_tagger
CFILE = image_tagger.c

all: $(EXE)

$(EXE): $(CFILE)
	$(CC) $(CFLAGS) -o $(EXE) $<

clean:
	rm *.o $(EXE)
