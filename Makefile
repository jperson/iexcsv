csrc = $(wildcard *.c)
obj = $(csrc:.c=.o)

CFLAGS = -Os -Wall

iex2csv: $(obj)
	$(CC) -o $@ $^ $(CFLAGS)
