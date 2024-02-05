module com.example.ncdu_os_gui {
    requires javafx.controls;
    requires javafx.fxml;


    opens com.example.ncdu_os_gui to javafx.fxml;
    exports com.example.ncdu_os_gui;
}