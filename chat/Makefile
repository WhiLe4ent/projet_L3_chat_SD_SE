CC=gcc
CFLAGS=-pthread
LDFLAGS=-lrt
SERVER_PORT=8080
PIPE_NAME=message_pipe

all: client server 

client: client.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

server: server.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

afficheur_msg: afficheur_msg
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o client server 

run: all
	@echo "Launching server..."
	@gnome-terminal -- ./server $(SERVER_PORT) &
	@sleep 1
	@echo "Launching client..."
	@gnome-terminal -- ./client


