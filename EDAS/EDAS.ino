// Libraries -------------------------------------------------------
#include <LiquidCrystal.h>
#include <SPI.h>
#include <SD.h>

// Constants -------------------------------------------------------
#define NEW_RUN_BUTTON 3
#define LOGGING_BUTTON 2
#define SD_PIN         4
#define LCD_RS         22
#define LCD_RW         23
#define LCD_EN         24
#define LCD_DB4        25
#define LCD_DB5        26
#define LCD_DB6        27
#define LCD_DB7        28

// Configuration ---------------------------------------------------
const int BAD_DATA_HOLD_TIME = 3000;
const int BUTTON_DEBOUNCE_TIME = 100;
const String CODE_VERSION = "0.1.0";

// Variables -------------------------------------------------------
LiquidCrystal lcd(LCD_RS, LCD_RW, LCD_EN, LCD_DB4, LCD_DB5, LCD_DB6, LCD_DB7);
bool dataCollectionPaused = false;
bool copyingFiles = false;
bool newRunButtonPressedDown = false;
bool loggingButtonPressedDown = false;
int newRunButtonLastPressTime = 0;
int loggingButtonLastPressTime = 0;
int newRunButtonHoldInitialTime = 0;
int runIndex = 0;
File runFile;

// Code ------------------------------------------------------------
void setup() {
  // Serial setup
  Serial.begin(9600);

  // SD setup
  SD.begin(SD_PIN);

  // LCD setup
  lcd.begin(16, 2);
  lcd.noAutoscroll();

  // Pin setup
  pinMode(LOGGING_BUTTON, INPUT);
  pinMode(NEW_RUN_BUTTON, INPUT);

  // Start up screen
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write("HELLO, BAJA");

  char versionLabel[14];
  snprintf(versionLabel, 14, "VERSION %s", CODE_VERSION);
  lcd.setCursor(0, 1);
  lcd.write(versionLabel);
  
  delay(2000);

  // REMOVE ---------------------------------------------
  startNewRun(false);
  return;
  // REMOVE ---------------------------------------------

  // Get run index from last shut down
  File current = SD.open("/").openNextFile();
  runIndex = 1;

  while (true){
    if (!current){
      startNewRun(false);
      break;
    }

    runIndex++;
    current.close();
    current.openNextFile();
  }
}

void loop() {
  // Bail if files are being copied
  if (copyingFiles)
    return;
  
  if (!dataCollectionPaused){
      // ADD FUNCTIONALITY: Collect data
  }
  
  // New run button
  if (newRunButtonPressed() && !newRunButtonPressedDown){         // !NEW RUN BUTTON DOWN!
    // Button debounce
    if (millis() - newRunButtonLastPressTime < BUTTON_DEBOUNCE_TIME)
      return;
    newRunButtonLastPressTime = millis();
    
    newRunButtonPressedDown = true;
    newRunButtonHoldInitialTime = millis();
  }
  else if (!newRunButtonPressed() && newRunButtonPressedDown){    // !NEW RUN BUTTON UP!
    // Button debounce
    if (millis() - newRunButtonLastPressTime < BUTTON_DEBOUNCE_TIME)
      return;
    newRunButtonLastPressTime = millis();
    
    newRunButtonPressedDown = false;
    startNewRun(false);
  }

  if (newRunButtonPressed() && newRunButtonPressedDown){          // !NEW RUN BUTTON HELD DOWN!
    if (millis() - newRunButtonHoldInitialTime > BAD_DATA_HOLD_TIME)
      startNewRun(true);
  }

  // Logging button
  if (loggingButtonPressed() && !loggingButtonPressedDown){       // !LOGGING BUTTON DOWN!
    // Button debounce
    if (millis() - loggingButtonLastPressTime < BUTTON_DEBOUNCE_TIME)
      return;
    loggingButtonLastPressTime = millis();
    
    loggingButtonPressedDown = true;
  }
  else if (!loggingButtonPressed() && loggingButtonPressedDown){  // !LOGGING BUTTON UP!
    // Button debounce
    if (millis() - loggingButtonLastPressTime < BUTTON_DEBOUNCE_TIME)
      return;
    loggingButtonLastPressTime = millis();
    
    loggingButtonPressedDown = false;
    toggleDataCollection();
  }
}

// New run button toggles next run or marks a run as bad data
bool newRunButtonPressed(){
  return digitalRead(NEW_RUN_BUTTON) == 1;
}

// Logging button pauses and resumes data collection
bool loggingButtonPressed(){
  return digitalRead(LOGGING_BUTTON) == 1;
}

// Pauses/unpauses the data collection
void toggleDataCollection(){
  dataCollectionPaused = !dataCollectionPaused;
}

// Pauses data collection and starts a new run
void startNewRun(bool markBadData){
  dataCollectionPaused = true;
  
//if (runFile){
    if (markBadData){
      copyingFiles = true;
/*
      File originalFile = runFile;
      String originalFileName = runFile.name();
      String newFileName = originalFileName.substring(0, originalFileName.length() - 4) + "_BAD.csv";
      originalFile.close();
      
      File newFile = SD.open(newFileName, FILE_READ);
      newFile.close();
  
      while (true){
        //String rowData = original.read();
        break;
      }
*/
      drawCopyScreen();
      delay(2000);
  
      copyingFiles = false;
    }
//  else
//    runFile.close();
//}

  runIndex++;
  //runFile = SD.open("Run " + runIndex, FILE_WRITE);

  drawRunScreen();
}

void drawRunScreen(){
  lcd.clear();

  // Draw time
  char timeLabel[9];
  snprintf(timeLabel, 9, "%s:%s:%s", "12", "34", "56");

  lcd.setCursor(8, 0);
  lcd.write(timeLabel);

  // Draw run index
  char runLabel[8];
  snprintf(runLabel, 8, "RUN %d", runIndex);

  lcd.setCursor(0, 0);
  lcd.write(runLabel);

  // Draw logging status
  char *pauseLabel = (dataCollectionPaused) 
    ? "LOGGING OFF" 
    : "LOGGING ON";

  lcd.setCursor(0, 1);
  lcd.write(pauseLabel);
}

void drawCopyScreen(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write("TAGGING BAD DATA");
  lcd.setCursor(0, 1);
  lcd.write("DO N0T TURN OFF!");
}
