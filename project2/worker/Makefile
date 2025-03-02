PROGS = worker

CFLAGS = -g -lpthread

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS)  -o $@ $<

clean:
	rm $(PROGS)
