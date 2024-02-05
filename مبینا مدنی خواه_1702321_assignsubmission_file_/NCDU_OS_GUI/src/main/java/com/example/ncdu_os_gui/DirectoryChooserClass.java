package com.example.ncdu_os_gui;

import javafx.application.Application;
import javafx.event.ActionEvent;
import javafx.event.EventHandler;
import javafx.geometry.HPos;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Scene;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.layout.*;
import javafx.scene.text.Font;
import javafx.scene.text.FontPosture;
import javafx.scene.text.FontWeight;
import javafx.stage.DirectoryChooser;
import javafx.stage.Stage;
import java.io.*;
import java.util.ArrayList;
import java.util.Objects;

public class DirectoryChooserClass extends Application {

    String path;
    Font font = Font.font("Courier New", FontWeight.BOLD,15);



    public static void main(String args[]) {
        launch(args);
    }

    public void start(Stage stage) throws IOException {



        try {

            stage.setTitle("NCDU");
            stage.setResizable(false);

            DirectoryChooser dir_chooser = new DirectoryChooser();

            Label label = new Label("Please select a file directory");
            label.setFont(font);

            Button button  = new Button("Select");

            Button next = new Button("Next");
            next.setDisable(true);

            next.setOnAction(new EventHandler<ActionEvent>() {
                @Override
                public void handle(ActionEvent event) {
                    ProcessBuilder pb = new ProcessBuilder(
                            "/Users/mbina/CLionProjects/OS_NCDU/main", path);
                    Process process = null;
                    try {
                        process = pb.start();
                        process.waitFor();
                        startSecondPage(stage);
                    } catch (IOException | InterruptedException e) {
                        throw new RuntimeException(e);
                    }
                }
            });

            EventHandler<ActionEvent> event =
                    new EventHandler<ActionEvent>() {
                        public void handle(ActionEvent e) {
                            File file = dir_chooser.showDialog(stage);

                            if (file != null) {
                                label.setText("-> "+file.getAbsolutePath() + " selected");
                                path = file.getAbsolutePath() + "/";
                                button.setText("re-Select");
                                next.setDisable(false);
                                next.setDefaultButton(true);
                            }
                        }
                    };

            button.setOnAction(event);

            HBox hbox = new HBox(30,button,next);
            hbox.setAlignment(Pos.CENTER);
            VBox vbox = new VBox(30, label, hbox);

            vbox.setAlignment(Pos.CENTER);

            Scene scene = new Scene(vbox, 860, 500);
            scene.getStylesheets().add(Objects.requireNonNull(getClass().getResource("application.css")).toExternalForm());


            stage.setScene(scene);

            stage.show();
        } catch (Exception e) {

            System.out.println(e.getMessage());
        }
    }

    public void startSecondPage(Stage stage) throws IOException {
        Scene scene2;
        //----------------------------------------------------------------------------------
        // Reading the file
        ArrayList<String> data = new ArrayList<>();
        File file = new File("/Users/mbina/CLionProjects/OS_NCDU/filename.txt");
        BufferedReader br = new BufferedReader(new FileReader(file));
        String st;

        while ((st = br.readLine()) != null)
            data.add(st);

        //-----------------------------------------------------------------------------------
        // PAGE 2
        AnchorPane root = new AnchorPane();

        BorderPane borderPane = new BorderPane();
        borderPane.setPrefHeight(600.0);
        borderPane.setPrefWidth(800.0);
        AnchorPane.setLeftAnchor(borderPane, 30.0);
        AnchorPane.setTopAnchor(borderPane, 30.0);

        HBox topHBox = new HBox();
        topHBox.setPrefHeight(76.0);
        topHBox.setPrefWidth(600.0);
        topHBox.setSpacing(30.0);
        BorderPane.setAlignment(topHBox, javafx.geometry.Pos.CENTER);
        borderPane.setTop(topHBox);

        HBox bottomHBox = new HBox();
        bottomHBox.setPrefHeight(100.0);
        bottomHBox.setPrefWidth(200.0);
        BorderPane.setAlignment(bottomHBox, javafx.geometry.Pos.CENTER);

        // Set bottomHBox as bottom of BorderPane
        borderPane.setBottom(bottomHBox);

        // Create center GridPane
        GridPane gridPane = new GridPane();
        gridPane.setPrefHeight(100);
        gridPane.setPrefWidth(530);
        gridPane.setGridLinesVisible(true);


        BorderPane.setAlignment(gridPane, javafx.geometry.Pos.CENTER);

        // Add column constraints
        ColumnConstraints col1 = new ColumnConstraints();
        col1.setHgrow(Priority.SOMETIMES);
        col1.setMinWidth(10.0);
        col1.setMaxWidth(250);
        col1.setPrefWidth(100.0);
        col1.setHalignment(HPos.CENTER);

        ColumnConstraints col2 = new ColumnConstraints();
        col2.setHgrow(Priority.SOMETIMES);
        col2.setMinWidth(10.0);
        col2.setPrefWidth(200);
        col2.setHalignment(HPos.CENTER);


        gridPane.getColumnConstraints().addAll(col1, col2);
        for (int i = 0; i < 7; i++) {
            RowConstraints row = new RowConstraints();
            row.setMinHeight(10.0);
            row.setPrefHeight(30.0);
            row.setVgrow(Priority.SOMETIMES);
            gridPane.getRowConstraints().add(row);
        }

        // Create Labels for GridPane
        Label numOfFilesLabel = new Label("Num of directory files");
        numOfFilesLabel.setFont(font);
        Label numOfFilesValueLabel = new Label(data.get(0));
        Label numOfFileTypesLabel = new Label("Number of file types");
        numOfFileTypesLabel.setFont(font);
        Label numOfFileTypesValueLabel = new Label(data.get(6));
        numOfFileTypesValueLabel.setWrapText(true);
        Label largestPathLabel = new Label("The largest file path");
        largestPathLabel.setFont(font);
        Label largestPathValueLabel = new Label(data.get(1));
        largestPathValueLabel.setWrapText(true);
        Label smallestPathLabel = new Label("The smallest file path");
        smallestPathLabel.setFont(font);
        Label smallestPathValueLabel = new Label(data.get(3));
        smallestPathValueLabel.setWrapText(true);
        Label rootSizeLabel = new Label("Root size");
        rootSizeLabel.setFont(font);
        Label rootSizeValueLabel = new Label(data.get(5));
        Label largestSizeLabel = new Label("The largest file size");
        largestSizeLabel.setFont(font);
        Label largestSizeValueLabel = new Label(data.get(2));
        Label smallestSizeLabel = new Label("The smallest file size");
        smallestSizeLabel.setFont(font);

        Label smallestSizeValueLabel = new Label(data.get(4));

        // Add Labels to GridPane
        gridPane.add(numOfFilesLabel, 0, 0);
        gridPane.add(numOfFilesValueLabel, 1, 0);
        gridPane.add(numOfFileTypesLabel, 0, 1);
        gridPane.add(numOfFileTypesValueLabel, 1, 1);
        gridPane.add(largestPathLabel, 0, 2);
        gridPane.add(largestPathValueLabel, 1, 2);
        gridPane.add(smallestPathLabel, 0, 4);
        gridPane.add(smallestPathValueLabel, 1, 4);
        gridPane.add(rootSizeLabel, 0, 6);
        gridPane.add(rootSizeValueLabel, 1, 6);
        gridPane.add(largestSizeLabel, 0, 3);
        gridPane.add(largestSizeValueLabel, 1, 3);
        gridPane.add(smallestSizeLabel, 0, 5);
        gridPane.add(smallestSizeValueLabel, 1, 5);

        // Set gridPane as center of BorderPane
        borderPane.setCenter(gridPane);

        // Set borderPane as the child of the root AnchorPane
        root.getChildren().add(borderPane);


        //layout 2
        StackPane layout2 = new StackPane();

        layout2.getChildren().add(root);
        scene2 = new Scene(layout2, 860, 500);
        scene2.getStylesheets().add(Objects.requireNonNull(getClass().getResource("application.css")).toExternalForm());

        stage.setScene(scene2);
        stage.setResizable(false);
    }
}
