/**
 * Author: Angel Benavides
 * Prompts user with difficulty for game.
 */
package view_controller;


import java.io.IOException;

import javafx.geometry.Insets;
import javafx.scene.Scene;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.layout.HBox;
import javafx.scene.layout.VBox;
import javafx.stage.Stage;

/**
 * A page used to select difficulty.
 */
public class DifficultyView extends VBox {

	private HBox top;
	private HBox middle; 
	private HBox bottom;
	private Button normal;
	private Button hard;
	private Button extreme;
	
	private WordleGUI gui;
	private Stage primaryStage;
	
	/**
	 * Saves parameters given for use in different scenes
	 * 
	 * @param accounts
	 * @param wordleGUI
	 * @param primaryStage
	 * @throws IOException
	 */
	public DifficultyView(WordleGUI wordleGUI, Stage primaryStage) throws IOException {
		gui = wordleGUI;
		this.primaryStage = primaryStage;
		initialize();
	}
	
	/**
	 * Initializes variables for nodes. Adds children to this
	 */
	private void initialize() {
		this.setStyle("-fx-background-color: #121212;");
		this.setPadding(new Insets(50));
		this.setSpacing(40);
		top = new HBox();
		middle = new HBox();
		bottom = new HBox();
		normal = new Button("Normal");
		hard = new Button("Hard");
		extreme = new Button("Extreme");
		setButton(top, normal, "normal", "green", 5);
		setButton(middle, hard, "hard", "orange", 7);
		setButton(bottom, extreme, "extreme", "red", 10);
		this.getChildren().addAll(top, middle, bottom);
	}
	
	/**
	 * Constructs a button & label to add in an Hbox. Uses parameters to set attributes.
	 * 
	 * @param row, HBox object to add a button and label
	 * @param button, difficulty button
	 * @param difficulty, String to set the button too.
	 * @param color, used to set background color of button.
	 * @param size, used in prompt for label.
	 */
	private void setButton(HBox row, Button button, String difficulty, String color, int size) {
		button.setStyle("-fx-background-color: "+ color + ";");
		button.setOnAction(e -> {
			GameView gameView = gui.getGameView();
			try {
				gameView.changeDifficulty(difficulty, size);
			} catch (IOException e1) {
				e1.printStackTrace();
			}
			Scene gameScene = new Scene(gameView, 500, 600);
			primaryStage.setScene(gameScene);
			System.out.println("Helllo testing");
		});
		Label prompt = new Label(prompt(difficulty, size));
		prompt.setStyle("-fx-text-fill: white;");
		row.getChildren().addAll(button, prompt);
	}
	
	/**
	 * Creates a prompt using given variables.
	 * 
	 * @param difficulty, difficulty of game
	 * @param size, number of letters in a word.
	 * @return prompt.
	 */
	private String prompt(String difficulty, int size) {
		return "  Choose "+ difficulty +" to guess a " + size + " letter word.";
	}

}
