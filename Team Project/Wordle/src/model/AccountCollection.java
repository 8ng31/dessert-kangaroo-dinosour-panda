package model;



import java.io.NotSerializableException;
import java.io.Serializable;
import java.util.HashMap;



public class AccountCollection implements Serializable {
	private static final long serialVersionUID = 1L; // Recommended for better version control

    public HashMap<String, User> accounts;
    private User currUser;

    public AccountCollection() {
    	this.currUser = null;
        this.accounts = new HashMap<>();
    }
    
    public boolean signup(String username, String password) throws NotSerializableException {
        if (accounts.containsKey(username)) {
            return false; // Username already exists
        }
        
        User new_account = new User(username, password);
        accounts.put(username, new_account);
        
        return true;
    }

    public boolean login(String username, String password) throws NotSerializableException {
    	User account = accounts.get(username);
        if (account == null) {
            return false; // Account doesn't exist
        }
        currUser = account;
        return account.checkPassword(password);
    }
    
    public HashMap<String, User> getAccounts() {
    	return this.accounts;
    }
    
    
    public User getCurrUser() {
    	return this.currUser;
    }
    
    public int getSize() {
    	return this.accounts.size();
    }
}

