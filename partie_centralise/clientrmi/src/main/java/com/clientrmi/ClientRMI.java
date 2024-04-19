package src.main.java.com.clientrmi;

import java.rmi.registry.LocateRegistry;
import java.rmi.registry.Registry;
import java.rmi.RemoteException;


public class ClientRMI {

    public static void main(String[] args) {
        try {
            // Adresse IP et port du serveur RMI
            String serverIP = "127.0.0.1"; // Remplacez par l'adresse IP du serveur
            int serverPort = 1099; // Port par défaut
            try {
                if (System.getSecurityManager() == null) {
                  System.setSecurityManager(new CustomSecurityManager());
                }
              } catch (Exception e) {
                 e.printStackTrace();
              }

            // Obtention du registre RMI du serveur
            Registry registry = LocateRegistry.getRegistry(serverIP, serverPort);

            // Recherche du stub de l'objet distant du serveur
            ICompte gestionCompte = (ICompte) registry.lookup("GestionCompte");

            // Appel de la méthode pour créer un compte
            boolean result = gestionCompte.creerCompte("nouveauPseudo", "nouveauMDP");
            
            // Affichage du résultat
            if(result) {
                System.out.println("Compte créé avec succès.");
            } else {
                System.out.println("Échec de la création du compte.");
            }

        } catch (RemoteException e) {
            System.err.println("Erreur RemoteException : " + e.getMessage());
            e.printStackTrace();
        } catch (java.rmi.NotBoundException e) {
            System.err.println("Erreur NotBoundException : " + e.getMessage());
            e.printStackTrace();
        } catch (Exception e) {
            System.err.println("Erreur inattendue : " + e.getMessage());
            e.printStackTrace();
        }
    }
}

