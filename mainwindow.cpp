#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <random>

// Is for storing GridWidget instances. Not the best way, but durind development I understood that i need to pretty often use them after i initialize them instead of QFrames in certain
// areas. So that's the fastest way to implement it without breaking anything. If you're reading this on github - find a better way to implement this
// 0 == Single Player preparation, 1 == Single Player Gameplay User Map, 2 == Single Player Gameplay Computer Map
// 3 == Multi Player preparation, 4 == Multi Player Gameplay User Map, 5 == Multi Player Gameplay Enemy Map
std::array<GridWidget*, 6> gridWidgets;

MainWindow::MainWindow(QWidget *parent):QMainWindow(parent),ui(new Ui::MainWindow){
    ui->setupUi(this);
    readSettings();// Find preffered language, user's name and icon
    translateAll();// Translate every button there is (except GridWidget buttons)
    ui->stackedWidget->setCurrentIndex(0);// Set stacked widget to main menu
    this->setWindowTitle("Battleship by deltamolfar");

    QMovie *MultiPlayerLoadingMovie = new QMovie(":/new/prefix1/res/LoadingRadar.gif");
    ui->label_MultiPlayerSettings_Anim->setMovie(MultiPlayerLoadingMovie);
    MultiPlayerLoadingMovie->start();

    // Initializing gridWidget objects, and storing them to global array for easy access.
    gridWidgets[0] = new GridWidget(ui->frame_SinglePlayerPrep, this);
    ui->verticalLayout_12->replaceWidget(ui->frame_SinglePlayerPrep, gridWidgets[0]);
    ui->verticalLayout_12->layout()->setAlignment(gridWidgets[0], Qt::AlignCenter);
    delete ui->frame_SinglePlayerPrep;

    gridWidgets[1] = new GridWidget(ui->frame_SinglePlayerGameplay_UserFrame, this);
    ui->page_SinglePlayerGameplay->layout()->replaceWidget(ui->frame_SinglePlayerGameplay_UserFrame, gridWidgets[1]);
    ui->page_SinglePlayerGameplay->layout()->setAlignment(gridWidgets[1], Qt::AlignCenter);
    delete ui->frame_SinglePlayerGameplay_UserFrame;

    gridWidgets[2] = new GridWidget(ui->frame_SinglePlayerGameplay_ComputerFrame, this);
    ui->page_SinglePlayerGameplay->layout()->replaceWidget(ui->frame_SinglePlayerGameplay_ComputerFrame, gridWidgets[2]);
    ui->page_SinglePlayerGameplay->layout()->setAlignment(gridWidgets[2], Qt::AlignCenter);
    delete ui->frame_SinglePlayerGameplay_ComputerFrame;



    gridWidgets[3] = new GridWidget(ui->frame_MultiPlayer_Prep, this);
    ui->verticalLayout_22->replaceWidget(ui->frame_MultiPlayer_Prep, gridWidgets[3]);
    ui->verticalLayout_22->layout()->setAlignment(gridWidgets[3], Qt::AlignCenter);
    delete ui->frame_MultiPlayer_Prep;

    gridWidgets[4] = new GridWidget(ui->frame_MultiPlayer_UserFrame, this);
    ui->page_MultiPlayerGameplay->layout()->replaceWidget(ui->frame_MultiPlayer_UserFrame, gridWidgets[4]);
    ui->page_MultiPlayerGameplay->layout()->setAlignment(gridWidgets[4], Qt::AlignCenter);
    delete ui->frame_MultiPlayer_UserFrame;

    gridWidgets[5] = new GridWidget(ui->frame_MultiPlayer_EnemyFrame, this);
    ui->page_MultiPlayerGameplay->layout()->replaceWidget(ui->frame_MultiPlayer_EnemyFrame, gridWidgets[5]);
    ui->page_MultiPlayerGameplay->layout()->setAlignment(gridWidgets[5], Qt::AlignCenter);
    delete ui->frame_MultiPlayer_EnemyFrame;

    ui->horizontalSlider->setValue(MusicLevel);
    ui->horizontalSlider_2->setValue(FXLevel);

    Miss.setSource(QUrl::fromLocalFile(":/new/prefix1/res/mixkit-fish-flapping-2457.wav"));
    Hit.setSource(QUrl::fromLocalFile(":/new/prefix1/res/mixkit-sea-mine-explosion-1184.wav"));
    Miss.setLoopCount(1);
    Hit.setLoopCount(1);
    Miss.setVolume(static_cast<float>(FXLevel)/100.0f);
    Hit.setVolume(static_cast<float>(FXLevel)/100.0f);

    MainMusic.setSource(QUrl::fromLocalFile(":/new/prefix1/res/tense-detective-looped-drone-10054.wav"));
    BattleMusic.setSource(QUrl::fromLocalFile(":/new/prefix1/res/phantom-116107.wav"));
    MainMusic.setLoopCount(QSoundEffect::Infinite);
    BattleMusic.setLoopCount(QSoundEffect::Infinite);
    MainMusic.setVolume(static_cast<float>(MusicLevel)/100.0f);
    BattleMusic.setVolume(static_cast<float>(MusicLevel)/100.0f);

    MainMusic.play();

    connect(&AiHitDelay, &QTimer::timeout, this, &MainWindow::SimulateEnemyHit);
    AiHitDelay.setSingleShot(true);

    clientSocket = new QTcpSocket(this);
}
MainWindow::~MainWindow(){// Destructor. Sends disconnect signals to another player, if connection is established.
    if(IsClient&clientSocket->isReadable()){Network_Client_SendMessage(MESSAGE_CLIENT_DISCONNECTED); clientSocket->close();}
    else{Network_Host_SendMessage(MESSAGE_HOST_DISCONNECTED);}
    delete ui;
}



//-----=====[+MULTI-PAGE FUNCTIONS+]=====-----

void MainWindow::SetCursorOnPrep(){// Called on ship button click, to represent ship to be placed on grid
    if(!IsHolding){unsetCursor(); return;}
    int cursorSizeNoffset;// For making tip of ship real cursor
    QPixmap cursorPixmap;
    if(HoldingType=="Carrier"){
        cursorSizeNoffset = 200;
        cursorPixmap = QPixmap(":/new/prefix1/res/carrier_top.jpg");
    }
    else if(HoldingType=="Cruiser"){
        cursorSizeNoffset = 150;
        cursorPixmap = QPixmap(":/new/prefix1/res/cruiser_top.jpg");
    }
    else if(HoldingType=="Submarine"){
        cursorSizeNoffset = 100;
        cursorPixmap = QPixmap(":/new/prefix1/res/submarine_top.jpg");
    }
    else{
        cursorSizeNoffset = 80;
        cursorPixmap = QPixmap(":/new/prefix1/res/boat_top.jpg");
    }

    cursorPixmap = cursorPixmap.scaled(cursorSizeNoffset, cursorSizeNoffset, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if(IsVertical){cursorSizeNoffset=40; cursorPixmap = cursorPixmap.transformed(QTransform().rotate(-90));}
    if(HoldingType=="Boat"){cursorSizeNoffset=60;}
    QCursor cursor(cursorPixmap, cursorSizeNoffset-20, 0);
    setCursor(cursor);
}

void MainWindow::readSettings(){// Makes saving language prefferences, icon and name - possible.
    QSettings settings("DeltaMolfarApps", "Battleships");
    if(settings.contains("language")){
        languageSetting = settings.value("language").toString();
    }
    else{
        languageSetting = "en";
        settings.setValue("language", "en");
    }

    if(settings.contains("UserName")){
        UserName = settings.value("UserName").toString();
        if(UserName.isEmpty()){// User name is empty, drop to default.
            UserName = "Captain";
            settings.setValue("UserName", "Captain");
        }
    }
    else{
        UserName = "Captain";
        settings.setValue("UserName", "Captain");
    }

    if(settings.contains("UserImagePath")){
        UserImagePath = settings.value("UserImagePath").toString();
        QFileInfo fileInfo(UserImagePath);
        if(!fileInfo.exists()&&!fileInfo.isFile()){// Image doesn't exist, drop to default one.
            UserImagePath = ":/new/prefix1/res/user_icon.png";
            settings.setValue("UserImagePath", ":/new/prefix1/res/user_icon.png");
        }
    }
    else{
        UserImagePath = ":/new/prefix1/res/user_icon.png";
        settings.setValue("UserImagePath", ":/new/prefix1/res/user_icon.png");
    }

    if(settings.contains("MusicLevel")){
        MusicLevel = settings.value("MusicLevel").toInt();
    }
    else{
        MusicLevel = 100;
        settings.setValue("MusicLevel", "100");
    }

    if(settings.contains("FXLevel")){
        FXLevel = settings.value("FXLevel").toInt();
    }
    else{
        FXLevel = 100;
        settings.setValue("FXLevel", "100");
    }


    if(languageSetting=="en"){
            ui->pushButton_menuLanguage->setStyleSheet("image: url(:/new/prefix1/res/united-kingdom.png);\nbackground-color: rgba(255, 255, 255, 0);\nborder-color: rgba(255, 255, 255, 0);");
        }
    else{
            ui->pushButton_menuLanguage->setStyleSheet("image: url(:/new/prefix1/res/ukraine.png);\nbackground-color: rgba(255, 255, 255, 0);\nborder-color: rgba(255, 255, 255, 0);");
    }
}
void MainWindow::translateAll(){// Invoked once on start, and then every time language is changed. Finds all children and calls translate() function for them.
    // Find all buttons in the main window
    QList<QPushButton*> buttons = findChildren<QPushButton*>();
    // Loop through all the buttons and translate them
    for (auto button : buttons) {
            // Skip the button with object name "pushButton_language"
            if (button->objectName() == "pushButton_menuLanguage") {
                continue;
            }

            // Translate the button text using the translate function
            QString buttonText = button->text();
            QString translatedText = translate(buttonText, languageSetting);
            button->setText(translatedText);
    }
}

bool isValidPosition(int row, int col){
    return (row >= 0 && row < 10 && col >= 0 && col < 10);
}
void MainWindow::AddShip(int startRow, int startCol, int shipLength, bool isVertical, int OriginalArray[10][10]){// Adds a ship to given vector
    if(isVertical){
        for (int row = startRow; row < startRow + shipLength; ++row){
            OriginalArray[row][startCol] = 1;
        }
    }
    else{
        for (int col = startCol; col > startCol - shipLength; --col){
            OriginalArray[startRow][col] = 1;
        }
    }
}
void MainWindow::ResetMaps(){// Resets both maps to default
    for (int i = 0; i < 10; ++i) {
        for (int j = 0; j < 10; ++j) {
            UserGameMap[i][j] = 0;
            EnemyGameMap[i][j] = 0;
        }
    }
}
bool MainWindow::IsPreparationComplete(bool IsSinglePlayer){// Returns true, if all ships are placed (if all labels on singleplayer/multiplayer preparation page == "x0")
    if(IsSinglePlayer){
        if(ui->label_SinglePlayerPrep_Carrier->text()=="x0"&&ui->label_SinglePlayerPrep_Cruiser->text()=="x0"&&ui->label_SinglePlayerPrep_Submarine->text()=="x0"&&ui->label_SinglePlayerPrep_Boat->text()=="x0"){
            return true;
        }
        return false;
    }
    else{
        if(ui->label_MultiPlayer_Carrier->text()=="x0"&&ui->label_MultiPlayer_Cruiser->text()=="x0"&&ui->label_MultiPlayer_Submarine->text()=="x0"&&ui->label_MultiPlayer_Boat->text()=="x0"){
            return true;
        }
        return false;
    }
}
void MainWindow::GetEnemyMap(bool Generate){// Fills enemy map (or generates one)
    memset(EnemyGameMap, 0, sizeof(EnemyGameMap));// Clears anything that was there to zero. Just additional fail proof.
    if(Generate){
        // Ship sizes
        QList<int> shipSizes;
        shipSizes << 4 << 3 << 3 << 2 << 2 << 2 << 1 << 1 << 1 << 1;

        // Shuffle the ship sizes randomly
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(shipSizes.begin(), shipSizes.end(), gen);

        // Place ships on the grid
        for(int size : shipSizes){
            bool placed = false;
            while (!placed) {
                int row = rand() % 10;
                int col = rand() % 10;
                std::uniform_int_distribution<int> dis(0, 1);
                bool isVerticalVar = dis(gen) == 0;

                if (checkShipPlacement(row, col, size, isVerticalVar, EnemyGameMap)) {
                    for(int i=0; i<size; i++){
                        if(isVerticalVar){
                            EnemyGameMap[row + i][col] = 1;
                        }
                        else{
                            EnemyGameMap[row][col-i] = 1;
                        }
                    }
                    placed = true;
                }
            }
        }
    }
}

void MainWindow::StartGame(bool Versus_AI){// Starts a game sequence (timer, if sent/turns/etc.)
    MainMusic.stop();
    BattleMusic.play();
    UserHealth=20;
    EnemyHealth=20;

    if(Versus_AI){
        IsUserTurn=true;
        gridWidgets[1]->SetGridDisabled(true);

        ui->progressBar->setValue(UserHealth*5);
        ui->progressBar_2->setValue(EnemyHealth*5);

    }
    else{
        if(IsClient){IsUserTurn=false;}
        else{IsUserTurn=true;}
        gridWidgets[4]->SetGridDisabled(true);

        ui->progressBar->setValue(UserHealth*5);
        ui->progressBar_2->setValue(EnemyHealth*5);
    }
}
void MainWindow::FinishGame(bool Win){// Finishes game
    AiHitDelay.stop();
    MainMusic.play();
    BattleMusic.stop();
    if(Win){
        if(languageSetting=="ua"){QMessageBox::information(this, "ПЕРЕМОГА!", "Ви перемогли!");}
        else{QMessageBox::information(this, "WIN!", "You've won!");}
    }
    else{
        if(languageSetting=="ua"){QMessageBox::information(this, "ПОРАЗКА", "Ви програли");}
        else{QMessageBox::information(this, "LOSE", "You've lost");}
    }

    ui->stackedWidget->setCurrentIndex(0);
}

void MainWindow::SimulateEnemyHit(){// Makes AI shoots at user's map
    int row;
    int col;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> distribution(0, 9);

    switch (DifficultyMode){
    case 1:{
        do{
            row = distribution(gen);
            col = distribution(gen);
        }while(!gridWidgets[1]->IsHitLegal(row, col));

        if(gridWidgets[1]->Hit(row, col, UserGameMap)){Hit.play(); UserHealth--; Game_Tick(false, true);}
        else{Miss.play(); Game_Tick(true, true);}
    }
        break;


    case 2:{// Doesn't work as intended, but does give an increased level of difficulty. Decided to left as is
        if(lastHitPos[0] != -1 && lastHitPos[1] != -1){
            row = lastHitPos[0];
            col = lastHitPos[1];

            // Determine the direction of the hit based on hitDirection
            switch (hitDirection){
            case 0:
                row--;
                break;
            case 1:
                row++;
                break;
            case 2:
                col--;
                break;
            case 3:
                col++;
                break;
            default:
                // Invalid hitDirection, generate a random hit
                row = distribution(gen);
                col = distribution(gen);
                break;
            }

            if(!gridWidgets[1]->IsHitLegal(row, col)){
                row = distribution(gen);
                col = distribution(gen);
            }
        }
        else{
            row = distribution(gen);
            col = distribution(gen);
        }

        if(gridWidgets[1]->Hit(row, col, UserGameMap)){
            UserHealth--;
            Hit.play();
            Game_Tick(false, true);
            lastHitPos[0] = row;
            lastHitPos[1] = col;
            hitDirection = -1;
        }
        else{
            Miss.play();
            Game_Tick(true, true);
        }
    }
        break;

    case 3:{// Biasing targets (Cheating difficulty. AI know where your ships at. Uses random fixed chance of missing/hitting).
        std::vector<std::pair<int, int>> targets;
        int maxScore = 0;

        for (int row = 0; row < 10; ++row) {
            for (int col = 0; col < 10; ++col) {
                // Skip if the cell is already hit
                if (!gridWidgets[1]->IsHitLegal(row, col)) {
                    continue;
                }

                int score = 0;

                // Calculate the score based on target bias
                if (UserGameMap[row][col] != 0) {
                    score += 10; // Ship present in the cell
                }

                // Check adjacent cells for potential ships
                if (row > 0 && UserGameMap[row - 1][col] != 0) {
                    score += 3; // Potential ship above
                }
                if (row < 9 && UserGameMap[row + 1][col] != 0) {
                    score += 3; // Potential ship below
                }
                if (col > 0 && UserGameMap[row][col - 1] != 0) {
                    score += 3; // Potential ship to the left
                }
                if (col < 9 && UserGameMap[row][col + 1] != 0) {
                    score += 3; // Potential ship to the right
                }

                // Update the maximum score and store the target
                if (score >= maxScore) {
                    if (score > maxScore) {
                        targets.clear();
                    }
                    targets.emplace_back(row, col);
                    maxScore = score;
                }
            }
        }

        // Select a random target from the highest scoring targets
        if(!targets.empty()){
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int> dist(0, targets.size() - 1);
            int index = dist(gen);
            int row = targets[index].first;
            int col = targets[index].second;

            // Introduce a chance of missing the target
            std::uniform_real_distribution<double> missDist(0.0, 1.0);
            double missChance = 0.65; // Adjust the miss chance as desired
            if(missDist(gen) <= missChance){
                // Missed the target
                while (gridWidgets[1]->IsHitLegal(row, col) && UserGameMap[row][col] != 0){
                    // Randomly shift the coordinates until it is a miss
                    row = (row + distribution(gen)) % 10;
                    col = (col + distribution(gen)) % 10;
                }
                gridWidgets[1]->Hit(row, col, UserGameMap);
                Miss.play();
                Game_Tick(true, true);
            }
            else{
                // Perform the hit
                if(gridWidgets[1]->Hit(row, col, UserGameMap)){
                    Hit.play();
                    UserHealth--;
                    Game_Tick(false, true);
                }
                else{
                    Miss.play();
                    Game_Tick(true, true);
                }
            }
        }
        else{
            // No high-scoring targets, resort to random hits
            do{
                row = distribution(gen);
                col = distribution(gen);
            }while(!gridWidgets[1]->IsHitLegal(row, col));

            // Introduce a chance of missing the target
            std::uniform_real_distribution<double> missDist(0.0, 1.0);
            double missChance = 0.2; // Adjust the miss chance as desired
            if (missDist(gen) <= missChance){
                // Missed the target
                Game_Tick(true, true);
            }
            else{
                if(gridWidgets[1]->Hit(row, col, UserGameMap)){
                    Hit.play();
                    UserHealth--;
                    Game_Tick(false, true);
                }
                else{
                    Miss.play();
                    Game_Tick(true, true);
                }
            }
        }
    }
        break;
    default:
        break;
    }
}
void MainWindow::Game_Tick(bool ChangeTurn, bool Versus_AI){// Handles change of turns and change of hp

    if(Versus_AI){
        ui->progressBar->setValue(UserHealth*5);
        ui->progressBar_2->setValue(EnemyHealth*5);

        if(UserHealth<=0){FinishGame(false); return;}
        if(EnemyHealth<=0){FinishGame(true); return;}
    }
    else{
        ui->progressBar_MultiPlayer_User->setValue(UserHealth*5);
        ui->progressBar_MultiPlayer_Enemy->setValue(EnemyHealth*5);
    }
    if(!IsClient){
    if(UserHealth<=0){Network_Host_SendMessage(MESSAGE_CLIENT_WON); FinishGame(false); ui->stackedWidget->setCurrentIndex(4); return;}
    if(EnemyHealth<=0){Network_Host_SendMessage(MESSAGE_HOST_WON); FinishGame(true); ui->stackedWidget->setCurrentIndex(4); return;}
    }


    if(ChangeTurn){
        if(Versus_AI){
            if(IsUserTurn){IsUserTurn=false; gridWidgets[2]->setDisabled(true);}
            else{IsUserTurn=true; gridWidgets[2]->setDisabled(false);}
        }
        else{
            if(IsUserTurn){IsUserTurn=false; gridWidgets[5]->setDisabled(true);}
            else{IsUserTurn=true; gridWidgets[5]->setDisabled(false);}
        }
    }

    if(Versus_AI&!IsUserTurn){
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> distribution(0, 6000-((DifficultyMode-1)*2000));
        AiHitDelay.setInterval(distribution(gen));
        AiHitDelay.start();
    }
}
bool MainWindow::checkShipPlacement(int startRow, int startCol, int shipLength, bool isVertical, int MapToCheck[10][10]){// Checks if ship is fully inside the grid, and isn't too close/overlap other ships
    // Check if the ship placement exceeds the grid boundaries
    if (isVertical){
        if(startRow+shipLength>10){
            return false; // Ship exceeds grid vertically
        }
        for (int row = startRow - 1; row <= startRow + shipLength; ++row) {
            for (int col = startCol - 1; col <= startCol + 1; ++col) {
                if (isValidPosition(row, col) && MapToCheck[row][col] == 1) {
                    return false; // Ship overlaps with another ship or too close to another ship
                }
            }
        }
    }
    if(!isVertical){
        if((startCol-shipLength+1)<0){
            return false; // Ship exceeds grid horizontally
        }
        for (int row = startRow - 1; row <= startRow + 1; ++row) {
            for (int col = startCol - shipLength; col <= startCol + 1; ++col) {
                if (isValidPosition(row, col) && MapToCheck[row][col] == 1) {
                    return false; // Ship overlaps with another ship or too close to another ship
                }
            }
        }
    }

    return true; // Ship can be placed without overlapping
}

void MainWindow::GridButtonPressed(int row, int col){
    if(ui->stackedWidget->currentIndex()==2){//Singleplayer preparation
        if(IsHolding){//Player holding a ship
            int AmountToAdd = 0;
            QLabel* ToModify;
            if(HoldingType=="Carrier"){// Player holding a Carrier
                AmountToAdd=4;
                ToModify = ui->label_SinglePlayerPrep_Carrier;
            }
            else if(HoldingType=="Cruiser"){// Player holding a Cruiser
                AmountToAdd=3;
                ToModify = ui->label_SinglePlayerPrep_Cruiser;
            }
            else if(HoldingType=="Submarine"){// Player holding a Submarine
                AmountToAdd=2;
                ToModify = ui->label_SinglePlayerPrep_Submarine;
            }
            else{// Player holding a Boat
                AmountToAdd=1;
                ToModify = ui->label_SinglePlayerPrep_Boat;
            }
            if(checkShipPlacement(row, col, AmountToAdd, IsVertical, UserGameMap)){
                AddShip(row, col, AmountToAdd, IsVertical, UserGameMap);
                gridWidgets[0]->updateButtonStyles(UserGameMap);
                ToModify->setText("x"+QString::number(ToModify->text().mid(1).toInt()-1));

                if(IsPreparationComplete(true)){ui->pushButton_SinglePlayerPrep_Start->setEnabled(true);}

                IsHolding=false;
                HoldingType="N/A";
                SetCursorOnPrep();
            }
        }
    }
    else if(ui->stackedWidget->currentIndex()==3){//Singleplayer gameplay
        if(gridWidgets[2]->IsHitLegal(row, col)){
            if(gridWidgets[2]->Hit(row, col, EnemyGameMap)){Hit.play(); EnemyHealth--; Game_Tick(false, true);}
            else{Miss.play(); Game_Tick(true, true);}
        }
        else{qDebug()<<"This hit is illegal!";}
    }

    if(ui->stackedWidget->currentIndex()==5){//Multiplayer preparation
        if(IsHolding){//Player holding a ship
            int AmountToAdd = 0;
            QLabel* ToModify;
            if(HoldingType=="Carrier"){// Player holding a Carrier
                AmountToAdd=4;
                ToModify = ui->label_MultiPlayer_Carrier;
            }
            else if(HoldingType=="Cruiser"){// Player holding a Cruiser
                AmountToAdd=3;
                ToModify = ui->label_MultiPlayer_Cruiser;
            }
            else if(HoldingType=="Submarine"){// Player holding a Submarine
                AmountToAdd=2;
                ToModify = ui->label_MultiPlayer_Submarine;
            }
            else{// Player holding a Boat
                AmountToAdd=1;
                ToModify = ui->label_MultiPlayer_Boat;
            }
            if(checkShipPlacement(row, col, AmountToAdd, IsVertical, UserGameMap)){
                AddShip(row, col, AmountToAdd, IsVertical, UserGameMap);
                gridWidgets[3]->updateButtonStyles(UserGameMap);
                ToModify->setText("x"+QString::number(ToModify->text().mid(1).toInt()-1));

                if(IsPreparationComplete(false)){ui->pushButton_MultiPlayerPrep_Start->setEnabled(true);}

                IsHolding=false;
                HoldingType="N/A";
                SetCursorOnPrep();
            }
        }
    }
    else if(ui->stackedWidget->currentIndex()==6){//Multiplayer gameplay
        if(gridWidgets[5]->IsHitLegal(row, col)){
            if(gridWidgets[5]->Hit(row, col, EnemyGameMap)){Hit.play(); EnemyHealth--; Game_Tick(false, false);}
            else{Miss.play(); Game_Tick(true, false);}
            if(IsClient){// Sends a message to host that client did land a hit
                Network_Client_SendMessage(MESSAGE_CLIENT_HIT, row, col);
            }
            else{// Sends a message to client that host did land a hit
                Network_Host_SendMessage(MESSAGE_HOST_HIT, row, col);
            }
        }
        else{qDebug()<<"This hit is illegal!";}
    }
}

//-----=====[-MULTI-PAGE FUNCTIONS-]=====-----



//-----=====[+MAIN MENU+]=====-----
void MainWindow::on_pushButton_menuLanguage_clicked(){
    QSettings settings("DeltaMolfarApps", "Battleships");
    if(languageSetting=="en"){
        ui->pushButton_menuLanguage->setStyleSheet("image: url(:/new/prefix1/res/ukraine.png);\nbackground-color: rgba(255, 255, 255, 0);\nborder-color: rgba(255, 255, 255, 0);");
        languageSetting="ua";
    }
    else{
        ui->pushButton_menuLanguage->setStyleSheet("image: url(:/new/prefix1/res/united-kingdom.png);\nbackground-color: rgba(255, 255, 255, 0);\nborder-color: rgba(255, 255, 255, 0);");
        languageSetting="en";
    }
    settings.setValue("language", languageSetting);
    translateAll();
}
void MainWindow::on_pushButton_menuSingleplayer_clicked(){
    ui->stackedWidget->setCurrentIndex(1);
}
void MainWindow::on_pushButton_menuMultiplayer_clicked(){
    if(isCheckConnectedToInternet()){
        ui->stackedWidget->setCurrentIndex(4);
        ui->label_MultiPlayerSettings_IconUser->setStyleSheet("image: url("+UserImagePath+");");
        ui->label_MultiPlayerSettings_UserName->setText(UserName);
        ui->stackedWidget_MultiPlayerSettings->setCurrentIndex(0);
        ui->pushButton_MultiPlayerSettings_Connect->setEnabled(true);
        ui->pushButton_MultiPlayerSettings_Create->setEnabled(true);
        ui->pushButton_MultiPlayerSettings_Ready_or_Start->setEnabled(false);
        IsClient=true;
    }
    else{qWarning("You're not connected to Wi-Fi");}
}
void MainWindow::on_pushButton_menuSettings_clicked(){
    ui->stackedWidget->setCurrentIndex(7);
}


//profile
void MainWindow::on_pushButton_menuProfile_clicked(){// Opening profile settings
    if(languageSetting=="ua"){
        ui->label_Profile_CurrentName->setText("Ваше поточне ім'я:");
        ui->label_Profile_CurrentIcon->setText("Ваш поточний аватар:");
    }
    else{
        ui->label_Profile_CurrentName->setText("Your current name:");
        ui->label_Profile_CurrentIcon->setText("Your current icon:");
    }
    ui->label_Profile_Name->setText(UserName);
    ui->label_Profile_Icon->setStyleSheet("image: url("+UserImagePath+");");
    ui->stackedWidget->setCurrentIndex(8);
}
void MainWindow::on_pushButton_Profile_Save_clicked(){// Saving profile changes
    QSettings settings("DeltaMolfarApps", "Battleships");

    if(languageSetting=="ua"){
        ui->label_Profile_CurrentName->setText("Ваше поточне ім'я:");
        ui->label_Profile_CurrentIcon->setText("Ваш поточний аватар:");
    }
    else{
        ui->label_Profile_CurrentName->setText("Your current name:");
        ui->label_Profile_CurrentIcon->setText("Your current icon:");
    }

    UserName=ui->label_Profile_Name->text();

    static QRegularExpression regex("url\\(([^\\)]+)\\)");
    QRegularExpressionMatch match = regex.match(ui->label_Profile_Icon->styleSheet());
    if (match.hasMatch()) { UserImagePath=match.captured(1); }

    settings.setValue("UserName", UserName);
    settings.setValue("UserImagePath", UserImagePath);

    UnsavedProfile = false;
}
void MainWindow::on_pushButton_Profile_Back_clicked(){// Going back to main menu from profile
    if(UnsavedProfile){
        QMessageBox::StandardButton reply = QMessageBox::warning(this, "Unsaved Changes", "Warning: Unsaved changes!\nAre you sure you want to go back?", QMessageBox::Ok | QMessageBox::Cancel);
        if (reply == QMessageBox::Ok) {
            ui->stackedWidget->setCurrentIndex(0);
        }
        else{
            return;
        }
    }
    else{
        ui->stackedWidget->setCurrentIndex(0);
    }
}
void MainWindow::on_pushButton_Profile_ChangeName_clicked(){// Changing name
    QDialog newDialog;
    QString Title, ButtonOk, ButtonCancel, LabelOldName, LabelNewName;

    if(languageSetting == "ua"){
        Title = "Змінити ім'я";
        ButtonOk = "Змінити";
        ButtonCancel = "Відмінити";
        LabelOldName = "Ваше поточне ім'я: ";
        LabelNewName = "Ваше нове ім'я: ";
    }
    else{
        Title = "Change name";
        ButtonOk = "Change";
        ButtonCancel = "Cancel";
        LabelOldName = "Your Current Name:";
        LabelNewName = "Your new name:";
    }

    newDialog.setWindowTitle(Title);
    QLabel* label_OldName = new QLabel(LabelOldName, &newDialog);
    QLabel* label_CurName = new QLabel(UserName, &newDialog);
    QLabel* label_NewName = new QLabel(LabelNewName, &newDialog);
    QLineEdit* lineEdit = new QLineEdit(&newDialog);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &newDialog);

    connect(buttonBox, &QDialogButtonBox::accepted, &newDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &newDialog, &QDialog::reject);

    QHBoxLayout* labelLayout1 = new QHBoxLayout;
    labelLayout1->addWidget(label_OldName);
    labelLayout1->addWidget(label_CurName);

    QHBoxLayout* labelLayout2 = new QHBoxLayout;
    labelLayout2->addWidget(label_NewName);
    labelLayout2->addWidget(lineEdit);

    QHBoxLayout* labelLayout3 = new QHBoxLayout;
    labelLayout3->addWidget(buttonBox);

    QVBoxLayout* mainLayout = new QVBoxLayout(&newDialog);
    mainLayout->addLayout(labelLayout1);
    mainLayout->addStretch();
    mainLayout->addLayout(labelLayout2);
    mainLayout->addStretch();
    mainLayout->addLayout(labelLayout3);

    if (newDialog.exec() == QDialog::Accepted) {
        QString newName = lineEdit->text();
        QString LabelChange;
        if(languageSetting=="ua"){LabelChange="Ваше поточне ім'я*:";}
        else{LabelChange="Your current name*:";}
        ui->label_Profile_CurrentName->setText(LabelChange);
        ui->label_Profile_Name->setText(newName);
        UnsavedProfile = true;
    }

    delete label_OldName;
    delete label_CurName;
    delete label_NewName;
    delete lineEdit;
    delete buttonBox;
    delete labelLayout1;
    delete labelLayout2;
    delete labelLayout3;
    delete mainLayout;
}

void MainWindow::on_pushButton_Profile_Icon_clicked(){// Changing Icon
    QString FilePath, FileDialogTitle;
    if(languageSetting=="ua"){
        FileDialogTitle="Виберіть зображення";
    }
    else{
        FileDialogTitle="Select Image";
    }

    FilePath = QFileDialog::getOpenFileName(this, "", QString(), "Image Files (*.png *.jpg *.jpeg)");

    if (!FilePath.isEmpty()){
        QString LabelChange;
        if(languageSetting=="ua"){LabelChange="Ваш поточний аватар*:";}
        else{LabelChange="Your current icon*:";}
        ui->label_Profile_CurrentIcon->setText(LabelChange);
        ui->label_Profile_Icon->setStyleSheet("image: url("+FilePath+");");
        UnsavedProfile = true;
    }
    else{
        QString Title, Problem;
        if(languageSetting=="ua"){Title="Помилка"; Problem="Не вдалось відкрити файл!";}
        else{Title="Error"; Problem="File couldn't be open!";}
        QMessageBox::critical(this, Title, Problem);
    }

}

//settings
void MainWindow::on_pushButton_Settings_Back_clicked(){
    ui->stackedWidget->setCurrentIndex(0);
}
void MainWindow::on_horizontalSlider_valueChanged(int value){
    QSettings settings("DeltaMolfarApps", "Battleships");

    ui->label_Settings_MusicProc->setText(QString::number(value)+"%");
    MusicLevel=value;
    MainMusic.setVolume(static_cast<float>(MusicLevel)/100.0f);
    BattleMusic.setVolume(static_cast<float>(MusicLevel)/100.0f);
    settings.setValue("MusicLevel", QString::number(MusicLevel));
}
void MainWindow::on_horizontalSlider_2_valueChanged(int value){
    QSettings settings("DeltaMolfarApps", "Battleships");

    ui->label_Settings_FXProc->setText(QString::number(value)+"%");
    FXLevel=value;
    Miss.setVolume(static_cast<float>(FXLevel)/100.0f);
    Hit.setVolume(static_cast<float>(FXLevel)/100.0f);
    settings.setValue("FXLevel", QString::number(FXLevel));
}

//credits
void MainWindow::on_pushButton_menuCredits_clicked(){
    if(languageSetting=="ua"){
        QMessageBox::information(this, "Про гру", "Про гру:\n\n\
Ця гра була розроблена deltamolfar як open-source проєкт.\n\n\
Розробка:\n\
- Розробник: deltamolfar (Вітер Захар)\n\
- Дата Створення: 21.06.2023\n\n\
Причина:\n\
Ця гра \"Морський бій\" була розроблена та створена як екзаменаційний проєкт для \"Прикарпатський Національний Університет\", а потім опублікована як Open-Source проєкт.\n\n\
Open-Source:\n\
 - Вихідний код цієї гри доступний на GitHub за посиланням: http://github.com(ChangeLater)\n\
 - Ви маєте право досліджувати, модифікувати, поширювати, та використовувати цю програму будь як.\n\n\
Контакт:\n\
 - Для питань, ви можете написати мені на: goldgamer9000@gmail.com.\n\
");
    }
    else{
        QMessageBox::information(this, "Credits", "Game Credits:\n\n\
This game was created by deltamolfar as an open-source project.\n\n\
Development:\n\
- Game Developer: deltamolfar (Viter Zakhar)\n\
- Date of Creation: 21.06.2023\n\n\
Purpose:\n\
This battleship game was designed and developed as an exam project for \"Прикарпатський Національний Університет\", and then published Open-Source.\n\n\
Open-Source:\n\
 - The source code of this game is available on GitHub at: http://github.com(ChangeLater)\n\
 - Feel free to explore, modify, contribute, distribute, and use in any means.\n\n\
Contact:\n\
 - For questions, you reach me out to: goldgamer9000@gmail.com.\n\
");
    }
}
//-----=====[-MAIN MENU-]=====-----



//-----=====[+SINGLE PLAYER MENU+]=====-----
void MainWindow::DifficultyStyleSheetHandler(){// Changes difficulty style sheets different depending on chosen mode. Could've been made more easy to read, but it contains no logic except for stylesheets
    QString EasyIconPath=":/new/prefix1/res/ship_easy.png", MidIconPath=":/new/prefix1/res/ship_mid.png", HardIconPath=":/new/prefix1/res/ship_hard.png";

    if(ui->pushButton_SinglePlayer_EasyMode->isChecked()){
            ui->pushButton_SinglePlayer_EasyMode->setStyleSheet("QPushButton {\nimage: url("+EasyIconPath+");\nbackground-color: rgb(110, 110, 110);\nborder: 4px solid rgb(30,230,30);\nborder-radius: 15px;\npadding: 32;\nfont: 400 16pt \"Segoe UI\";\n}\nQPushButton:hover {\nbackground-color: #0066CC;\nborder-color: #1E90FF;\n}\nQPushButton:pressed {\nbackground-color: #003399;\nborder-color: #1E90FF;\n}");
    }
    else{
            ui->pushButton_SinglePlayer_EasyMode->setStyleSheet("QPushButton {\nimage: url("+EasyIconPath+");\nbackground-color: rgb(60, 60, 60);\nborder: 4px solid rgb(150,150,150);\nborder-radius: 15px;\npadding: 64;\nfont: 400 16pt \"Segoe UI\";\n}\nQPushButton:hover {\nbackground-color: #0066CC;\nborder-color: #1E90FF;\n}\nQPushButton:pressed {\nbackground-color: #003399;\nborder-color: #1E90FF;\n}");
    }


    if(ui->pushButton_SinglePlayer_MediumMode->isChecked()){
            ui->pushButton_SinglePlayer_MediumMode->setStyleSheet("QPushButton {\nimage: url("+MidIconPath+");\nbackground-color: rgb(110, 110, 110);\nborder: 4px solid rgb(30,230,30);\nborder-radius: 15px;\npadding: 32;\nfont: 400 16pt \"Segoe UI\";\n}\nQPushButton:hover {\nbackground-color: #0066CC;\nborder-color: #1E90FF;\n}\nQPushButton:pressed {\nbackground-color: #003399;\nborder-color: #1E90FF;\n}");
    }
    else{
            ui->pushButton_SinglePlayer_MediumMode->setStyleSheet("QPushButton {\nimage: url("+MidIconPath+");\nbackground-color: rgb(60, 60, 60);\nborder: 4px solid rgb(150,150,150);\nborder-radius: 15px;\npadding: 64;\nfont: 400 16pt \"Segoe UI\";\n}\nQPushButton:hover {\nbackground-color: #0066CC;\nborder-color: #1E90FF;\n}\nQPushButton:pressed {\nbackground-color: #003399;\nborder-color: #1E90FF;\n}");
    }


    if(ui->pushButton_SinglePlayer_HardMode->isChecked()){
            ui->pushButton_SinglePlayer_HardMode->setStyleSheet("QPushButton {\nimage: url("+HardIconPath+");\nbackground-color: rgb(110, 110, 110);\nborder: 4px solid rgb(30,230,30);\nborder-radius: 15px;\npadding: 32;\nfont: 400 16pt \"Segoe UI\";\n}\nQPushButton:hover {\nbackground-color: #0066CC;\nborder-color: #1E90FF;\n}\nQPushButton:pressed {\nbackground-color: #003399;\nborder-color: #1E90FF;\n}");
    }
    else{
            ui->pushButton_SinglePlayer_HardMode->setStyleSheet("QPushButton {\nimage: url("+HardIconPath+");\nbackground-color: rgb(60, 60, 60);\nborder: 4px solid rgb(150,150,150);\nborder-radius: 15px;\npadding: 64;\nfont: 400 16pt \"Segoe UI\";\n}\nQPushButton:hover {\nbackground-color: #0066CC;\nborder-color: #1E90FF;\n}\nQPushButton:pressed {\nbackground-color: #003399;\nborder-color: #1E90FF;\n}");
    }
}

void MainWindow::SinglePlayerButtonChange(int Num){// When clicked on any of singleplayer difficulty mode button
    switch(Num){
        case 1:{// Clicked on easy
                ui->label_SinglePlayer_Menu_Difficulty->setText("Easy");
                ui->pushButton_SinglePlayer_EasyMode->setChecked(true);
                ui->pushButton_SinglePlayer_MediumMode->setChecked(false);
                ui->pushButton_SinglePlayer_HardMode->setChecked(false);
                DifficultyMode=1;
                break;
        }
        case 2:{// Clicked on medium
                ui->label_SinglePlayer_Menu_Difficulty->setText("Medium");
                ui->pushButton_SinglePlayer_EasyMode->setChecked(false);
                ui->pushButton_SinglePlayer_MediumMode->setChecked(true);
                ui->pushButton_SinglePlayer_HardMode->setChecked(false);
                DifficultyMode=2;
                break;
        }
        case 3:{// Clicked on hard
                ui->label_SinglePlayer_Menu_Difficulty->setText("Hard");
                ui->pushButton_SinglePlayer_EasyMode->setChecked(false);
                ui->pushButton_SinglePlayer_MediumMode->setChecked(false);
                ui->pushButton_SinglePlayer_HardMode->setChecked(true);
                DifficultyMode=3;
                break;
        }
    }
    DifficultyStyleSheetHandler();
}

void MainWindow::on_pushButton_SinglePlayer_EasyMode_clicked(){
    SinglePlayerButtonChange(1);
}
void MainWindow::on_pushButton_SinglePlayer_MediumMode_clicked(){
    SinglePlayerButtonChange(2);
}
void MainWindow::on_pushButton_SinglePlayer_HardMode_clicked(){
    SinglePlayerButtonChange(3);
}
void MainWindow::on_pushButton_SinglePlayer_Menu_Play_clicked(){//Pressed when done with settings, goes to putting battleships on a grid
    ui->label_SinglePlayerPrep_Carrier->setText("x1");
    ui->label_SinglePlayerPrep_Cruiser->setText("x2");
    ui->label_SinglePlayerPrep_Submarine->setText("x3");
    ui->label_SinglePlayerPrep_Boat->setText("x4");
    ui->pushButton_SinglePlayerPrep_Start->setEnabled(false);

    ResetMaps();
    gridWidgets[0]->updateButtonStyles(UserGameMap);
    ui->stackedWidget->setCurrentIndex(2);
}
void MainWindow::on_pushButton_SinglePlayer_Menu_Back_clicked(){
    ui->stackedWidget->setCurrentIndex(0);
}
//-----=====[-SINGLE PLAYER MENU-]=====-----



//-----=====[+SINGLE PLAYER PREPARATION+]=====-----
void MainWindow::on_pushButton_SinglePlayerPrep_Start_clicked(){
    gridWidgets[1]->updateButtonStyles(UserGameMap);
    GetEnemyMap(true);
    //gridWidgets[2]->updateButtonStyles(EnemyGameMap); - Uncomment to see enemy's ships

    switch(DifficultyMode){
        case 1:
            ui->label_SinglePlayerGameplay_AINAME->setText("AI (easy)");
            break;
        case 2:
            ui->label_SinglePlayerGameplay_AINAME->setText("AI (medium)");
            break;
        case 3:
            ui->label_SinglePlayerGameplay_AINAME->setText("AI (hard)");
            break;
        default: qDebug()<<"Error! DifficultyMode = "<<DifficultyMode;
    }

    ui->label_SinglePlayerGameplay_UserName->setText(UserName);
    StartGame(true);
    ui->stackedWidget->setCurrentIndex(3);
}
//Functions of ship-buttons
void MainWindow::on_pushButton_SinglePlayerPrep_Carrier_clicked(){
    if(IsHolding&&HoldingType=="Carrier"){IsHolding=false; HoldingType="N/A";}
    else{if(ui->label_SinglePlayerPrep_Carrier->text()=="x0"){return;}IsHolding=true; HoldingType="Carrier";}
    IsVertical=false;
    SetCursorOnPrep();
}
void MainWindow::on_pushButton_SinglePlayerPrep_Cruiser_clicked(){
    if(IsHolding&&HoldingType=="Cruiser"){IsHolding=false; HoldingType="N/A";}
    else{if(ui->label_SinglePlayerPrep_Cruiser->text()=="x0"){return;}IsHolding=true; HoldingType="Cruiser";}
    IsVertical=false;
    SetCursorOnPrep();
}
void MainWindow::on_pushButton_SinglePlayerPrep_Submarine_clicked(){
    if(IsHolding&&HoldingType=="Submarine"){IsHolding=false; HoldingType="N/A";}
    else{if(ui->label_SinglePlayerPrep_Submarine->text()=="x0"){return;}IsHolding=true; HoldingType="Submarine";}
    IsVertical=false;
    SetCursorOnPrep();
}
void MainWindow::on_pushButton_SinglePlayerPrep_Boat_clicked(){
    if(IsHolding&&HoldingType=="Boat"){IsHolding=false; HoldingType="N/A";}
    else{if(ui->label_SinglePlayerPrep_Boat->text()=="x0"){return;}IsHolding=true; HoldingType="Boat";}
    IsVertical=false;
    SetCursorOnPrep();
}
void MainWindow::on_pushButton_SinglePlayerPrep_Back_clicked(){
    ui->stackedWidget->setCurrentIndex(1);
    IsHolding=false;
    HoldingType="N/A";
    SetCursorOnPrep();
}
void MainWindow::on_pushButton_SinglePlayerPrep_Rules_clicked(){
    QMessageBox msg;
    if(languageSetting=="ua"){msg.setText("Існує всьго декілька правил розставлення кораблів:\n \
- Корабель повинен знаходитись повністю в ігровому полі\n \
- Корабель не повинен натраляти на інший корабель\n \
- Корабель повинен бути на відстані хоча б в 1 клітинку від інших кораблів");}
    else{msg.setText("There's only few rules of ship placement:\n \
- Ship must be fully inside the play grid\n \
- Ship shouldn't ovelap other ships\n \
- Ship must have at least 1 block gap to other ships");}
     msg.exec();
}
//-----=====[-SINGLE PLAYER PREPARATION-]=====-----



//-----=====[+SINGLE PLAYER GAMEPLAY+]=====-----
void MainWindow::on_pushButton_SinglePlayerGameplay_Surrender_clicked(){//When clicked on surrender. Are you sure window pops out.
     QMessageBox::StandardButton SurrenderMSG;
     QString Title, Message;
     if(languageSetting=="ua"){Title = "Здатись"; Message = "Ви впевнені, що хочете здатись?";}
     else{Title = "Surrender"; Message = "Are you sure you want to surrender?";}
     SurrenderMSG = QMessageBox::question(this, Title, Message, QMessageBox::Yes|QMessageBox::No);
     if (SurrenderMSG == QMessageBox::Yes) {
            FinishGame(false);
            ui->stackedWidget->setCurrentIndex(0);
     }
}
//-----=====[-SINGLE PLAYER GAMEPLAY-]=====-----



//-----=====[+MULTI PLAYER NETWORKING+]=====-----
// Handles connection from client to host (If User is a host)
void MainWindow::Network_NewConnectionHandler(){
    // Accept the incoming connection
    clientSocket = server->nextPendingConnection();
    ui->pushButton_MultiPlayerSettings_Ready_or_Start->setEnabled(true);
    ui->label_MultiPlayerSettings_HostStatus->setText("Host status: Player connected. Waiting for you to start a game!");

    // Connect the readyRead signal to a slot to handle incoming data from the client
    connect(clientSocket, &QTcpSocket::readyRead, this, &MainWindow::Network_Host_HandleClientData);

    QSoundEffect soundEffect;
    soundEffect.setSource(QUrl::fromLocalFile(":/new/prefix1/res/mixkit-unlock-new-item-game-notification-254.wav"));
    soundEffect.setVolume(FXLevel/100);
    soundEffect.setLoopCount(1);

    soundEffect.play();
}
// Handles connection of client to host (If User is client)
void MainWindow::Network_ClientConnetedToHostHandler(){

    ui->pushButton_MultiPlayerSettings_Connect->setEnabled(false);
    ui->pushButton_MultiPlayerSettings_Create->setEnabled(false);
    ui->stackedWidget_MultiPlayerSettings->setCurrentIndex(2);
    connect(clientSocket, &QTcpSocket::readyRead, this, &MainWindow::Network_Client_HandleHostData);

    QSoundEffect soundEffect;
    soundEffect.setSource(QUrl::fromLocalFile(":/new/prefix1/res/mixkit-unlock-new-item-game-notification-254.wav"));
    soundEffect.setVolume(FXLevel/100);
    soundEffect.setLoopCount(1);

    soundEffect.play();
}

void MainWindow::Network_ClientConnectionError(QAbstractSocket::SocketError error){
    QString ErrorString;
    switch (error){
    case QAbstractSocket::ConnectionRefusedError:
           ErrorString = "Connection refused by the host.";
           break;
    case QAbstractSocket::RemoteHostClosedError:
           ErrorString = "The remote host closed the connection.";
           break;
    case QAbstractSocket::HostNotFoundError:
           ErrorString = "The host was not found.";
           break;
    case QAbstractSocket::SocketTimeoutError:
           ErrorString = "Socket operation timed out.";
           break;
    // Add more cases for other error types as needed
    default:
           ErrorString = "Unkown error";
           break;
    }

    if(languageSetting=="ua"){QMessageBox::critical(this, "Помилка!", "Не вдалось підключитись до хоста.\n Помилка:"+ErrorString);}
    else{{QMessageBox::critical(this, "Error!", "Couldn't connect to host.\n Error:"+ErrorString);}}
}

void MainWindow::Network_Host_HandleClientData(){
    // Read the incoming data
    QByteArray data = clientSocket->readAll();
    QDataStream stream(&data, QIODevice::ReadOnly);

    QString message;
    stream >> message;// Message is always goes first. Declares type of message received

    if(message == MESSAGE_CLIENT_READY){//Client pressed 'ready' button. Download his GameUserMap as EnemyGameMap.
        ui->label_MultiPlayerPrep_EnemyReady->setText("Enemy: Ready");

        int receivedArray[10][10];

        for (int i = 0; i < 10; ++i) {
            for (int j = 0; j < 10; ++j) {
                stream >> receivedArray[i][j];
            }
        }

        for(int i = 0; i < 10; ++i){
            for (int j = 0; j < 10; ++j) {
                EnemyGameMap[i][j] = receivedArray[i][j];
            }
        }

    }
    else if(message == MESSAGE_CLIENT_DISCONNECTED){// When client disconnected (by his will, or he closed game)
        if(languageSetting=="ua"){QMessageBox::critical(this, "Відключення", "Клієнт вийшов із гри!");}
        else{QMessageBox::critical(this, "Disconnect", "Client left the game!");}
        server->close();
        ui->label_MultiPlayerSettings_HostStatus->setText("Host status: Waiting for another player");
        ui->stackedWidget->setCurrentIndex(0);
    }
    else if(message == MESSAGE_CLIENT_HIT){// Receives where client pressed "hit".
        int row, col;
        stream >> row >> col;
        if(gridWidgets[4]->Hit(row, col, UserGameMap)){UserHealth--; Game_Tick(false, false);}
        else{Game_Tick(true, false);}
    }
}
void MainWindow::Network_Client_HandleHostData(){
    // Read the incoming data
    QByteArray data = clientSocket->readAll();
    QDataStream stream(&data, QIODevice::ReadOnly);

    QString message;
    stream >> message;

    if (message == MESSAGE_START_PREP){
        ui->stackedWidget->setCurrentIndex(5);
        ui->label_MultiPlayerPrep_EnemyReady->setText("Enemy: Not ready");
        ui->label_MultiPlayerPrep_UserReady->setText("You: Not ready");
        ResetMaps();
        IsHolding=false;
        HoldingType="N/A";
        SetCursorOnPrep();
    }
    else if(message == MESSAGE_HOST_READY){
        ui->label_MultiPlayerPrep_EnemyReady->setText("Enemy: Ready");

        // Create a new 2D array to store the received data
        int receivedArray[10][10];

        // Read each element from the stream and populate the received array
        for (int i = 0; i < 10; ++i) {
            for (int j = 0; j < 10; ++j) {
                stream >> receivedArray[i][j];
            }
        }

        // Copy the contents of the received array to EnemyGameMap
        for(int i = 0; i < 10; ++i){
            for (int j = 0; j < 10; ++j) {
                EnemyGameMap[i][j] = receivedArray[i][j];
            }
        }
    }
    else if(message == MESSAGE_START_GAME){
        gridWidgets[4]->updateButtonStyles(UserGameMap);
        ui->stackedWidget->setCurrentIndex(6);
        StartGame(false);
    }
    else if(message == MESSAGE_CLIENT_WON){
        FinishGame(true);
        ui->stackedWidget->setCurrentIndex(4);
    }
    else if(message == MESSAGE_HOST_WON){
        FinishGame(false);
        ui->stackedWidget->setCurrentIndex(4);
    }
    else if(message == MESSAGE_HOST_DISCONNECTED){
        if(languageSetting=="ua"){QMessageBox::critical(this, "Відключення", "Хост вийшов із гри!");}
        else{QMessageBox::critical(this, "Disconnect", "Host left the game!");}
        clientSocket->close();
        ui->stackedWidget->setCurrentIndex(0);
    }
    else if(message == MESSAGE_HOST_HIT){
        int row, col;
        stream >> row >> col;
        if(gridWidgets[4]->Hit(row, col, UserGameMap)){UserHealth--; Game_Tick(false, false);}
        else{Game_Tick(true, false);}
    }

}
//Sends a message of specified type to client. Row and Col are defaulted to zero
void MainWindow::Network_Host_SendMessage(QString MessageType, int row, int col, int DoubleArray[10][10]){
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);

    if (MessageType == MESSAGE_START_PREP){
        stream << MESSAGE_START_PREP;
    }
    else if(MessageType == MESSAGE_HOST_READY){
        stream << MESSAGE_HOST_READY;

        // Write the UserGameMap array to the stream
        for (int i = 0; i < 10; ++i) {
            for (int j = 0; j < 10; ++j) {
                stream << DoubleArray[i][j];
            }
        }
    }
    else if(MessageType == MESSAGE_START_GAME){
        stream << MESSAGE_START_GAME;
    }
    else if(MessageType == MESSAGE_CLIENT_WON){
        stream << MESSAGE_CLIENT_WON;
    }
    else if(MessageType == MESSAGE_HOST_WON){
        stream << MESSAGE_HOST_WON;
    }
    else if(MessageType == MESSAGE_HOST_DISCONNECTED){
        stream << MESSAGE_HOST_DISCONNECTED;
    }
    else if(MessageType == MESSAGE_HOST_HIT){
        stream << MESSAGE_HOST_HIT << row << col;
    }

    clientSocket->write(data);
}
//Sends a message of specified type to host. Row and Col are defaulted to zero
void MainWindow::Network_Client_SendMessage(QString MessageType, int row, int col, int DoubleArray[10][10]){
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);

    if(MessageType == MESSAGE_CLIENT_READY){
        stream << MESSAGE_CLIENT_READY;

        // Write the UserGameMap array to the stream
        for (int i = 0; i < 10; ++i) {
            for (int j = 0; j < 10; ++j) {
                stream << DoubleArray[i][j];
            }
        }
    }
    else if(MessageType == MESSAGE_CLIENT_DISCONNECTED){
        stream << MESSAGE_CLIENT_DISCONNECTED;
    }
    else if(MessageType == MESSAGE_CLIENT_HIT){
        stream << MESSAGE_CLIENT_HIT << row << col;
    }

    clientSocket->write(data);
}
//-----=====[-MULTI PLAYER NETWORKING-]=====-----



//-----=====[+MULTI PLAYER MENU+]=====-----
void MainWindow::on_pushButton_MultiPlayerSettings_Back_clicked(){
    QMessageBox::StandardButton BackMSG;
    QString Title, Message;
    if(languageSetting=="ua"){Title = "Вернутись?"; Message = "Все буде скинуто!";}
    else{Title = "Go back?"; Message = "Everything will be reset!";}
    BackMSG = QMessageBox::question(this, Title, Message, QMessageBox::Yes|QMessageBox::No);
    if (BackMSG == QMessageBox::Yes) {
        ui->stackedWidget->setCurrentIndex(0);
        if(!IsClient){Network_Host_SendMessage(MESSAGE_HOST_DISCONNECTED); server->close();}
        else{Network_Client_SendMessage(MESSAGE_CLIENT_DISCONNECTED); clientSocket->close();}
        IsClient=true;
    }
}
void MainWindow::on_pushButton_MultiPlayerSettings_Ready_or_Start_clicked(){
    if(IsClient){return;}// Client should not be able to press on it. So that's just a fail-safe
    ui->label_MultiPlayerPrep_EnemyReady->setText("Enemy: Not ready");
    ui->label_MultiPlayerPrep_UserReady->setText("You: Not ready");

    ResetMaps();

    ui->stackedWidget->setCurrentIndex(5);

    IsHolding=false;
    HoldingType="N/A";
    SetCursorOnPrep();

    Network_Host_SendMessage(MESSAGE_START_PREP);
}
void MainWindow::on_pushButton_MultiPlayerSettings_QnA_clicked(){
    QMessageBox msg;
    if(languageSetting=="ua"){msg.setText("Часті питання:\n - Як приєднатись до друга? -> Вам потрібно запитати його IP і вписати його у вікно підключення \
\n- Як знайти моє IP? - Відвідайте https://www.myip.com/");}
    else{msg.setText("General Question:\n - How to connect to my friend? -> You need to ask him his IP and put into connection window \
\n- How to get IP? - Visit https://www.myip.com/");}
    msg.exec();
}
void MainWindow::on_pushButton_MultiPlayerSettings_Create_clicked(){
    server = new QTcpServer(this);

    if(!server->listen(QHostAddress::Any, 2323)){
        qDebug() << "Server could not start!" << server->errorString();
        return;
    }
    IsClient=false;
    QString LocalIpAddress = getWlanIpAddress();
    if(languageSetting=="ua"){QMessageBox::information(this, "Хост створено", "Ваш внутрішній IP:"+LocalIpAddress);}
    else{QMessageBox::information(this, "Host created", "Your internal IP:"+LocalIpAddress);}
    ui->stackedWidget_MultiPlayerSettings->setCurrentIndex(1);
    ui->label_MultiPlayerSettings_InternalIP->setText("Your internal IP: "+LocalIpAddress);
    ui->pushButton_MultiPlayerSettings_Connect->setEnabled(false);
    ui->pushButton_MultiPlayerSettings_Create->setEnabled(false);
    connect(server, &QTcpServer::newConnection, this, &MainWindow::Network_NewConnectionHandler);

    qDebug() << "Server started. Waiting for incoming connection";
}
// Tries to connect to host
void MainWindow::on_pushButton_MultiPlayerSettings_Connect_clicked(){
    clientSocket = new QTcpSocket(this);

    // Connect the connected signal to a slot to handle successful connection
    connect(clientSocket, &QTcpSocket::connected, this, &MainWindow::Network_ClientConnetedToHostHandler);

    // Connect the error signal to a slot to handle connection errors
    connect(clientSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(Network_ClientConnectionError(QAbstractSocket::SocketError)));


    QDialog dialog(this);
    QLabel* label = new QLabel("Host IP:", &dialog);
    QLineEdit* lineEdit = new QLineEdit(&dialog);
    QPushButton* okButton = new QPushButton("OK", &dialog);
    QPushButton* cancelButton = new QPushButton("Cancel", &dialog);

    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    layout->addWidget(label);
    layout->addWidget(lineEdit);
    layout->addWidget(okButton);
    layout->addWidget(cancelButton);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    QString hostIpAddress;

    if (dialog.exec() == QDialog::Accepted) {
        hostIpAddress = lineEdit->text();
    }
    else{
        return;
    }

    // Connect to the host at the specified IP address and port 2323
    clientSocket->connectToHost(hostIpAddress, 2323);

    qDebug() << "Connecting to the host...";
}

//-----=====[-MULTI PLAYER MENU-]=====-----



//-----=====[+MULTI PLAYER PREPARATIONS+]=====-----
void MainWindow::on_pushButton_MultiPlayerPrep_Start_clicked(){
    if(IsClient && ui->label_MultiPlayerPrep_UserReady->text() != "You: Ready"){
        Network_Client_SendMessage(MESSAGE_CLIENT_READY, 0, 0, UserGameMap);
        ui->label_MultiPlayerPrep_UserReady->setText("You: Ready");
        ui->pushButton_MultiPlayerPrep_Start->setEnabled(false);
        return;
    }
    else if(!IsClient && ui->label_MultiPlayerPrep_UserReady->text() != "You: Ready"){
        Network_Host_SendMessage(MESSAGE_HOST_READY, 0, 0, UserGameMap);
        ui->label_MultiPlayerPrep_UserReady->setText("You: Ready");
        if(languageSetting=="ua"){ui->pushButton_MultiPlayerPrep_Start->setText("ПОЧАТИ БИТВУ");}
        else{ui->pushButton_MultiPlayerPrep_Start->setText("START BATTLE");
        }
        return;
    }

    if(!IsClient && ui->label_MultiPlayerPrep_UserReady->text()=="You: Ready"&&ui->label_MultiPlayerPrep_EnemyReady->text()=="Enemy: Ready"){
        Network_Host_SendMessage(MESSAGE_START_GAME);
        gridWidgets[4]->updateButtonStyles(UserGameMap);
        StartGame(false);
        ui->stackedWidget->setCurrentIndex(6);
    }
    else{
        if(languageSetting=="ua"){QMessageBox::critical(this, "Не дозволено", "Обидва гравці повинні бути готові до початку гри!");}
        else{QMessageBox::critical(this, "Not allowed", "Both players has to be ready for game!");}
    }
}
void MainWindow::on_pushButton_MultiPlayer_Carrier_clicked(){
    if(IsHolding&&HoldingType=="Carrier"){IsHolding=false; HoldingType="N/A";}
    else{if(ui->label_MultiPlayer_Carrier->text()=="x0"){return;}IsHolding=true; HoldingType="Carrier";}
    IsVertical=false;
    SetCursorOnPrep();
}
void MainWindow::on_pushButton_MultiPlayer_Cruiser_clicked(){
    if(IsHolding&&HoldingType=="Cruiser"){IsHolding=false; HoldingType="N/A";}
    else{if(ui->label_MultiPlayer_Cruiser->text()=="x0"){return;}IsHolding=true; HoldingType="Cruiser";}
    IsVertical=false;
    SetCursorOnPrep();
}
void MainWindow::on_pushButton_MultiPlayer_Submarine_clicked(){
    if(IsHolding&&HoldingType=="Submarine"){IsHolding=false; HoldingType="N/A";}
    else{if(ui->label_MultiPlayer_Submarine->text()=="x0"){return;}IsHolding=true; HoldingType="Submarine";}
    IsVertical=false;
    SetCursorOnPrep();
}
void MainWindow::on_pushButton_MultiPlayer_Boat_clicked(){
    if(IsHolding&&HoldingType=="Boat"){IsHolding=false; HoldingType="N/A";}
    else{if(ui->label_MultiPlayer_Boat->text()=="x0"){return;}IsHolding=true; HoldingType="Boat";}
    IsVertical=false;
    SetCursorOnPrep();
}
void MainWindow::on_pushButton_MultiPlayerPrep_Disconnect_clicked(){
    QMessageBox::StandardButton SurrenderMSG;
    QString Title, Message;

    if(languageSetting=="ua"){Title = "Від'єднатись"; Message = "Ви впевнені, що хочете Від'єднатись?";}
    else{Title = "Disconnect"; Message = "Are you sure you want to disconnect?";}

    SurrenderMSG = QMessageBox::question(this, Title, Message, QMessageBox::Yes|QMessageBox::No);

    if (SurrenderMSG == QMessageBox::Yes) {
        ui->stackedWidget->setCurrentIndex(0);
        if(!IsClient){Network_Host_SendMessage(MESSAGE_HOST_DISCONNECTED); server->close();}
        else{Network_Client_SendMessage(MESSAGE_CLIENT_DISCONNECTED); clientSocket->close();}
        IsClient=true;
    }
}
void MainWindow::on_pushButton_MultiPlayerPrep_Rules_clicked(){
    QMessageBox msg;
    if(languageSetting=="ua"){msg.setText("Існує всьго декілька правил розставлення кораблів:\n \
- Корабель повинен знаходитись повністю в ігровому полі\n \
- Корабель не повинен натраляти на інший корабель\n \
- Корабель повинен бути на відстані хоча б в 1 клітинку від інших кораблів");}
    else{msg.setText("There's only few rules of ship placement:\n \
- Ship must be fully inside the play grid\n \
- Ship shouldn't ovelap other ships\n \
- Ship must have at least 1 block gap to other ships");}
     msg.exec();
}
//-----=====[-MULTI PLAYER PREPARATIONS-]=====-----



//-----=====[+MULTI PLAYER GAMEPLAY+]=====-----
void MainWindow::on_pushButton_MultiPlayer_Surrender_clicked(){
    QMessageBox::StandardButton SurrenderMSG;
    QString Title, Message;

    if(languageSetting=="ua"){Title = "Здатись"; Message = "Ви впевнені, що хочете здатись?";}
    else{Title = "Surrender"; Message = "Are you sure you want to surrender?";}

    SurrenderMSG = QMessageBox::question(this, Title, Message, QMessageBox::Yes|QMessageBox::No);

    if (SurrenderMSG == QMessageBox::Yes) {
        ui->stackedWidget->setCurrentIndex(0);
        FinishGame(false);
        if(!IsClient){Network_Host_SendMessage(MESSAGE_HOST_DISCONNECTED); server->close();}
        else{Network_Client_SendMessage(MESSAGE_CLIENT_DISCONNECTED); clientSocket->close();}
        IsClient=true;
    }
}
//-----=====[-MULTI PLAYER GAMEPLAY-]=====-----
