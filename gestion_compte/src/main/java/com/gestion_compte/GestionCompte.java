package com.gestion_compte ;

import java.io.*;
import java.rmi.RemoteException;
import java.rmi.server.UnicastRemoteObject;
import java.util.ArrayList;
import java.util.List;
import java.rmi.registry.LocateRegistry;
import java.rmi.registry.Registry;

public class GestionCompte extends UnicastRemoteObject implements ICompte {
    private static final String FILE_PATH = "./src/main/resources/bdd_compte.txt";

    protected GestionCompte() throws RemoteException {
        super();
    }

    @Override
    public boolean creerCompte(String pseudo, String mdp) throws RemoteException {
        // Vérifier si le pseudo est déjà utilisé
        if (compteExisteDeja(pseudo.trim())) {
            System.out.println("Le pseudo est déjà utilisé.");
            return false;
        }

        // Ajouter le compte à la liste des comptes
        try {
            try (BufferedWriter writer = new BufferedWriter(new FileWriter(FILE_PATH, true))) {
                writer.write(pseudo.trim() + "#" + mdp.trim()); // Ajouter le pseudo et le mot de passe séparés par #
                writer.newLine(); // Nouvelle ligne pour le prochain compte
            } // Ajouter le pseudo et le mot de passe séparés par une virgule
            System.out.println("Compte créé pour " + pseudo);
            return true;
        } catch (IOException e) {
            System.err.println("Erreur lors de l'écriture dans le fichier.");
            return false;
        }
    }

    private boolean compteExisteDeja(String pseudo) {
        List<String> comptes = chargerComptes();
        for (String compte : comptes) {
            String[] infoCompte = compte.split("#");
            if (infoCompte[0].equals(pseudo.trim())) {
                return true;
            }
        }
        return false;
    }

    
    @Override
    public boolean supprimerCompte(String pseudo, String mdp) throws RemoteException {
        // Charger les comptes depuis la base de données
        List<String> comptes = chargerComptes();
        
        // Vérifier si le compte existe et si le mot de passe correspond
        boolean compteTrouve = false;
        for (int i = 0; i < comptes.size(); i++) {
            String[] infoCompte = comptes.get(i).split("#"); // Utiliser le bon séparateur
            if (infoCompte[0].equals(pseudo.trim()) && infoCompte[1].equals(mdp.trim())) {
                // Si le compte est trouvé et le mot de passe correspond, le supprimer de la base de données
                comptes.remove(i);
                compteTrouve = true;
                break;
            }
        }
    
        // Si le compte a été trouvé et supprimé, mettre à jour la base de données
        if (compteTrouve) {
            if (sauvegarderComptes(comptes)) {
                System.out.println("Compte supprimé pour " + pseudo);
                return true;
            } else {
                System.out.println("Erreur lors de la sauvegarde de la base de données.");
                return false;
            }
        } else {
            // Si le compte n'est pas trouvé ou le mot de passe ne correspond pas, renvoyer false
            System.out.println("Compte introuvable ou mot de passe incorrect.");
            return false;
        }
    }
    
    private boolean sauvegarderComptes(List<String> comptes) {
        // Sauvegarder les comptes dans la base de données
        try {
            try (BufferedWriter writer = new BufferedWriter(new FileWriter(FILE_PATH))) {
                for (String compte : comptes) {
                    writer.write(compte);
                    writer.newLine();
                }
            }
            return true;
        } catch (IOException e) {
            System.err.println("Erreur lors de la sauvegarde de la base de données.");
            return false;
        }
    }
    
    private List<String> chargerComptes() {
        List<String> comptes = new ArrayList<>();
        try (BufferedReader reader = new BufferedReader(new FileReader(FILE_PATH))) {
            String line;
            while ((line = reader.readLine()) != null) {
                line = line.trim(); // Supprimer les espaces en début et fin de ligne
                if (!line.isEmpty()) { // Ignorer les lignes vides
                    comptes.add(line);
                }
            }
        } catch (IOException e) {
            System.err.println("Erreur lors de la lecture du fichier.");
        }
        return comptes;
    }
    
    
    @Override
    public boolean connexion(String pseudo, String mdp) throws RemoteException {
        // Charger les comptes depuis la base de données
        List<String> comptes = chargerComptes();
        
        // Vérifier si le compte existe et si le mot de passe correspond
        for (String compte : comptes) {
            String[] infoCompte = compte.split("#"); 
            if (infoCompte.length == 2) {
                String pseudoFromFile = infoCompte[0].trim();
                String mdpFromFile = infoCompte[1].trim();
                if (pseudoFromFile.equals(pseudo.trim()) && mdpFromFile.equals(mdp.trim())) {
                    // Si le compte est trouvé et le mot de passe correspond, renvoyer true
                    System.out.println("Connexion réussie pour " + pseudo + " " + mdp);
                    return true;
                }
            }
        }
    
        // Si le compte n'est pas trouvé ou le mot de passe ne correspond pas, renvoyer false
        System.out.println("Connexion échouée pour " + pseudo + " " + mdp + ": Pseudo ou mot de passe incorrect.");
        return false;
    }
    
    

    public void handleMessage(String message) throws RemoteException {
        // Séparer le préfixe et les données du message
        //String action = message.substring(0, 5);
        String[] parts = message.split("#");
        System.out.println("Sep parts " + parts[0] + " " + parts[1]);

        String prefix = parts[0];
        String data = parts[1];
    
        // Extraire le pseudo et le mot de passe
        String[] credentials = data.split("#");
        String pseudo = credentials[0];
        String mdp = credentials[1];
    
        boolean result = false; // Initialiser le résultat à faux
    
        // Exécuter la bonne fonction en fonction du préfixe
        switch (prefix) {
            case "C_ACC":
                // Action pour le préfixe C_ACC
                System.out.println("Demande de création de compte reçue pour " + pseudo + ".");
                // Appel de la méthode pour créer un compte
                result = creerCompte(pseudo, mdp);
                break;
            case "A_ACC":
                // Action pour le préfixe A_ACC (connexion)
                System.out.println("Demande de connexion reçue pour " + pseudo + ".");
                // Appel de la méthode pour connexion
                result = connexion(pseudo, mdp);
                break;
            case "D_ACC":
                // Action pour le préfixe D_ACC (suppression de compte)
                System.out.println("Demande de suppression de compte reçue pour " + pseudo + ".");
                // Appel de la méthode pour supprimer un compte
                result = supprimerCompte(pseudo, mdp);
                break;
            default:
                System.out.println("Préfixe non reconnu : " + prefix);
                break;
        }
    
        // Afficher le résultat
        System.out.println("Résultat de l'opération : " + result);
    }
    
    public static void main(String[] args) {
        try {
            // Création de l'objet distant
            GestionCompte gestionCompte = new GestionCompte();
    
            // Export de l'objet distant pour qu'il puisse recevoir des appels distants
            // GestionCompte stub = gestionCompte;
    
            // Obtention du registre RMI
            Registry registry = LocateRegistry.createRegistry(1099);
    
            // Enregistrement du stub de l'objet distant dans le registre RMI
            registry.rebind("GestionCompte", gestionCompte);
    
            System.out.println("Serveur RMI prêt.");
        } catch (RemoteException e) {
            System.err.println("Erreur lors du démarrage du serveur RMI : " + e.toString());
        }
    }
}
