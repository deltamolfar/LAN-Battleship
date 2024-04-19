#include "mainwindow.h"
#include <QApplication>

// made by deltamolfar (Viter Zakhar Andriyovich) - 2023

int main(int argc, char *argv[]){
    QApplication a(argc, argv);
    a.setApplicationName("Battleship");
    a.setWindowIcon(QIcon(":/new/prefix1/res/icon.png"));
    MainWindow w;
    w.show();
    return a.exec();
}
