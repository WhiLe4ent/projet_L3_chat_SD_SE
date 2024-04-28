package com.clientrmi ;

import com.gestion_compte.ICompte;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.rmi.registry.LocateRegistry;
import java.rmi.registry.Registry;
import java.rmi.RemoteException;


public class ClientRMI {

    public static void main(String[] args) {
        try {
            // Adresse IP et port du serveur RMI
            String serverIP = "127.0.0.1" ; // "10.1.13.62"; // Remplacez par l'adresse IP du serveur
            int serverPort = 1099; // Port par défaut
            
            // Charger le fichier de politique de sécurité
            System.setProperty("java.security.policy", "./resources/security.policy");
            
            
            try {
                if (System.getSecurityManager() == null) {
                    System.setSecurityManager(new CustomSecurityManager());
                }
            } catch (Exception e) {
            }

            // Obtention du registre RMI du serveur
            Registry registry = LocateRegistry.getRegistry(serverIP, serverPort);

            // Recherche du stub de l'objet distant du serveur
            ICompte gestionCompte = (ICompte) registry.lookup("GestionCompte");

            // Démarrer un thread pour écouter les messages UDP
            new Thread(() -> {
                try {
                    // Créer un socket UDP pour écouter les messages
                    DatagramSocket socket = new DatagramSocket(4003); // Port arbitraire

                    byte[] buffer = new byte[2048 + 5];

                    while (true) {
                        DatagramPacket packet = new DatagramPacket(buffer, buffer.length);
                        socket.receive(packet);

                        // Extraire le message reçu
                        String message = new String(packet.getData(), 0, packet.getLength());

                        // Traiter le message
                        String action = message.substring(0, 5);
                        System.out.println("Action :  " + action);

                        String[] parts = message.substring(5).split("#");
                        String pseudo = parts[0];
                        System.out.println("Reçu pseudo : " + pseudo);

                        String mdp = parts[1];
                        System.out.println("Reçu mdp : " + mdp);

                        Boolean result = false;

                        // Exécuter l'action en fonction des 5 premiers caractères du message
                        switch (action) {
                            case "C_ACC":
                                result = gestionCompte.creerCompte(pseudo, mdp);
                                System.out.println("Compte " + (result ? "créé" : "non créé"));
                                break;
                            case "A_ACC":
                                result = gestionCompte.connexion(pseudo, mdp);
                                System.out.println((result ? "Connecté bv chef" : "Pas connecté chef"));
                                break;
                            case "D_ACC":
                                result = gestionCompte.supprimerCompte(pseudo, mdp);
                                System.out.println("Compte " + (result ? "supprimer" : "non supprimé"));

                                break;
                            default:
                                System.out.println("Action non reconnue : " + action);
                                break;
                        }

                        // Préparer la réponse
                        String response = result ? "Success" : "Failed";
                        byte[] responseData = response.getBytes();

                        // Envoyer la réponse au serveur C via UDP
                        InetAddress clientAddress = packet.getAddress();
                        int clientPort = packet.getPort();
                        DatagramPacket responsePacket = new DatagramPacket(responseData, responseData.length, clientAddress, clientPort);
                        socket.send(responsePacket);

                        System.out.println("Réponse envoyée au serveur C : " + response);
                    }
                } catch (SocketException e) {
                    e.printStackTrace();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }).start();


        } catch (RemoteException e) {
            System.err.println("Erreur RemoteException : " + e.getMessage());

        } catch (java.rmi.NotBoundException e) {
            System.err.println("Erreur NotBoundException : " + e.getMessage());

        } catch (Exception e) {
            System.err.println("Erreur inattendue : " + e.getMessage());

        }
    }
}
