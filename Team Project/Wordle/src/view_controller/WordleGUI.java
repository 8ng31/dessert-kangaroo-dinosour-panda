package view_controller;

import java.io.FileOutputStream;
import java.io.IOException;
import java.io.ObjectOutputStream;
import java.util.ArrayList;
import java.util.Arrays;

import javafx.application.Application;
import javafx.scene.Scene;
import javafx.scene.layout.BorderPane;
import javafx.stage.Stage;

import model.AccountCollection;
import model.Game;
import model.User;

/**
 * This is the main class for the Wordle game application.
 * It sets up the primary stage for the application and initializes
 * the necessary panes for login, game view, and statistics display.
 */
public class WordleGUI extends Application {
    /**
     * Main entry point for the application.
     * @param args command line arguments
     */
    public static void main(String[] args) {
        launch(args);
    }
    
    private Stage primaryStage; // the primary stage for the application
    public static AccountCollection accounts; // collection of player accounts
    private LoginCreateAccountPane loginPane; // login screen pane
    public static GameView gameView; // pane for playing a single game of Wordle
    private StatsPage statsPage; // pane for displaying user statistics
    private User currUser; // the current user
    private DifficultyView difficulty;
    private Game normal;
    private Game hard;
    private Game extreme;
    
    /**
     * Starts the primary stage and initializes the GUI layout.
     * It also sets the scene to the login pane and shows the primary stage.
     * 
     * @param primaryStage the primary window of the application
     * @throws IOException if there is an I/O problem loading accounts
     */
    @Override
    public void start(Stage primaryStage) throws IOException {
        this.primaryStage = primaryStage;
        
        initializeGames();
        LayoutGUI();
        saveAccounts(accounts);
        primaryStage.setTitle("Wordle");
        Scene loginScene = new Scene(loginPane, 600, 600);
        primaryStage.setScene(loginScene);
        primaryStage.show();
    }

    /**
     * Initializes games with different word dictionaries.
     * @throws IOException 
     */
    private void initializeGames() throws IOException {
		normal = new Game("./wordle-words.txt");
		hard = new Game("./wordle-words-7.txt");
		extreme = new Game("./wordle-words-10.txt");
		
	}

    
    public DifficultyView getDifficulty() {
    	return difficulty;
    }
    
    public Game getNormal() {
    	return normal;
    }
    
    public Game getHard() {
    	return hard;
    }
    
    public Game getExtreme() {
    	return extreme;
    }

	/**
     * Sets up the layout for the GUI, loads accounts, and initializes
     * the various panes that make up the application.
     * 
     * @throws IOException if there is an I/O problem loading accounts
     */
    void LayoutGUI() throws IOException {
        accounts = new AccountCollection();
        accounts.signup("Tom", "1");
        
        try {
            accounts = LoginCreateAccountPane.loadAccounts();
        } catch (IOException | ClassNotFoundException e) {
            e.printStackTrace();
            accounts = new AccountCollection();
        }
        
        loginPane = new LoginCreateAccountPane(accounts, this, primaryStage);
        statsPage = new StatsPage(accounts, this, primaryStage);
        gameView = new GameView(accounts, this, primaryStage, "normal");
        difficulty = new DifficultyView(this, primaryStage);
        statsPage.setCurrentGameView(gameView);
    }
    
    /**
     * Retrieves the current GameView instance.
     * 
     * @return the gameView instance for playing the game
     */
    public GameView getGameView() {
        return gameView;
    }
    
    /**
     * Retrieves the current StatsPage instance.
     * 
     * @return the statsPage instance for displaying statistics
     */
    public StatsPage getStatsPage() {
        return statsPage;
    }
    
    /**
     * Saves the current accounts to a file.
     * 
     * @param accounts the account collection to be saved
     * @throws IOException if there is an I/O error during saving
     */
    public void saveAccounts(AccountCollection accounts) throws IOException {
        try (ObjectOutputStream out = new ObjectOutputStream(new FileOutputStream("accounts.ser"))) {
            out.writeObject(accounts);
        }
    }

}
