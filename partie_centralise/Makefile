CC=gcc
CFLAGS=-pthread
LDFLAGS=-lrt
SERVER_PORT=8080
SERVER_IP=127.0.0.1
PIPE_NAME=message_pipe_

all: ../client_chat/client ./main #./server_com/server ./client_chat/afficheur_msg ./partie_centralise/file_message/file_msg ./partie_centralise/gestion_requete/gest_req

client: ../client_chat/client.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

main: ./main.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

server: ./server_com/server.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

afficheur_msg: ./client_chat/afficheur_msg.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

file_msg: ./file_message/file_msg.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

gest_req: ./gestion_requete/gest_req.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)



%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Define phony targets
.PHONY: clean clean_pipe run run_client run_server run_gest_req run_file_msg

clean:
	rm -f *.o ../client_chat/client ./client_chat/afficheur_msg ./server_com/server ./file_message/file_msg ./gestion_requete/gest_req  ./client_chat/message_pipe_* ./pipe_to_gestion ./pipe_gest_to_files_msg ./pipe_com_to_files_msg ./pipe_to_com 

clean_pipe:
	rm -rf ./pipe_to_gestion ./pipe_gest_to_file_msg ./pipe_com_to_file_msg ./pipe_to_com ../client_chat/message_pipe_*



run: all
	@echo "Launching Centrale part..."
	@gnome-terminal -- ./main
	@sleep 1
	@echo "Launching client..."
	@gnome-terminal -- ../client_chat/client $(SERVER_PORT) $(SERVER_IP)
	@sleep 1
	@echo "Launching client 2..."
	@gnome-terminal -- ../client_chat/client $(SERVER_PORT) $(SERVER_IP)
	@sleep 1
	@echo "Launching client 3..."
	@gnome-terminal -- ../client_chat/client $(SERVER_PORT) $(SERVER_IP)

run_client: client
	@echo "Launching client..."
	@gnome-terminal -- ../client_chat/client $(SERVER_PORT) $(SERVER_IP)


run_server: server
	@echo "Launching server..."
	@gnome-terminal -- ./server_com/server $(SERVER_PORT) 

run_gest_req: gest_req
	@echo "Launching gest_req..."
	@gnome-terminal -- ./gestion_requete/gest_req 



run_file_msg: file_msg
	@echo "Launching file_msg..."
	@gnome-terminal -- ./file_message/file_msg 




