package model;
import java.util.Random;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class Game {
	public /*static*/ final List<String> meaningfulWords = new ArrayList<>();
    
	/*
	 * Adding 147 5-letter words to the list
	 */
	public /*static*/ void addToList(String filePath) throws IOException {
		//String filePath = "./wordle-words.txt";
		BufferedReader reader = new BufferedReader(new FileReader(filePath));
		String line;
        while ((line = reader.readLine()) != null) {
            // Split the line into words using space as the delimiter
            String[] words = line.split("\n");
            for (String word : words) {
                // Add the word to the list (you can also perform any desired preprocessing here)
                meaningfulWords.add(word);
            }
        }
	}
	
	public String randomWordGenerator() {
		Random random = new Random();
        int randomIndex = random.nextInt(meaningfulWords.size());
        return meaningfulWords.get(randomIndex);
	}
	
	
    
	public Game(String string) throws IOException {
		addToList(string);
    }
	
	public String randomWord() {
		String randomWord = randomWordGenerator();
        System.out.println("Random "+ randomWord.length() + "-letter word: " + randomWord);
		return randomWord;
	}
}