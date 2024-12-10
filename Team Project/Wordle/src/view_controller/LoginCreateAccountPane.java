package view_controller;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.NotSerializableException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.util.ArrayList;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Scene;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.PasswordField;
import javafx.scene.control.TextField;
import javafx.scene.effect.DropShadow;
import javafx.scene.layout.HBox;
import javafx.scene.layout.VBox;
import javafx.scene.paint.Color;
import javafx.scene.text.Font;
import javafx.scene.text.FontWeight;
import javafx.stage.Stage;
import model.AccountCollection;
import model.User;

public class LoginCreateAccountPane extends VBox {
	/* -------- LABELS  -----------*/
	// all of the elements to add to the pane
	private Label wordle = new Label("WORDLE!");

	private Label user = new Label("Account Name: ");
	private TextField userField = new TextField();
	private HBox userBox = new HBox(user, userField);

	private PasswordField passField = new PasswordField();
	private Label pass = new Label("Password: ");
	private HBox passBox = new HBox(pass, passField);

	private Button login = new Button("Login");
	private Button register = new Button("Register");
	private HBox logRegBox = new HBox(login, register);

	private Label info = new Label("Login or Register a new account!");
	
	/*---------- END OF LABELS ----------*/

	private AccountCollection accounts;
	private WordleGUI gui;
	private GameView gameView;
	private Stage primaryStage;

	/**
     * Constructor for the login and account creation pane.
     * Initializes the UI components and sets up the scene.
     *
     * @param accounts      The collection of account data.
     * @param gui           Reference to the main GUI class for navigation.
     * @param primaryStage  The primary stage of the application to set scenes.
     */
	public LoginCreateAccountPane(AccountCollection accounts, WordleGUI gui, Stage primaryStage) {
	    this.accounts = accounts; // get accounts for login logic
	    this.gui = gui;
	    this.primaryStage = primaryStage;

	    // set children
	    this.setAlignment(Pos.CENTER);
	    this.setPadding(new Insets(10, 10, 10, 10));
	    this.setSpacing(10);
	    
	    // Align userBox and user Label a little to the left
	    HBox.setMargin(user, new Insets(0, 0, 0, -10)); // adjust the value as needed
	    
	    userBox.setAlignment(Pos.CENTER);
	    passBox.setAlignment(Pos.CENTER);
	    logRegBox.setAlignment(Pos.CENTER);
	    logRegBox.setSpacing(30);
	    this.getChildren().addAll(wordle, userBox, passBox, logRegBox, info);

	    // set buttons
	    setLoginButtons();
	    styleComponents();
	}

	/**
     * Applies styling to the components of the pane.
     * Sets up fonts, colors, effects, and spacing for various UI elements.
     */
	private void styleComponents() {
	    // Typography 
	    wordle.setFont(Font.font("Arial", FontWeight.BOLD, 32));
	    user.setFont(Font.font("Arial", FontWeight.NORMAL, 18));
	    pass.setFont(Font.font("Arial", FontWeight.NORMAL, 18));
	    userField.setFont(Font.font("Arial", 18));
	    passField.setFont(Font.font("Arial", 18));
	    info.setFont(Font.font("Arial", FontWeight.BOLD, 18));

	    // Color Scheme
	    this.setStyle("-fx-background-color: #FFFFFF;"); // Set to a white background
	    wordle.setTextFill(Color.BLACK); // Set to a standard black color
	    login.setStyle("-fx-background-color: #333333; -fx-text-fill: white; -fx-border-radius: 5; -fx-background-radius: 5;"); // Dark gray button
	    register.setStyle("-fx-background-color: #333333; -fx-text-fill: white; -fx-border-radius: 5; -fx-background-radius: 5;");

	    // Enhancements to Buttons
	    DropShadow dropShadow = new DropShadow();
	    login.setEffect(dropShadow);
	    register.setEffect(dropShadow);

	    login.setOnMouseEntered(e -> login.setStyle("-fx-background-color: #555555; -fx-text-fill: white; -fx-border-radius: 5; -fx-background-radius: 5;")); // Lighter gray when hovered
	    login.setOnMouseExited(e -> login.setStyle("-fx-background-color: #333333; -fx-text-fill: white; -fx-border-radius: 5; -fx-background-radius: 5;"));
	    
	    register.setOnMouseEntered(e -> register.setStyle("-fx-background-color: #555555; -fx-text-fill: white; -fx-border-radius: 5; -fx-background-radius: 5;"));
	    register.setOnMouseExited(e -> register.setStyle("-fx-background-color: #333333; -fx-text-fill: white; -fx-border-radius: 5; -fx-background-radius: 5;"));

	    // Spacing and Alignment
	    userBox.setSpacing(10);
	    passBox.setSpacing(10);

	    // Text Fields Style
	    userField.setStyle("-fx-border-radius: 5; -fx-background-radius: 5;");
	    passField.setStyle("-fx-border-radius: 5; -fx-background-radius: 5;");
	}


	 /**
     * Configures the login and register buttons including their action events.
     * Handles user authentication and account registration processes.
     */
	private void setLoginButtons() {
		// set user login button
		
		login.setOnAction(e -> {
			// get data
			String user = userField.getText();
			String pass = passField.getText();
			// clear fields
			userField.setText("");
			passField.setText("");
			
			try {
				if(accounts.login(user, pass)) {
					info.setText("Logged in");
					//gameView = gui.getGameView();
					DifficultyView selectDiff = gui.getDifficulty();
//					currUser = accounts.getUser(user); //Setting the current user
					
					System.out.println("Current User:" + this.accounts.getCurrUser().getUsername()); //For testing
					
					Scene gameScene = new Scene(selectDiff, 400, 400);
					primaryStage.setScene(gameScene);
					
					return;
				}else {
					info.setText("Account not found");
				}
			} catch (NotSerializableException e1) {
				// TODO Auto-generated catch block
				e1.printStackTrace();
			}

			
			
		});
		// set register button
		register.setOnAction(e -> {
			// get data
			String user = userField.getText();
			String pass = passField.getText();
			// clear fields
			userField.setText("");
			passField.setText("");
			if (user.equals("") || pass.equals("")) {
				info.setText("Need to fill out both fields!");
				return;
			}
			
			try {
				if(accounts.signup(user,pass)) {
					info.setText("Account created!");
					try {
			            saveAccounts(accounts); // Save the updated accounts list after registration
			        } catch (IOException e1) {
			            e1.printStackTrace();
			            info.setText("Error saving account information.");
			        }
					return;
				}else {
					info.setText("User name taken");
					return;
				}
			} catch (NotSerializableException e1) {
				
				e1.printStackTrace();
			}
			
		});
	}
	
	
	/**
     * Saves the current state of accounts to a file for persistence.
     * Utilizes serialization to write the AccountCollection object.
     *
     * @param accounts  The account collection to be saved.
     * @throws IOException  If an I/O error occurs during saving.
     */
	public static void saveAccounts(AccountCollection accounts) throws IOException {
	    try (ObjectOutputStream out = new ObjectOutputStream(new FileOutputStream("accounts.ser"))) {
	        out.writeObject(accounts);
	    }
	}
	
	/**
     * Loads the account collection from a file.
     * Deserializes the object data into an AccountCollection instance.
     *
     * @return The loaded AccountCollection.
     * @throws IOException            If an I/O error occurs during loading.
     * @throws ClassNotFoundException If the class of a serialized object cannot be found.
     */
	public static AccountCollection loadAccounts() throws IOException, ClassNotFoundException {
        File file = new File("accounts.ser");
        if (!file.exists()) {
            return new AccountCollection(); // if the file doesn't exist, return a new instance
        }
        try (ObjectInputStream in = new ObjectInputStream(new FileInputStream(file))) {
            return (AccountCollection) in.readObject();
        }
    }

}
