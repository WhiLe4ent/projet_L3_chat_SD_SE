package com.gestion_compte;


import java.rmi.Remote;
import java.rmi.RemoteException;

public interface ICompte extends Remote {
    
    /**
     * Cr�ation d'un nouveau compte. Le pseudo pr�cis� ne doit pas d�j� �tre
     * utilis� par un autre compte.
     * @param pseudo le pseudo du compte
     * @param mdp le mot de passe du compte
     * @return <code>true</code> si le compte a �t� cr��, <code>false</code>
     * sinon (le pseudo est d�j� utilis�)
     */
    boolean creerCompte(String pseudo, String mdp) throws RemoteException;
    
    /**
     * Suppression d'un compte. La pr�cision du mot de passe permet de
     * s'assurer qu'un utilisateur supprime un de ses comptes.
     * @param pseudo le pseudo du compte de l'utilisateur
     * @param mdp le mot de passe du compte de l'utilisateur
     * @return <code>true</code> si la suppression est effective (couple
     * pseudo/mdp valide), <code>false</code> sinon
     */ 
    boolean supprimerCompte(String pseudo, String mdp) throws RemoteException;
    
    /**
     * Validation de la connexion d'un utilisateur au syst�me.
     * @param pseudo le pseudo du compte de l'utilisateur
     * @param mdp le mot de passe du compte de l'utilisateur
     * @return <code>true</code> s'il existe un compte avec le 
     * couple pseudo/mdp pr�cis�, <code>false</code> sinon
     */
    boolean connexion(String pseudo, String mdp) throws RemoteException;
} 