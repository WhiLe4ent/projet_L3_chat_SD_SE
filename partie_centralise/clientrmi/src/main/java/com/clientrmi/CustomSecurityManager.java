package src.main.java.com.clientrmi;



public class CustomSecurityManager extends SecurityManager {
    @Override
    public void checkConnect(String host, int port) {
        // Permettre toutes les connexions
    }

    @Override
    public void checkConnect(String host, int port, Object context) {
        // Permettre toutes les connexions
    }

    @Override
    public void checkPermission(java.security.Permission perm) {
        // Ignorer les vérifications de permission pour simplifier
    }
}