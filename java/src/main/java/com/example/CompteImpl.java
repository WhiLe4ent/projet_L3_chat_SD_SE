package com.example ;

import java.io.*;
import java.rmi.RemoteException;
import java.rmi.server.UnicastRemoteObject;
import java.util.HashMap;
import java.util.Map;

public class CompteImpl extends UnicastRemoteObject implements ICompte {
    private Map<String, String> comptes; // pseudo -> mot de passe
    private String fichierComptes; // Nom du fichier pour stocker les comptes

    protected CompteImpl(String fichierComptes) throws RemoteException {
        super();
        this.fichierComptes = fichierComptes;
        this.comptes = new HashMap<>();
        chargerComptes();
    }

    private void chargerComptes() {
        try (BufferedReader br = new BufferedReader(new FileReader(fichierComptes))) {
            String ligne;
            while ((ligne = br.readLine()) != null) {
                String[] parts = ligne.split(":");
                comptes.put(parts[0], parts[1]);
            }
        } catch (FileNotFoundException e) {
            // Le fichier n'existe pas, alors créez-le
            creerFichierComptes();
        } catch (IOException e) {
            // Le fichier ne peut pas être lu, donc ignorez
        }
    }

    private void creerFichierComptes() {
        try {
            File fichier = new File(fichierComptes);
            if (!fichier.exists()) {
                fichier.createNewFile();
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void sauvegarderComptes() {
        try (BufferedWriter bw = new BufferedWriter(new FileWriter(fichierComptes))) {
            for (Map.Entry<String, String> entry : comptes.entrySet()) {
                bw.write(entry.getKey() + ":" + entry.getValue());
                bw.newLine();
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }


    @Override
    public boolean creerCompte(String pseudo, String mdp) throws RemoteException {
        if (comptes.containsKey(pseudo)) {
            return false; 
        } else {
            comptes.put(pseudo, mdp);
            sauvegarderComptes();
            return true;
        }
    }

    @Override
    public boolean supprimerCompte(String pseudo, String mdp) throws RemoteException {

        if (comptes.containsKey(pseudo) && comptes.get(pseudo).equals(mdp)) {
            comptes.remove(pseudo);
            sauvegarderComptes();
            return true;

        } else {
            return false; 
        }
    }

    @Override
    public boolean connexion(String pseudo, String mdp) throws RemoteException {
        return comptes.containsKey(pseudo) && comptes.get(pseudo).equals(mdp);
    }

    public static void main(String[] args) {
        try {
            CompteImpl compte = new CompteImpl("comptes.txt");
            // Publiez l'objet RMI dans le registre RMI ici
            Naming.rebind("Compte", compte);
            System.out.println("Serveur Compte prêt.");
        } catch (Exception e) {
            System.err.println("Erreur : " + e);
        }
    }
}
