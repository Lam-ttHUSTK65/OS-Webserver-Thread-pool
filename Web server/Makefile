CC = gcc
CFLAGS = -g
LIBS = -lpthread
BINS = server 
OBJS = server.o myqueue.o

all: $(BINS)

$(BINS): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BINS) $(OBJS)
