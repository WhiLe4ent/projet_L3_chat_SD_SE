package com.example ;

import java.rmi.Remote;
import java.rmi.RemoteException;

public interface ICompte extends Remote {
    
    /**
     * Creation d'un nouveau compte. Le pseudo precise ne doit pas deje etre
     * utilise par un autre compte.
     * @param pseudo le pseudo du compte
     * @param mdp le mot de passe du compte
     * @return <code>true</code> si le compte a ete cree, <code>false</code>
     * sinon (le pseudo est deje utilise)
     */
    boolean creerCompte(String pseudo, String mdp) throws RemoteException;
    
    /**
     * Suppression d'un compte. La precision du mot de passe permet de
     * s'assurer qu'un utilisateur supprime un de ses comptes.
     * @param pseudo le pseudo du compte de l'utilisateur
     * @param mdp le mot de passe du compte de l'utilisateur
     * @return <code>true</code> si la suppression est effective (couple
     * pseudo/mdp valide), <code>false</code> sinon
     */ 
    boolean supprimerCompter(String pseudo, String mdp) throws RemoteException;
    
    /**
     * Validation de la connexion d'un utilisateur au systeme.
     * @param pseudo le pseudo du compte de l'utilisateur
     * @param mdp le mot de passe du compte de l'utilisateur
     * @return <code>true</code> s'il existe un compte avec le 
     * couple pseudo/mdp precise, <code>false</code> sinon
     */
    boolean connexion(String pseudo, String mdp) throws RemoteException;
} 