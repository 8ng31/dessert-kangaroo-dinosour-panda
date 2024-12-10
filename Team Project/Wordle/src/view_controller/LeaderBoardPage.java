package view_controller;

import java.util.ArrayList;
import java.util.Collections;

import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Scene;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.image.Image;
import javafx.scene.image.ImageView;
import javafx.scene.layout.Background;
import javafx.scene.layout.BackgroundFill;
import javafx.scene.layout.Border;
import javafx.scene.layout.BorderPane;
import javafx.scene.layout.BorderStroke;
import javafx.scene.layout.BorderStrokeStyle;
import javafx.scene.layout.BorderWidths;
import javafx.scene.layout.CornerRadii;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Priority;
import javafx.scene.layout.Region;
import javafx.scene.layout.StackPane;
import javafx.scene.layout.VBox;
import javafx.scene.paint.Color;
import javafx.scene.text.Font;
import javafx.scene.text.FontWeight;
import javafx.stage.Stage;
import model.AccountCollection;
import model.User;
import model.UserInfo;

public class LeaderBoardPage extends VBox {

	private Label boardTitle = new Label("Wordle\nLeaderboard");
	private Label rankLabel = new Label("Rank");
	private Label userLabel = new Label("User");
	private Label avgGuessLabel = new Label("Win/Loss Ratio");
	Region spacer = new Region();
	private HBox heading = new HBox(rankLabel, userLabel, spacer, avgGuessLabel);
	private Background background = new Background(new BackgroundFill(Color.WHITE, CornerRadii.EMPTY, javafx.geometry.Insets.EMPTY));
	private Border border = new Border(new BorderStroke(Color.LIGHTGRAY, BorderStrokeStyle.SOLID, CornerRadii.EMPTY, new BorderWidths(2)));

	private VBox board = new VBox();
	private VBox userBoard = new VBox();
	
	private ArrayList<UserInfo> infos = new ArrayList<>();
	private AccountCollection accounts;
	private GameView previousGameView;
	private Stage primaryStage;


	public LeaderBoardPage(Stage primaryStage, AccountCollection accounts) {
		this.primaryStage = primaryStage;
		this.accounts = accounts;
		setLayout();
	}

	// set the layout
	private void setLayout() {
		this.setAlignment(Pos.CENTER);
		this.setSpacing(10);
		boardTitle.setFont(Font.font("Courier New", FontWeight.BOLD, 30));
		boardTitle.setAlignment(Pos.CENTER);
		
		HBox.setHgrow(spacer, Priority.ALWAYS);
		heading.setSpacing(30);
		heading.setPadding(new Insets(0, 20, 0, 20));
		board.setSpacing(10);
		board.setPadding(new Insets(10, 10, 10, 10));
		userBoard.setSpacing(10);
		board.getChildren().addAll(heading, userBoard);
		board.setBorder(border);
		board.setBackground(background);
		
		 // Create the "Back" button
	    Button backButton = new Button();
	    ImageView gameBackIcon = new ImageView(new Image("file:images/back.png"));
		gameBackIcon.setFitWidth(40); // Set the fit width of the icon
		gameBackIcon.setFitHeight(40); // Set the fit height of the icon
		gameBackIcon.setPreserveRatio(true); // Preserve the ratio
		gameBackIcon.setSmooth(true);
		backButton.setGraphic(gameBackIcon); // Set the icon to the button
		backButton.setStyle("-fx-border-width: 0; -fx-background-radius: 0; -fx-background-color: transparent;");
		backButton.setMinSize(40, 40); // Set the minimum size of the button to be square
		backButton.setPrefSize(40, 40); // Set the preferred size of the button to be square
		backButton.setMaxSize(40, 40); // Set the maximum size of the button to be square
	    backButton.setOnAction(e -> goBack()); 


	    HBox buttonBox = new HBox(backButton);
	    buttonBox.setAlignment(Pos.CENTER_LEFT); 
	    buttonBox.setPadding(new Insets(10, 0, 10, 10)); 

	    this.getChildren().add(0, buttonBox); 
		
		this.getChildren().addAll(boardTitle, board);
	}
	
	
	
	//this function either updates the user info or adds it to the list, then it updates the board
	//to show the true user rankings according to avg guess
	public void updateUserInfo(String name, String picPath, double avgGuess) {
		boolean contains = false;
		for (UserInfo user : infos) {
			if (user.getName().equals(name)) {
				contains = true;
				//we found user, so we can update
				user.setAVGguess(avgGuess);
			}
		}
		//if not found user then we create new entry
		if (!contains) {
			infos.add(new UserInfo(name, picPath, avgGuess));
		}
		//finally, sort list and update userBoard
		Collections.sort(infos,Collections.reverseOrder());
		System.out.println(infos);
		updateUserBoard();
	}
	
	private void updateUserBoard() {
		//clear the current userBoard
		userBoard.getChildren().clear();
		
		LeaderBoardUserBlock block;
		UserInfo user;
		//iterate through the top 5 users in the user info list and add them to userBoard
		for (int i = 0; i < 5; i++){
			//stop if less than 5 users atm
			if ((i + 1) > infos.size()) {
				return;
			}
			user = infos.get(i);
			block = new LeaderBoardUserBlock(i + 1, user.getPath(), user.getName(), user.getAVGguess());
			this.userBoard.getChildren().add(block);
		}
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
	
}
