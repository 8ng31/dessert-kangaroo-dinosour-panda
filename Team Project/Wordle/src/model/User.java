package model;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.Arrays;

public class User implements Serializable {
	
	private String username;
	private String password;
	private int played;
	private int winRatio;
	private int currStreak;
	private int maxStreak;
	private int wins;
	private ArrayList<Integer> guessList;
	private String picPath;
	private float winLossRatio;
	
	public User(String username, String password) {
		// TODO Auto-generated constructor stub
		this.guessList = new ArrayList<>(Arrays.asList(0, 0, 0, 0, 0, 0));
		this.username = username;
		this.password = password;
	}

	public void incrementGuessCount(int round) {
	    // 'round' should be the round number (1 through 6) where the user won.
	    // Subtract 1 from the round number to get the zero-based index.
	    int index = round ;

	    // Check if the index is within the bounds of the list.
	    if (index >= 0 && index < guessList.size()) {
	        // Get the current count for the round, increment it, and update the list.
	        guessList.set(index, guessList.get(index) + 1);
	    } else {
	
	        System.err.println("Round number out of bounds for guess distribution list.");
	    }
	}
	
	public void setPicPath(String picPath) {
		this.picPath = picPath;
	}
	
	public String getPicPath() {
		return this.picPath;
	}
	
	public ArrayList getGuessList(){
		return this.guessList;
	}

	
	public String getUsername() {
		return username;
	}
	
	public boolean checkPassword(String password) {
		return this.password.equals(password);
	}
	
	
	public int getPlayed() {
        return played;
    }

    // Setter for played
    public void setPlayed() {
        this.played++;
    }

    // Getter for winRatio
    public double getWinRatio() {
        if (played == 0) {
            return 0; // To avoid division by zero
        }
        winRatio = (int) (((double) wins / (double) played) * 100);
        
        return winRatio; // Ratio as a percentage
    }

    // Getter for currStreak
    public int getCurrStreak(Boolean won) {
    	if(won == true) {
    		this.currStreak++;
    		if (this.currStreak > this.maxStreak) {
                this.maxStreak = this.currStreak;
            }
    	}
    	else {
    		this.currStreak = 0;
    	}
        return currStreak;
    }
    
    public int getCurrStreak() {
    	return this.currStreak;
    }
 

    // Getter for maxStreak
    public int getMaxStreak() {
        return maxStreak;
    }

    // Getter for wins
    public int getWins(Boolean won) {
    	if(won) {
    		this.wins++;
    	}
    
        return wins;
    }
    

    

}
