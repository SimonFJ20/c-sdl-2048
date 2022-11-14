
TARGET = game

CC = gcc
LD = gcc

CFLAGS = -Wall -Wpedantic -Werror -O3
LFLAGS = -lm -lSDL2 -lSDL2_ttf

# comment line below, if you don't want the inserted tile to be colored green
CFLAGS += -DCOLOR_INSERTED_TILE

CFILES = $(wildcard *.c)
OFILES = $(patsubst %.c, %.o, $(CFILES))
HFILES = $(wildcard *.h)

$(TARGET): $(OFILES)
	$(LD) -o $@ $(LFLAGS) $^

%.o: %.c $(HFILES)
	$(CC) -o $@ -c $(CFLAGS) $<

.PHONY: clean

clean:
	$(RM) $(OFILES) $(TARGET)
