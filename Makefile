CC=gcc
CFLAGS=-pthread
LDFLAGS=-lrt
SERVER_PORT=8080
PIPE_NAME=message_pipe

all: ./client_chat/client ./partie_centralise/main #./partie_centralise/server_com/server ./client_chat/afficheur_msg ./partie_centralise/file_message/file_msg ./partie_centralise/gestion_requete/gest_req

client: ./client_chat/client.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

main: ./partie_centralise/main.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# server: ./partie_centralise/server_com/server.c
# 	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# afficheur_msg: ./client_chat/afficheur_msg.c
# 	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# file_msg: ./partie_centralise/file_message/file_msg.c
# 	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# gest_req: ./partie_centralise/gestion_requete/gest_req.c
# 	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)



%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o ./client_chat/client ./partie_centralise/server_com/server ./client_chat/message_pipe_* ./client_chat/afficheur_msg ./partie_centralise/pipe_to_gestion ./partie_centralise/pipe_gest_to_files_msg ./partie_centralise/pipe_com_to_files_msg ./partie_centralise/pipe_to_com 

clean_pipe:
	rm -rf ./partie_centralise/pipe_to_gestion ./partie_centralise/pipe_gest_to_file_msg ./partie_centralise/pipe_com_to_file_msg ./partie_centralise/pipe_to_com ./client_chat/message_pipe_*

# @echo "Launching server..."
# @gnome-terminal -- ./partie_centralise/server_com/server $(SERVER_PORT) &
# @sleep 1
# @gnome-terminal -- ./partie_centralise/file_message/file_msg &
# @sleep 1
# @gnome-terminal -- ./partie_centralise/gestion_requete/gest_req &
# @sleep 1
run: all
	@echo "Launching Centrale part..."
	@gnome-terminal -- ./partie_centralise/main
	@sleep 1
	@echo "Launching client..."
	@gnome-terminal -- ./client_chat/client
	@sleep 1
	@echo "Launching client 2..."
	@gnome-terminal -- ./client_chat/client
	@sleep 1
	@echo "Launching client 3..."
	@gnome-terminal -- ./client_chat/client

run_client: all

	@echo "Launching client..."
	@gnome-terminal -- ./client_chat/client
