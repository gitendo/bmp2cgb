CFLAGS = -Wall -Wextra

SOURCES = $(wildcard *.c)
OBJS = $(patsubst %.c, %.o, $(SOURCES))

.PHONY:	all clean

all:	bmp2cgb

clean:
	$(RM) bmp2cgb *.o

bmp2cgb:	$(OBJS)
	$(CC) -o $@ $^
