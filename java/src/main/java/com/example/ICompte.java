package com.example ;

import java.rmi.Remote;
import java.rmi.RemoteException;

public interface ICompte extends Remote {
    
    /**
     * Création d'un nouveau compte.Le pseudo précisé ne doit pas déjà être
     * utilisé par un autre compte.
     * @param pseudo le pseudo du compte
     * @param mdp le mot de passe du compte
     * @return <code>true</code> si le compte a été créé, <code>false</code>
     * sinon (le pseudo est déjà utilisé)
     * @throws java.rmi.RemoteException
     */
    boolean creerCompte(String pseudo, String mdp) throws RemoteException;
    
    /**
     * Suppression d'un compte.La précision du mot de passe permet de
     * s'assurer qu'un utilisateur supprime un de ses comptes.
     * @param pseudo le pseudo du compte de l'utilisateur
     * @param mdp le mot de passe du compte de l'utilisateur
     * @return <code>true</code> si la suppression est effective (couple
     * pseudo/mdp valide), <code>false</code> sinon
     * @throws java.rmi.RemoteException
     */ 
    boolean supprimerCompter(String pseudo, String mdp) throws RemoteException;
    
    /**
     * Validation de la connexion d'un utilisateur au système.
     * @param pseudo le pseudo du compte de l'utilisateur
     * @param mdp le mot de passe du compte de l'utilisateur
     * @return <code>true</code> s'il existe un compte avec le 
     * couple pseudo/mdp précisé, <code>false</code> sinon
     * @throws java.rmi.RemoteException
     */
    boolean connexion(String pseudo, String mdp) throws RemoteException;
} 