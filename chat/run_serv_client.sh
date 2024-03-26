#!/bin/bash

# Compilation des fichiers source C
gcc serveur.c -o serveur
gcc client.c -o client

# Lancement du serveur dans un terminal
gnome-terminal -- ./serveur &

# Lancement du client dans un terminal
gnome-terminal -- ./client
