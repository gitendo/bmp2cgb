CFLAGS = -Wall

SOURCES = $(wildcard *.c)
OBJS = $(patsubst %.c, %.o, $(SOURCES))

all:	bmp2cgb

bmp2cgb:	$(OBJS)
	$(CC) -o $@ $^
