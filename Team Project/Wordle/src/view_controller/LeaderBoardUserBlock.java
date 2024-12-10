package view_controller;

import javafx.geometry.Insets;
import javafx.scene.control.Label;
import javafx.scene.effect.DropShadow;
import javafx.scene.image.Image;
import javafx.scene.image.ImageView;
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
import javafx.scene.paint.Color;
import javafx.scene.text.Font;
import javafx.scene.text.FontWeight;

public class LeaderBoardUserBlock extends HBox {
	// fields
	private Image pic = new Image("file:images/x.png", false);
	private ImageView userPic = new ImageView(pic);
	private Label userName = new Label("blank");
	private Label avgGuess = new Label("0");
	private Label rank = new Label("1");
	private Background background = new Background(
			new BackgroundFill(Color.WHITE, CornerRadii.EMPTY, javafx.geometry.Insets.EMPTY));

	// Create spacer nodes to maintain consistent spacing
	Region spacer = new Region();

	// default constructor
	public LeaderBoardUserBlock() {
		setLayout();
		HBox.setHgrow(spacer, Priority.ALWAYS); // Allow spacer1 to grow and maintain spacing

		this.getChildren().addAll(this.rank, this.userPic, this.userName, spacer, this.avgGuess);
	}

	// constructor with all fields
	public LeaderBoardUserBlock(int rank, String picLink, String userName, double avgGuess) {
		this.rank.setText(String.valueOf(rank));
		this.pic = new Image(picLink, false);
		this.userPic = new ImageView(pic);
		this.userName.setText(userName);
		this.avgGuess.setText(String.valueOf(avgGuess) + "%");

		setLayout();
		HBox.setHgrow(spacer, Priority.ALWAYS); // Allow spacer1 to grow and maintain spacing

		this.getChildren().addAll(this.rank, this.userPic, this.userName, spacer, this.avgGuess);
	}

	// set the general layout
	private void setLayout() {
		this.rank.setFont(Font.font("Courier New", FontWeight.BOLD, 20));
		this.userName.setFont(Font.font("Courier New", FontWeight.BOLD, 20));
		this.avgGuess.setFont(Font.font("Courier New", FontWeight.BOLD, 20));
		this.avgGuess.setTextFill(Color.GREEN);

		// Create a DropShadow effect
		DropShadow dropShadow = new DropShadow();
		dropShadow.setColor(Color.LIGHTGRAY); // Set the shadow color
		dropShadow.setRadius(10); // Set the shadow radius
		dropShadow.setOffsetY(3);
		this.setBackground(background);
		// Apply the DropShadow effect to the HBox
		this.setEffect(dropShadow);

		// this.setBorder(border);
		this.setPadding(new Insets(15, 40, 15, 30));
		this.setSpacing(30);

	}

	// setters
	public void setRank(int rank) {
		this.rank.setText(String.valueOf(rank));
	}

	public void setUserPic(String picLink) {
		this.pic = new Image(picLink, false);
		this.userPic.setImage(pic);
	}

	public void setUserName(String userName) {
		this.userName.setText(userName);
	}

	public void setAVGguess(double avgGuess) {
		this.avgGuess.setText(String.valueOf(avgGuess));
	}

	// getters
	public int getRank() {
		return Integer.valueOf(rank.getText());
	}

	public String getName() {
		return String.valueOf(userName.getText());
	}

	public double getAVGguess() {
		return Double.valueOf(avgGuess.getText());
	}

}
