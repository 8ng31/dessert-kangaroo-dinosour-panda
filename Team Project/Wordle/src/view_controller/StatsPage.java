package view_controller;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;

import javafx.scene.Group;
import javafx.scene.Scene;
import javafx.scene.chart.BarChart;
import javafx.scene.chart.CategoryAxis;
import javafx.scene.chart.NumberAxis;
import javafx.scene.chart.XYChart;
import javafx.scene.chart.XYChart.Data;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.ProgressBar;
import javafx.scene.image.Image;
import javafx.scene.image.ImageView;
import javafx.scene.layout.BorderPane;
import javafx.scene.layout.HBox;
import javafx.scene.layout.VBox;
import javafx.scene.text.Font;
import javafx.scene.text.FontWeight;
import javafx.scene.text.Text;
import javafx.stage.Stage;
import model.AccountCollection;

public class StatsPage extends VBox {
	private Label statsLabel = new Label("STATISTICS");

	private Label played = new Label("0");
	private Label winPercent = new Label("0");
	private Label currStreak = new Label("0");
	private Label maxStreak = new Label("0");
	private HBox stats = new HBox(played, winPercent, currStreak, maxStreak);

	private Label playedLabel = new Label("Played");
	private Label winPercentLabel = new Label("Win %");
	private Label currStreakLabel = new Label("Current\nStreak");
	private Label maxStreakLabel = new Label("Max\nStreak");
	private HBox statsLabels = new HBox(playedLabel, winPercentLabel, currStreakLabel, maxStreakLabel);

	private Label guessDistLabel = new Label("GUESS DISTRIBUTION");

	private Label ones = new Label("1 : 0");
	private Label twos = new Label("2 : 0");
	private Label threes = new Label("3 : 0");
	private Label fours = new Label("4 : 0");
	private Label fives = new Label("5 : 0");
	private Label sixes = new Label("6 : 0");
	private VBox guesses = new VBox(ones, twos, threes, fours, fives, sixes);
	private GameView previousGameView;
	private AccountCollection accounts;

	CategoryAxis yAxis = new CategoryAxis();
	NumberAxis xAxis = new NumberAxis(0, 6, 1);

	private WordleGUI gui;
	private Stage primaryStage;

	// Create the BarChart with inverted axes
	BarChart<Number, String> barChart = new BarChart<>(xAxis, yAxis);

	// Create the data series
	XYChart.Series<Number, String> dataSeries = new XYChart.Series<>();

	public StatsPage(AccountCollection accounts, WordleGUI wordleGUI, Stage primaryStage) {
		this.gui = wordleGUI;
		this.primaryStage = primaryStage;
		this.accounts = accounts;
		setLayout();
		xAxis.setStyle("-fx-tick-label-font-size: 12pt;");
		yAxis.setStyle("-fx-tick-label-font-size: 12pt;");
		xAxis.setLabel("WINS");
		yAxis.setLabel("GUESSES");
	}

	// update the stats of the user
	public void setStats(int played, int winPercent, int currStreak, int maxStreak) {
		barChart.getYAxis().setVisible(false);
		this.played.setText(String.valueOf(played));
		this.winPercent.setText(String.valueOf(winPercent));
		this.currStreak.setText(String.valueOf(currStreak));
		this.maxStreak.setText(String.valueOf(maxStreak));
	}

	// update the guess distribution of the user
	public void setGuessDistribution(ArrayList<Integer> guessDistributions) {
		if (guessDistributions.size() != 6) {
			System.out.println("GUESS DIST ARRAYLIST MUST BE SIZE 6");
		}
		barChart.getYAxis().setVisible(false);
		ArrayList<Label> guesses = new ArrayList<>(Arrays.asList(ones, twos, threes, fours, fives, sixes));

		for (int i = 0; i < 6; i++) {
			guesses.get(i).setText((i + 1) + " : " + guessDistributions.get(i));
			dataSeries.getData().add(new XYChart.Data<>(guessDistributions.get(i), String.valueOf(i + 1)));
		}

	}

	private void displayLabelForData(Data<Number, String> data) {
		Text dataText = new Text(String.valueOf(data.getYValue()));
		dataText.setStyle("-fx-font-size: 10pt;");

		data.getNode().parentProperty().addListener((observable, oldParent, newParent) -> {
			if (newParent != null) {
				((Group) newParent).getChildren().add(dataText);
			}
		});

		data.nodeProperty().addListener((observable, oldNode, newNode) -> {
			if (newNode != null) {
				newNode.boundsInParentProperty().addListener((observableBounds, oldBounds, newBounds) -> {
					dataText.setLayoutX(Math.round(newNode.getBoundsInParent().getMinX()
							+ newNode.getBoundsInParent().getWidth() / 2 - dataText.prefWidth(-1) / 2));
					dataText.setLayoutY(Math.round(newNode.getBoundsInParent().getMinY() - 5));
				});
			}
		});
	}

	// sets the general layout of the page
	private void setLayout() {
		this.setPadding(new Insets(10, 50, 50, 50));
		this.setAlignment(Pos.CENTER);
		this.setSpacing(10);

		font(statsLabel);
		font(played);
		font(winPercent);
		font(currStreak);
		font(maxStreak);
		font(guessDistLabel);

		stats.setSpacing(40);
		stats.setAlignment(Pos.CENTER);
		statsLabels.setSpacing(25);
		statsLabels.setAlignment(Pos.CENTER);
		guesses.setSpacing(2);

		barChart.getData().add(dataSeries);

		// this.getChildren().addAll(statsLabel, stats, statsLabels, guessDistLabel,
		// guesses, barChart);

		this.getChildren().addAll(statsLabel, stats, statsLabels, guessDistLabel, barChart);

		// Create the "Back" button
		Button backButton = new Button();
		ImageView gameBackIcon = new ImageView(new Image("file:images/back.png"));
		gameBackIcon.setFitWidth(40); 
		gameBackIcon.setFitHeight(40); 
		gameBackIcon.setPreserveRatio(true); 
		backButton.setGraphic(gameBackIcon); 
		backButton.setStyle("-fx-border-width: 0; -fx-background-radius: 0; -fx-background-color: transparent;");
		backButton.setMinSize(40, 40); 
		backButton.setPrefSize(40, 50); 
		backButton.setMaxSize(40, 40); 

		backButton.setOnAction(e -> goBack());

		// Create an HBox for the button
		HBox buttonBox = new HBox(backButton);
		buttonBox.setAlignment(Pos.CENTER_LEFT);
		buttonBox.setPadding(new Insets(10, 0, 10, 10));

		this.getChildren().add(0, buttonBox);

	}

	private void goBack() {
	    if (previousGameView != null) {
	        Scene currentScene = primaryStage.getScene();
	        if (currentScene != null) {
	            BorderPane root = new BorderPane(previousGameView);
	            currentScene.setRoot(root);
	        }
	    }
	}

	public void setCurrentGameView(GameView currentGameView) {
		if (currentGameView != null) {
			// Store the reference to the previous GameView
			this.previousGameView = currentGameView;

		
		}
	}

	// helper function
	private void font(Label l) {
		l.setFont(Font.font("Courier New", FontWeight.BOLD, 25));
	}
}
