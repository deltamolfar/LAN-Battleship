#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "qframe.h"
#include "qlabel.h"
#include "QLineEdit"
#include "qpushbutton.h"
#include <QMainWindow>
#include <QDebug>
#include <QMessageBox>
#include <QGridLayout>
#include <QFile>
#include <QSettings>
#include <QCursor>
#include <QPixmap>
#include <QWidget>
#include <QKeyEvent>
#include <QHostInfo>
#include <QtNetwork/QNetworkInterface>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMovie>
#include <QTimer>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QSoundEffect>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

inline QString MESSAGE_START_PREP = "START_PREP";
inline QString MESSAGE_HOST_READY = "HOST_READY";
inline QString MESSAGE_CLIENT_READY = "CLIENT_READY";
inline QString MESSAGE_START_GAME = "START_GAME";
inline QString MESSAGE_CLIENT_WON = "CLIENT_WON";
inline QString MESSAGE_HOST_WON = "HOST_WON";
inline QString MESSAGE_HOST_DISCONNECTED = "HOST_DISCONNECTED";
inline QString MESSAGE_CLIENT_DISCONNECTED = "CLIENT_DISCONNECTED";
inline QString MESSAGE_HOST_HIT = "HIT_HOST";
inline QString MESSAGE_CLIENT_HIT = "CLIENT_HOST";


class MainWindow : public QMainWindow{
    Q_OBJECT

public:
    Ui::MainWindow* getUI() const{
        return ui;
    }
    QSoundEffect Miss, Hit, MainMusic, BattleMusic;
    int MusicLevel=100, FXLevel=100;
    int UserGameMap[10][10] = {{0}}, EnemyGameMap[10][10] = {{0}};// UserGameMap - Map used for gameplay as user's map. EnemyGameMap - Map used for gameplay as enemy's map.
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


public slots:
    void GridButtonPressed(int row, int col);

private slots:

    bool isCheckConnectedToInternet() {// Checks for internet connection (not the most reliable way, best i could've implement quickly)
        QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

        for (const QNetworkInterface& interface : interfaces)
        {
            if (interface.isValid() && interface.flags().testFlag(QNetworkInterface::IsUp) &&
                interface.flags().testFlag(QNetworkInterface::IsRunning) &&
                !interface.flags().testFlag(QNetworkInterface::IsLoopBack))
            {
                QList<QNetworkAddressEntry> addresses = interface.addressEntries();
                for (const QNetworkAddressEntry& address : addresses)
                {
                    if (address.ip().protocol() == QAbstractSocket::IPv4Protocol &&
                        address.ip().toString() != "127.0.0.1")
                    {
                        // Found a connected Wi-Fi interface
                        return true;
                    }
                }
            }
        }

        // No connected Wi-Fi interface found
        return false;
        }

    QString getWlanIpAddress() {
        QString localhostname =  QHostInfo::localHostName();
        QString localhostIP;
        QList<QHostAddress> hostList = QHostInfo::fromName(localhostname).addresses();
        foreach (const QHostAddress& address, hostList) {
            if (address.protocol() == QAbstractSocket::IPv4Protocol && address.isLoopback() == false) {
                localhostIP = address.toString();
            }
        }

        qDebug() << "Localhost name: " << localhostname;
        qDebug() << "IP = " << localhostIP;

        return localhostIP;
    }

    void on_pushButton_menuLanguage_clicked();// When clicked on language button (flag)

    void readSettings();// Reads local settings (Name, Chosen Language, Sound level, etc.)

    void translateAll();// Translates all buttons/labels that exist in translation file

    bool checkShipPlacement(int startRow, int startCol, int shipLength, bool isVertical, int MapToCheck[10][10]);

    QString translate(const QString& source, const QString& targetLanguage){// Looks up translation for given source string, in given language
        static QMap<QString, QMap<QString, QString>> translations;
        // If the translations map is empty, load translations from file.
        if (translations["en"].empty()){
            QFile file("D:\\DeltaMolfar\\Work\\PNU\\CPL_Warships\\res\\en_tr.txt");
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                qWarning() << "Failed to open en_tr.txt file";
                return source;
            }
            QTextStream in(&file);
            while(!in.atEnd()){
                QString line = in.readLine();
                QStringList parts = line.split(":");
                translations["en"][parts[0]] = parts[1];
            }


            if (translations["ua"].empty()){
                QFile file("D:\\DeltaMolfar\\Work\\PNU\\CPL_Warships\\res\\ua_tr.txt");
                if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    qWarning() << "Failed to open ua_tr.txt file";
                    return source;
                }
                QTextStream in(&file);
                while(!in.atEnd()){
                    QString line = in.readLine();
                    QStringList parts = line.split(":");
                    translations["ua"][parts[0]] = parts[1];
                }
            }
        }
        auto it = translations.find(targetLanguage);
        if (it != translations.end()){
            auto submap = it.value();
            if (submap.contains(source)){
                return submap.value(source);
            }
            else{
                qDebug() << "Translation not found for" << source << "in" << targetLanguage;
            }
        }
        else{
            qDebug() << "No translations found for language:" << targetLanguage;
        }

        // If no translation is found, return the source string.
        return source;
    }
    void on_pushButton_menuSingleplayer_clicked();

    void on_pushButton_menuMultiplayer_clicked();

    void on_pushButton_SinglePlayer_Menu_Back_clicked();

    void on_pushButton_SinglePlayer_EasyMode_clicked();// When chosen easy mode in single player

    void on_pushButton_SinglePlayer_MediumMode_clicked();// When chosen medium mode in single player

    void on_pushButton_SinglePlayer_HardMode_clicked();// When chosen hard mode in single player

    void SinglePlayerButtonChange(int Num);

    void AddShip(int startRow, int startCol, int shipLength, bool isVertical, int OriginalArray[10][10]);

    void ResetMaps();

    bool IsPreparationComplete(bool IsSinglePlayer);

    void GetEnemyMap(bool Generate);// Gets enemy map from another user, or generates one if AI.

    void StartGame(bool Versus_AI);// Starts game sequence

    void FinishGame(bool Win);

    void SimulateEnemyHit();

    void Game_Tick(bool ChangeTurn, bool Versus_AI);

    void DifficultyStyleSheetHandler();// Changes apearence of difficulty buttons on click

    void Network_NewConnectionHandler();// Handles new connection to a server (if this client is a server)

    void Network_ClientConnetedToHostHandler();

    void Network_ClientConnectionError(QAbstractSocket::SocketError error);

    void Network_Host_HandleClientData();

    void Network_Client_HandleHostData();

    void Network_Host_SendMessage(QString MessageType, int row=0, int col=0, int DoubleArray[10][10]=0);

    void Network_Client_SendMessage(QString MessageType, int row=0, int col=0, int DoubleArray[10][10]=0);

    void on_pushButton_SinglePlayer_Menu_Play_clicked();

    void on_pushButton_SinglePlayerPrep_Start_clicked();// When clicked on start in singleplayer preparation

    void on_pushButton_SinglePlayerGameplay_Surrender_clicked();// When clicked on surrender button in singleplayer gameplay

    void on_pushButton_SinglePlayerPrep_Carrier_clicked();// When clicked on carrier in singleplayer preparation

    void on_pushButton_SinglePlayerPrep_Cruiser_clicked();// When clicked on cruiser in singleplayer preparation

    void on_pushButton_SinglePlayerPrep_Submarine_clicked();// When clicked on submarine in singleplayer preparation

    void on_pushButton_SinglePlayerPrep_Boat_clicked();// When clicked on boat in singleplayer preparation

    void SetCursorOnPrep();// Changes cursor depending on ship chosen

    void on_pushButton_SinglePlayerPrep_Back_clicked();

    void on_pushButton_MultiPlayerSettings_Back_clicked();

    void on_pushButton_MultiPlayerSettings_QnA_clicked();

    void on_pushButton_SinglePlayerPrep_Rules_clicked();

    void on_pushButton_MultiPlayerSettings_Create_clicked();

    void on_pushButton_MultiPlayerSettings_Connect_clicked();

    void on_pushButton_MultiPlayerSettings_Ready_or_Start_clicked();

    void on_pushButton_MultiPlayerPrep_Start_clicked();

    void on_pushButton_MultiPlayer_Carrier_clicked();

    void on_pushButton_MultiPlayer_Cruiser_clicked();

    void on_pushButton_MultiPlayer_Submarine_clicked();

    void on_pushButton_MultiPlayer_Boat_clicked();

    void on_pushButton_MultiPlayerPrep_Disconnect_clicked();

    void on_pushButton_MultiPlayer_Surrender_clicked();

    void on_pushButton_MultiPlayerPrep_Rules_clicked();

    void on_pushButton_menuSettings_clicked();

    void on_pushButton_menuProfile_clicked();

    void on_pushButton_menuCredits_clicked();

    void on_pushButton_Profile_Save_clicked();

    void on_pushButton_Profile_Back_clicked();

    void on_pushButton_Profile_ChangeName_clicked();

    void on_pushButton_Profile_Icon_clicked();

    void on_pushButton_Settings_Back_clicked();

    void on_horizontalSlider_valueChanged(int value);

    void on_horizontalSlider_2_valueChanged(int value);

private:
    QString UserImagePath, EnemyImagePath, UserName, EnemyName, languageSetting, HoldingType;// languageSettings - holds which language to use. HoldingType - holds which ship player is currently holding.
    bool UnsavedProfile=false, IsClient=true, IsUserTurn=false, IsHolding = false, IsVertical = false;// IsHolding - is user holding any ship. IsVertical - If the ship user holding ship be vertical.
    int UserHealth=20, EnemyHealth=20, DifficultyMode=2, lastHitPos[2] = {-1}, hitDirection = -1; // 0: up, 1: down, 2: left, 3: right; // DifficultyMode: 1 - Easy, 2 - Medium, 3 - Hard
    QTimer AiHitDelay;
    std::vector<std::vector<int>> scores = std::vector<std::vector<int>>(10, std::vector<int>(10, 0));
    QTcpServer* server;
    QTcpSocket *clientSocket;
    QTcpSocket *hostSocket;
    Ui::MainWindow *ui;

protected:
    void keyPressEvent(QKeyEvent *event) override{// Called when any key pressed (used for turning ships in preparation on 'R')
        if(event->key() == Qt::Key_R){
            if(IsVertical){IsVertical=false;}
            else{IsVertical=true;}
            SetCursorOnPrep();
        }
    }
};



class GridWidget : public QFrame{// GridWidget - grid that is used in preparation and in gameplay.
    Q_OBJECT
public:
    GridWidget(QFrame* qFrame, QWidget* parent) : QFrame(qFrame->parentWidget()){
        QGridLayout *gridLayout = new QGridLayout(this);

        gridLayout->setSpacing(5);

        // Creates labels for column markings
        for (int col = 0; col < 10; ++col) {
            QString colMark = QString::number(col + 1);
            QLabel *label = new QLabel(colMark, this);
            label->setAlignment(Qt::AlignCenter);
            label->setStyleSheet("color: rgb(235, 235, 235);\nfont: 700 18pt \"Consolas\";");
            gridLayout->addWidget(label, 0, col + 1);
        }

        // Creates labels for row markings and buttons for grid cells
        for(int row = 0; row<10; ++row){
            QString rowMark = QChar('A' + row);
            QLabel *label = new QLabel(rowMark, this);
            label->setAlignment(Qt::AlignCenter);
            label->setStyleSheet("color: rgb(235, 235, 235);\nfont: 700 18pt \"Consolas\";");
            gridLayout->addWidget(label, row + 1, 0);

            for(int col = 0; col<10; ++col){
                QPushButton *button = new QPushButton(this);
                button->setFixedSize(50, 50);
                button->setStyleSheet(buttonStyleFree);
                gridLayout->addWidget(button, row + 1, col + 1);
                connect(button, &QPushButton::pressed, this, [this, row, col]() {
                    buttonPressed(row, col);
                });
                button->setObjectName(QString("button_%1_%2").arg(row).arg(col));
            }
        }

        setLayout(gridLayout);
    }

    void updateButtonStyles(const int gridArray[10][10]){
        for(int row = 0; row < 10; ++row){
            for(int col = 0; col < 10; ++col){
                QPushButton* button = findChild<QPushButton*>(QString("button_%1_%2").arg(row).arg(col));
                if(button){
                    int value = gridArray[row][col];
                    if(value == 0){
                        button->setStyleSheet(buttonStyleFree);
                    }
                    else if(value == 1){
                        button->setStyleSheet(buttonStyleOccupied);
                    }
                }
            }
        }
    }

    void SetGridDisabled(bool disabled){
        for(int row = 0; row<10; ++row){
            for(int col = 0; col<10; ++col){
                QPushButton* button = findChild<QPushButton*>(QString("button_%1_%2").arg(row).arg(col));
                if(button){
                    button->setDisabled(disabled);
                }
            }
        }
    }

    bool Hit(int row, int col, int MapToHit[10][10]){
        QPushButton* button = findChild<QPushButton*>(QString("button_%1_%2").arg(row).arg(col));
        static QRegularExpression rg("\\s*/\\*\\s*PutImageHere\\s*\\*/");
        if(MapToHit[row][col]==0){
            button->setStyleSheet( button->styleSheet().replace(rg, "/* PutImageHere */   image: url(:/new/prefix1/res/mark_miss.png);") );
            return false;
        }
        else{
            button->setStyleSheet( button->styleSheet().replace(rg, "/* PutImageHere */   image: url(:/new/prefix1/res/mark_hit.png);") );
            return true;
        }
    }

    bool IsHitLegal(int row, int col){
        QPushButton* button = findChild<QPushButton*>(QString("button_%1_%2").arg(row).arg(col));
        if(button->styleSheet().contains("image: url(:/new/prefix1/res/mark_hit.png);")||button->styleSheet().contains("image: url(:/new/prefix1/res/mark_miss.png);")){return false;}
        return true;
    }

private slots:
    void buttonPressed(int row, int col){// Called when button on grid is pressed
        // Traverse the widget hierarchy to access the MainWindow
        QWidget* currentWidget = parentWidget();  // Get the immediate parent widget (QStackedWidget)
        QWidget* mainWindowWidget = nullptr;

        // Traverse up the widget hierarchy until MainWindow is found
        while (currentWidget) {
            if (MainWindow* mainWindow = qobject_cast<MainWindow*>(currentWidget)) {
                mainWindowWidget = mainWindow;
                break;
            }
            currentWidget = currentWidget->parentWidget();
        }

        // Check if the MainWindow instance was found
        if (mainWindowWidget) {
            MainWindow* mainWindow = qobject_cast<MainWindow*>(mainWindowWidget);
            if (mainWindow) {
                mainWindow->GridButtonPressed(row, col);
            }
        }
    }

private:
    QString buttonStyleFree = "\
        QPushButton { /* PutImageHere */\
            background-color: rgb(220,220,220); \
            border: 2px solid black; \
    } \
        QPushButton:hover { \
            border-color: rgb(0,210,210); \
    } \
        QPushButton:pressed { \
            border-color: rgb(0,175,175); \
    }";
    QString buttonStyleOccupied = "\
        QPushButton { /* PutImageHere */\
            background-color: rgb(40,40,40); \
            border: 2px solid black; \
    } \
        QPushButton:hover { \
            border-color: rgb(0,210,210); \
    } \
        QPushButton:pressed { \
            border-color: rgb(0,175,175); \
    }";
};

#endif // MAINWINDOW_H

// https://www.flaticon.com/free-icons/letter-x -> Letter x icons created by Alfredo Hernandez - Flaticon
// https://www.flaticon.com/free-icons/close -> Close icons created by Pixel perfect - Flaticon
