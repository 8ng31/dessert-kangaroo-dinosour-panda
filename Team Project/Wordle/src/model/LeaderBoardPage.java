package model;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Map;
import java.util.Comparator;

import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.control.Label;
import javafx.scene.layout.Background;
import javafx.scene.layout.BackgroundFill;
import javafx.scene.layout.Border;
import javafx.scene.layout.BorderStroke;
import javafx.scene.layout.BorderStrokeStyle;
import javafx.scene.layout.BorderWidths;
import javafx.scene.layout.CornerRadii;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Priority;
import javafx.scene.layout.Region;
import javafx.scene.layout.VBox;
import javafx.scene.paint.Color;
import javafx.scene.text.Font;
import javafx.scene.text.FontWeight;
import view_controller.LeaderBoardUserBlock;

public class LeaderBoardPage extends VBox {

	private Label boardTitle = new Label("Wordle\nLeaderboard");
	private Label rankLabel = new Label("Rank");
	private Label userLabel = new Label("User");
	private Label avgGuessLabel = new Label("W");
	Region spacer = new Region();
	private HBox heading = new HBox(rankLabel, userLabel, spacer, avgGuessLabel);
	private ArrayList<LeaderBoardUserBlock> userBlocks = new ArrayList<>();
	private Background background = new Background(new BackgroundFill(Color.WHITE, CornerRadii.EMPTY, javafx.geometry.Insets.EMPTY));
	private Border border = new Border(new BorderStroke(Color.LIGHTGRAY, BorderStrokeStyle.SOLID, CornerRadii.EMPTY, new BorderWidths(2)));

	private VBox board = new VBox();
	private AccountCollection accounts;

	public LeaderBoardPage(AccountCollection accounts) {
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
		board.getChildren().add(heading);
		board.setBorder(border);
		setUserBlocks();

		this.getChildren().addAll(boardTitle, board);
	}

	// uses an array list of user blocks to fill up the board with dummy 
	private void setUserBlocks() {
		LeaderBoardUserBlock block;
		
//		for(int i = 0; i<accounts.getSize(); i++) {
//			block = new LeaderBoardUserBlock();
//			block.setRank(i + 1);
//			block.setUserName(accounts.);
//			this.userBlocks.add(block);
//			board.setBackground(background); 
//			board.getChildren().add(userBlocks.get(i));
//		}
		
		int i = 0;
		for (Map.Entry<String, User> entry : accounts.getAccounts().entrySet()) {
            String username = entry.getKey();
            User user = entry.getValue();
            
            block = new LeaderBoardUserBlock();
			block.setRank(i + 1);
			block.setUserName(username);
			block.setAVGguess(user.getWinRatio());
//			block.setUserPic("src/model/images/pp2.png"); //Need to fix and figure out
			this.userBlocks.add(block);
			Collections.sort(this.userBlocks, Comparator.comparing(LeaderBoardUserBlock::getAVGguess));
			Collections.reverse(this.userBlocks);
			board.setBackground(background); 
			board.getChildren().add(userBlocks.get(i));
			i++;
            
        }
	
	}
	//updates an individual user block in the rankings
	//needs the rank to change, the profile pic file path, the name, and the avg guess
	public void updateBlock(int rank, String picFilePath, String userName, double avgGuess) {
		LeaderBoardUserBlock block = userBlocks.get(rank - 1);
		block.setUserPic(picFilePath);
		block.setUserName(userName);
		block.setAVGguess(avgGuess);
	}
}
