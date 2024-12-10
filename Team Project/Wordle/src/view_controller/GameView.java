/**
 * Authors: Dirty Cache
 */

package view_controller;

import java.io.File;
import java.io.IOException;
import java.util.Map;
import javafx.animation.TranslateTransition;
import javafx.event.EventHandler;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Scene;
import javafx.scene.control.Alert;
import javafx.scene.control.Alert.AlertType;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.Menu;
import javafx.scene.control.MenuBar;
import javafx.scene.control.MenuItem;
import javafx.scene.control.TextField;
import javafx.scene.control.TextFormatter;
import javafx.scene.image.Image;
import javafx.scene.image.ImageView;
import javafx.scene.input.KeyCode;
import javafx.scene.input.KeyEvent;
import javafx.scene.layout.BorderPane;
import javafx.scene.layout.HBox;
import javafx.scene.layout.StackPane;
import javafx.scene.layout.VBox;
import javafx.scene.media.Media;
import javafx.scene.media.MediaPlayer;
import javafx.scene.paint.Color;
import javafx.scene.text.Font;
import javafx.stage.Stage;
import javafx.util.Duration;
import model.AccountCollection;
import model.Game;
import model.User;

import java.net.URI;

/**
 * The GameView class represents the main game interface for the Wordle game.
 * It extends BorderPane and manages the layout for the game's GUI, including
 * the input fields for guesses, the on-screen keyboard, and various game controls.
 */

public class GameView extends BorderPane {

	private TextField guesses[][];
	private final int ROWS = 6; // Decide size of wordle guesses
	private int COLS; // Decide length of word
	private int row;
	private int col;
	private VBox container;
	private Button letters[];
	private VBox keyboard;
	private Game game;
	private String correctAnswer;
	private Boolean win;
	private EventHandler<KeyEvent> enterKey;
	private EventHandler<KeyEvent> deleteKey;
	private StatsPage stats;
	private String difficulty;
	
	private WordleGUI gui;
	private Stage primaryStage;
	private AccountCollection accounts;
	
	 /**
     * Constructs a new GameView with a reference to the account collection,
     * the main Wordle GUI controller, and the primary stage of the application.
     * 
     * @param accounts The collection of user accounts.
     * @param wordleGUI The main Wordle GUI controller.
     * @param primaryStage The primary stage for the application.
     * @throws IOException If an I/O error occurs.
     */
	public GameView(AccountCollection accounts, WordleGUI wordleGUI, Stage primaryStage, String difficulty) throws IOException {
		this.accounts = accounts;
		gui = wordleGUI;
		this.primaryStage = primaryStage;
		this.difficulty = difficulty;
		this.COLS = difficulty.length();
		initialize();
	}

	/**
     * Initializes the game view by setting up the text fields for guesses,
     * generating the on-screen keyboard, and styling the components.
     * 
     * @throws IOException If an I/O error occurs during initialization.
     */
	private void initialize() throws IOException {
		container = new VBox();
		container.setSpacing(5);
		container.setPadding(new Insets(7));
		keyboard = new VBox();
		keyboard.setSpacing(5);
		game = getGame(difficulty);
		win = false;
		row = 0;
		col = 0;
		System.out.println("Getting word now");
		correctAnswer = game.randomWord().toUpperCase();
		guesses = new TextField[ROWS][COLS];
		letters = new Button[27];
		activateEnterKey();
		activateDeleteKey();
		createTextFields();
		gen_keyboard();
		setStyle();

		setTopButtonsAndTitle();
		this.setCenter(container);
		this.setBottom(keyboard);
	}

	/**
	 * Selected the appropriate word generator for the difficulty selected.
	 * 
	 * @param difficulty, difficulty of game
	 * @return word generator.
	 */
	private Game getGame(String difficulty) {
		if(difficulty.equals("normal")) return gui.getNormal();
		System.out.println("Did not get normal");
		if(difficulty.equals("hard")) return gui.getHard();
		if(difficulty.equals("extreme")) return gui.getExtreme();
		return null;
	}

	/**
	 * Changes the difficulty set and initializes the pane.
	 * 
	 * @param newMode
	 * @param size
	 * @throws IOException
	 */
	public void changeDifficulty(String newMode, int size) throws IOException {
		difficulty = newMode;
		COLS = size;
		initialize();
	}
	  
	/**
     * Styles the components of the game view, including the background color
     * and text field styles.
     */
	private void setStyle() {
		this.setStyle("-fx-background-color: #121212;");
		for (int r = 0; r < ROWS; r++) {
			for (int c = 0; c < COLS; c++) {
				guesses[r][c].setStyle("-fx-background-color: #121212; -fx-text-fill: #FFFFF0; -fx-border-color: white;"
						+ "-fx-font-family: 'Comic Sans MS'");

			}
		}

	}

	/**
	 * Sets a listener for the enter key
	 */
	private void activateEnterKey() {
		enterKey = e -> {
			if (e.getCode() == KeyCode.ENTER) {
				try {
					enterKey();
				} catch (InterruptedException e1) {
					e1.printStackTrace();
				}

			}

		};

	}

	/**
	 * Sets a listener for the delete key.
	 */
	private void activateDeleteKey() {
		deleteKey = e -> {
			if (e.getCode() == KeyCode.DELETE || e.getCode() == KeyCode.BACK_SPACE) {
				deleteKey();
			}
		};
	}

	 /**
     * Handles the logic when the delete key is pressed by the user.
     * It clears the text from the current text field and moves the focus
     * back to the previous text field if applicable.
     */
	private void deleteKey() {
		guesses[row][col].setText("");
		if (col != 0) {
			guesses[row][col].setEditable(false);
			guesses[row][--col].requestFocus();
			guesses[row][col].setEditable(true);
		}
	}

	 /**
     * Handles the logic when the enter key is pressed by the user.
     * It gathers the text from the text fields, checks for correctness,
     * and updates the game state accordingly.
	 * @throws InterruptedException 
     */
	private void enterKey() throws InterruptedException {
		if (col != COLS - 1 || row >= ROWS)
			return;
		if (guesses[row][col].getText() == "") {
			return;
		}
		if (row >= ROWS)
			return;
		String string = "";
		for (int i = 0; i < COLS; i++) {
			string = string.concat(guesses[row][i].getText());
			guesses[row][i].setEditable(false);
		}
		if (game.meaningfulWords.contains(string.toLowerCase()) == false) {
			alertError();
			return;
		}
		String[] userWord = string.split("");
		String[] correctWordArray = correctAnswer.split("");
		int correctCharacters = 0;
		for (int i = 0; i < userWord.length; i++) {
			char firstChar = (userWord[i]).charAt(0);
			int number = firstChar - 'A' + 1;
			if (userWord[i].equals(correctWordArray[i])) {
				correctWordArray[i] = "-";
				guesses[row][i].setStyle("-fx-background-color: lightgreen;");
				letters[number].setStyle("-fx-color: lightgreen;");
				correctCharacters += 1;
				if (correctCharacters == COLS) { // When player wins
					winAnimation();
					//alertWinner(row + 1);
					playWinSound(true);
					
					win = true;
					statsSetting(win); // When player wins

				}
			}
			else if (lookForLetter(correctWordArray, firstChar)) {
				guesses[row][i].setStyle("-fx-background-color: yellow;");
				letters[number].setStyle("-fx-color: yellow;");
			} else {
				guesses[row][i].setStyle("-fx-background-color: lightgrey;");
				letters[number].setStyle("-fx-color: lightgrey;");
			}
		}
		row++;
		col = 0;
		if (row < ROWS && win == false) {
			guesses[row][col].setEditable(true);
			guesses[row][col].requestFocus();
		} else if (row >= ROWS && win == false) {
			playWinSound(false);
			alertLoser(correctAnswer);
			statsSetting(false); // When player loses
		} else {

		}
	}
	
	/**
	 * Plays a sound effect based on the game outcome.
	 * 
	 * This method selects and plays an audio file depending on whether the game was won or lost.
	 * The method determines the appropriate sound file (win or lose) and then constructs a {@link MediaPlayer}
	 * to play the selected sound. It handles the file path in a way that ensures compatibility across different operating systems.
	 * 
	 * 
	 * @param won A boolean indicating the game outcome. If {@code true}, the win sound is played; otherwise, the lose sound is played.
	 */
	private void playWinSound(Boolean won) {
		String path;
		if(won) {
			 path = "sounds/win.mp3";
		}
		else {
			 path = "sounds/lose.mp3";
		}
		
        // Need a File and URI object so the path works on all OSs
        File file = new File(path);
        URI uri = file.toURI();

        Media media = new Media(uri.toString());
        MediaPlayer mediaPlayer = new MediaPlayer(media);
        mediaPlayer.play();
	     
	}
	
	

	/**
     * Plays an animation sequence when the user guesses the correct word.
     * Each text field containing the correct letter will jump in a sequential pattern.
     */
	private void winAnimation() {
		for (int i = 0; i < COLS; i++) { // Exclude the button
			TextField textField = guesses[row][i];

			// Create a TranslateTransition for each TextField
			TranslateTransition jumpTransition = new TranslateTransition(Duration.seconds(0.5), textField);
			jumpTransition.setFromY(0);
			jumpTransition.setToY(-20);
			jumpTransition.setFromY(-20);
			jumpTransition.setToY(0);
			jumpTransition.setCycleCount(1); // Set to 1 for only one cycle
			jumpTransition.setAutoReverse(true);
			jumpTransition.setDelay(Duration.seconds(i * 0.2)); // Add a delay for staggered effect

			jumpTransition.play();
		}
	}

	
	/**
     * Updates the user's game statistics and saves the account information
     * when the game is won or lost.
     * 
     * @param won Indicates whether the user won or lost the game.
     */
	private void statsSetting(boolean won) {

		stats = gui.getStatsPage();
		User currUser = this.accounts.getCurrUser();

		if (won) {
			currUser.setPlayed();
			currUser.getWins(won);

			// Save Accounts after winning
			stats.setStats(currUser.getPlayed(), (int) currUser.getWinRatio(), currUser.getCurrStreak(won),
					currUser.getMaxStreak());

			currUser.incrementGuessCount(row);
			stats.setGuessDistribution(currUser.getGuessList());

			// Saving account serializable
			try {
				// Attempt to save existing accounts
				LoginCreateAccountPane.saveAccounts(accounts);
			} catch (IOException e) {
				// If accounts cannot be saved
				e.printStackTrace();

			}

		}

		else {
			currUser.setPlayed();
			stats.setStats(currUser.getPlayed(), (int) currUser.getWinRatio(), currUser.getCurrStreak(won),
					currUser.getMaxStreak());
			try {
				// Attempt to save existing accounts
				LoginCreateAccountPane.saveAccounts(accounts);
			} catch (IOException e) {
				// If accounts cannot be saved
				e.printStackTrace();

			}

		}

	}

	/**
     * Creates an alert to notify the user that they have not entered a valid word.
     * It resets the text fields for the current guess.
     */
	private void alertError() {
		Alert alert = new Alert(AlertType.INFORMATION);
		alert.setTitle("ERROR!!!");
		alert.setHeaderText("Please enter a valid word");
		alert.showAndWait();
		for (int i = 0; i < COLS; i++) {
			guesses[row][i].setText("");
			guesses[row][i].setEditable(false);
		}
		col = 0;
		guesses[row][col].requestFocus();
		guesses[row][col].setEditable(true);
	}

	/**
     * Creates an alert to congratulate the user for guessing the correct word.
     * 
     * @param tries The number of attempts it took the user to guess the word.
     */
	private void alertWinner(int tries) {
		System.out.println("Correct Answer");
		Alert alert = new Alert(AlertType.INFORMATION);
		alert.setTitle("Winner winner chicken dinner");
		alert.setHeaderText("'All I wanna say is Congratulations' ~Post Malone");
		alert.setContentText("You got it in " + tries + " guess(es)");
		alert.showAndWait();
	}

	 /**
     * Creates an alert to notify the user that they have failed to guess the word
     * within the given number of tries.
     * 
     * @param correctAnswer The correct word the user was trying to guess.
     */
	private void alertLoser(String correctAnswer) {
		System.out.println("Failed to find word");
		Alert alert = new Alert(AlertType.INFORMATION);
		alert.setTitle("Loser loser vegetable dinner");
		alert.setHeaderText("'You miss all the shots you don't take, or in this case, words' ~Angel Benavides");
		alert.setContentText("Sucks to suck because the correct answer is " + correctAnswer);
		alert.showAndWait();
		try {
			initialize();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

	}

	/**
     * Searches the given array of correct word characters for a specific letter.
     * 
     * @param correctWordArray The array of characters from the correct word.
     * @param firstChar The character to search for within the array.
     * @return true if the letter is found, false otherwise.
     */
	private boolean lookForLetter(String[] correctWordArray, char firstChar) {
		for (int i = 0; i < correctWordArray.length; i++) {
			if (correctWordArray[i].charAt(0) == firstChar) {
				correctWordArray[i] = "-";
				return true;
			}
		}
		return false;
	}

	/**
	 * Creates a menu items and adds to menu bar.
	 * 
	 * @return Menu Bar
	 */
	  private MenuBar setMenu() {
			MenuBar menuBar = new MenuBar();
			Menu options = new Menu("New Game");
			
			MenuItem normal = new MenuItem("Normal");
			normal.setOnAction(e -> {
				try {
					changeDifficulty("normal", 5);
				} catch (IOException e1) {
					// TODO Auto-generated catch block
					e1.printStackTrace();
				}
			});

			MenuItem hard = new MenuItem("Hard");
			hard.setOnAction(e -> {
				try {
					changeDifficulty("hard", 7);
				} catch (IOException e1) {
					// TODO Auto-generated catch block
					e1.printStackTrace();
				}
			});
			
			MenuItem extreme = new MenuItem("Extreme");
			extreme.setOnAction(e -> {
				try {
					changeDifficulty("extreme", 10);
				} catch (IOException e1) {
					// TODO Auto-generated catch block
					e1.printStackTrace();
				}
			});
				
			options.getItems().addAll(normal, hard, extreme);
			menuBar.getMenus().addAll(options);
			return menuBar;
		}
	
	/**
     * Sets up the top buttons and title for the game view.
     */
	private void setTopButtonsAndTitle() {
		// Create a BorderPane for overall layout
		BorderPane borderPane = new BorderPane();

		MenuBar newGame = setMenu();

		// Title in the center
		Label titleLabel = new Label("Wordle");
		titleLabel.setFont(new Font("Courier New", 24));
		titleLabel.setTextFill(Color.WHITE);
		StackPane centerStack = new StackPane(titleLabel);
		centerStack.setAlignment(Pos.CENTER);

		// Buttons on the right
		HBox buttonBox = new HBox(20);
		buttonBox.setAlignment(Pos.CENTER_RIGHT);
		buttonBox.setPadding(new Insets(0, 20, 0, 0));
		buttonBox.setPrefHeight(40);

		// Create and add buttons with icons and actions
		Button leaderboardButton = new Button();
		ImageView leaderboardIcon = new ImageView(new Image("file:images/leaderboard.png"));
		leaderboardIcon.setFitWidth(50); // Set the fit width of the icon
		leaderboardIcon.setFitHeight(50); // Set the fit height of the icon
		leaderboardIcon.setPreserveRatio(true); // Preserve the ratio
		leaderboardButton.setGraphic(leaderboardIcon); // Set the icon to the button
		leaderboardButton.setStyle("-fx-border-width: 0; -fx-background-radius: 0; -fx-background-color: transparent;");
		leaderboardButton.setMinSize(20, 20); // Set the minimum size of the button
		leaderboardButton.setPrefSize(20, 20); // Set the preferred size of the button
		leaderboardButton.setMaxSize(20, 20); // Set the maximum size of the button

		leaderboardButton.setOnAction(e -> {

			addLeaderBoard();

		});

		Button gameStatsButton = new Button();
		ImageView gameStatsIcon = new ImageView(new Image("file:images/gamestats.png"));
		gameStatsIcon.setFitWidth(40); // Set the fit width of the icon
		gameStatsIcon.setFitHeight(40); // Set the fit height of the icon
		gameStatsIcon.setPreserveRatio(true); // Preserve the ratio
		gameStatsButton.setGraphic(gameStatsIcon); // Set the icon to the button
		gameStatsButton.setStyle("-fx-border-width: 0; -fx-background-radius: 0; -fx-background-color: transparent;");
		gameStatsButton.setMinSize(20, 20); // Set the minimum size of the button
		gameStatsButton.setPrefSize(20, 20); // Set the preferred size of the button
		gameStatsButton.setMaxSize(20, 20); // Set the maximum size of the button

		gameStatsButton.setOnAction(e -> {
			// Action for game statistics button
			stats = gui.getStatsPage();
			User currUser = this.accounts.getCurrUser();
			stats.setStats(currUser.getPlayed(), (int) currUser.getWinRatio(), currUser.getCurrStreak(),
					currUser.getMaxStreak());
			Scene gameScene = new Scene(stats, 600, 600);
			primaryStage.setScene(gameScene);

		});

		// Add buttons to the HBox
		buttonBox.getChildren().addAll(leaderboardButton, gameStatsButton);
		
		buttonBox.getChildren().add(0, newGame);
		// Set the layout on the BorderPan
		borderPane.setCenter(centerStack);
		borderPane.setRight(buttonBox);

		// Set background color of buttonBox to match the image you provided
		buttonBox.setStyle("-fx-background-color: #000000;"); // Set to your preferred color
		centerStack.setStyle("-fx-background-color: #000000;"); // Set to your preferred color
		borderPane.setStyle("-fx-background-color: #000000;"); // Set to your preferred color

		// Assuming 'this' is a BorderPane or other pane that takes up the top of your
		// application window
		this.setTop(borderPane);
	}

	/**
	 * Prompts an alert to the user that they need to play a game first
	 * before clicking the leaderboard.
	 */
	private void emptyLeaderBoard() {
		Alert alert = new Alert(AlertType.INFORMATION);
		alert.setTitle("ERROR!");
		alert.setHeaderText("Play a game first.");
		alert.showAndWait();

	}

	/**
	 * Adds a player to the leader board.
	 */
	private void addLeaderBoard() {
		LeaderBoardPage leaderBoardPage = new LeaderBoardPage(primaryStage, accounts);
		leaderBoardPage.setCurrentGameView(gui.getGameView());
		int i = 1;
		for (Map.Entry<String, User> entry : accounts.getAccounts().entrySet()) {
			String username = entry.getKey();
			User user = entry.getValue();
			if (user.getPlayed() == 0) {
				emptyLeaderBoard();
				return;
			} else {
				leaderBoardPage.updateUserInfo(username, "file:images/pp" + i + ".png",
						((double) user.getWinRatio()));
				i++;

			}
		}
		primaryStage.setScene(new Scene(leaderBoardPage, 500, 500));

	}

	/**
	 * Creates a keyboard graphic by using a VBox and adding 3 HBoxes to represent a
	 * keyboard. Assigns letters from alphabet in numerical order to an array in
	 * Textfields. This is to keep track of the letters for future use. The indexes
	 * within the
	 */
	private void gen_keyboard() {
		String alpha = "0ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		int[] topLetters = { 17, 23, 5, 18, 20, 25, 21, 9, 15, 16 };
		int[] middleLetters = { 1, 19, 4, 6, 7, 8, 10, 11, 12 };
		int[] bottomLetters = { 26, 24, 3, 22, 2, 14, 13 };

		for (int i = 1; i < 27; i++) {
			Button temp = createKey(alpha.substring(i, i + 1));
			letters[i] = temp;
		}

		HBox top = createKeyLine(topLetters);

		HBox middle = createKeyLine(middleLetters);

		HBox bottom = createKeyLine(bottomLetters);
		bottom.setSpacing(5);
		Button backspace = new Button("Del");
		backspace.setPrefHeight(35);
		backspace.setOnAction(e -> {
			deleteKey();
		});
		Button enter = new Button("Enter");
		enter.setPrefHeight(35);
		enter.setOnAction(e -> {
			try {
				enterKey();
			} catch (InterruptedException e1) {
				// TODO Auto-generated catch block
				e1.printStackTrace();
			}
		});
		bottom.getChildren().add(0, backspace);
		bottom.getChildren().add(enter);
		keyboard.setPadding(new Insets(10));
		keyboard.getChildren().addAll(top, middle, bottom);
	}

	/**
	 * Creates an HBox with an array of buttons that contain alphabet letters.
	 * 
	 * @param topletters, an array of integers that represent buttons in the letters
	 *                    array.
	 * @return key_row, an HBox with an array of buttons with letters as the text.
	 */
	private HBox createKeyLine(int[] topletters) {
		HBox key_row = new HBox();
		key_row.setAlignment(Pos.CENTER);
		key_row.setSpacing(5);
		for (int index : topletters) {
			key_row.getChildren().add(letters[index]);
		}
		return key_row;
	}

	/**
	 * Creates a keyboard button with a given string. Used for letters of the
	 * alphabet.
	 * 
	 * @param string, a letter
	 * 
	 * @return button, a customized Button.
	 */
	private Button createKey(String string) {
		Button button = new Button(string);
		button.setPrefHeight(35);
		button.setPrefWidth(35);
		button.setStyle("-fx-color: white;");
		button.setOnAction(e -> {
			guesses[row][col].setText(string);
		});
		return button;
	}

	/**
	 * Creates the textfields for guessing a five letter word.
	 * 
	 * 
	 */
	private void createTextFields() {
		for (int r = 0; r < ROWS; r++) {
			HBox box = new HBox();
			box.setSpacing(5);
			box.setAlignment(Pos.CENTER); // Sets text fields in center.
			for (int c = 0; c < COLS; c++) {
				TextFormatter<String> tf = getTextFormatter();
				TextField temp = new TextField();
				temp.addEventHandler(KeyEvent.KEY_PRESSED, deleteKey);
				temp.addEventHandler(KeyEvent.KEY_PRESSED, enterKey);
				temp.setTextFormatter(tf);
				temp.setEditable(false);
				temp.setPrefHeight(40);
				temp.setPrefWidth(40);
				temp.textProperty().addListener((obs, oldVal, newVal) -> {
					if (newVal.length() > 0 && col != COLS - 1) {
						guesses[row][col].setEditable(false);
						guesses[row][++col].requestFocus();
						guesses[row][col].setEditable(true);
						return;
					}
				});
				guesses[r][c] = temp;
				box.getChildren().add(temp);
			}
			container.getChildren().add(box);
		}
		guesses[row][col].setEditable(true);
		guesses[row][col].requestFocus();
	}

	/**
	 * Checks the input text of a field. 
	 * Restricts any characters other than alphabetic.
	 * 
	 * @return Upper case letter
	 */
	private TextFormatter<String> getTextFormatter() {
		TextFormatter<String> tf = new TextFormatter<>(text -> {
			String newText = text.getControlNewText().toUpperCase();
			if (newText.length() > 1) {
				System.out.println("More than one character");
				text.setText("");
				return text;
			}

			if (newText.length() == 1) {
				char c = newText.charAt(0);
				if (!Character.isAlphabetic(c)) {
					System.out.println("Non-alphabetic character");
					text.setText("");
					return text;
				}
			}
			text.setText(newText);
			text.setRange(0, text.getControlText().length());
			return text;
		});
		return tf;
	}

}
