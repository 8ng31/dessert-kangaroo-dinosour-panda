package model;

public class UserInfo implements Comparable<UserInfo>{
	private String name;
	private String picPath;
	private double avgGuess;
	
	public UserInfo(String name, String picPath, double avgGuess) {
		this.name = name;
		this.avgGuess = avgGuess;
		this.picPath = picPath;
	}
	
	public double getAVGguess() {
		return avgGuess;
	}
	public String getName() {
		return name;
	}
	public String getPath() {
		return picPath;
	}
	
	public void setAVGguess(double avgGuess) {
		this.avgGuess = avgGuess;
	}

	//this should compare the avg guesses of both user info in order
	//to allow for accurate sorting of a user list
	@Override
	public int compareTo(UserInfo o) {
		return (int) (this.avgGuess - ((UserInfo)o).getAVGguess());
	}
	
	@Override
	public String toString() {
		String str = "{Name: " + name + ", avg guess: " + avgGuess + "}";
		return str;
	}
	
}
