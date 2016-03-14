.PHONY : all clean
all: Client Server
CFLAGS =-Wall
LIBS = -lm

Client: Client.o
	$(CC) $(LDFLAGS) $^ -o $@

Server: Server.o
	$(CC) $(LDFLAGS) $^ -o $@

clean:
	rm -rf *.o Client Server
	rm *\~
