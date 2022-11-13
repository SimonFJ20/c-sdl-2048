
TARGET = game

CC = gcc
LD = gcc

CFLAGS = -Wall -Wpedantic
LFLAGS = -lm -lSDL2 -lSDL2_ttf

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
