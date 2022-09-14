// Libraries -------------------------------------------------------
#include <LiquidCrystal.h>
#include <SPI.h>
#include <SD.h>

// Constants -------------------------------------------------------
#define LOGGING_BUTTON 2
#define NEW_RUN_BUTTON 3
#define WRITING_LED    4
#define SD_PIN         10
#define LCD_RS         22
#define LCD_RW         23
#define LCD_EN         24
#define LCD_DB4        25
#define LCD_DB5        26
#define LCD_DB6        27
#define LCD_DB7        28

// Configuration ---------------------------------------------------
const int BAD_DATA_HOLD_TIME = 2000;
const int BUTTON_DEBOUNCE_TIME = 100;
const char *CODE_VERSION = "0.1.5";

// Variables -------------------------------------------------------
LiquidCrystal lcd(LCD_RS, LCD_RW, LCD_EN, LCD_DB4, LCD_DB5, LCD_DB6, LCD_DB7);
bool errorEncountered = false;
bool dataCollectionPaused = false;
bool copyingFiles = false;
bool newRunButtonIsPressedDown = false;
bool loggingButtonIsPressedDown = false;
int newRunButtonHoldInitialTime = 0;
long lastMillis;
int ticks = 0;
int runIndex = 0;
File runFile;

// Code ------------------------------------------------------------
void setup() {
  // Serial setup
  Serial.begin(9600);

  // LCD setup
  lcd.begin(16, 2);
  lcd.noAutoscroll();

  // SD setup
  //bool sdCardInitialized = SD.begin(SD_PIN);
  bool sdCardInitialized = true;
  if (!sdCardInitialized){
    errorEncountered = true;
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SD ERROR:");
    lcd.setCursor(0, 1);
    lcd.print("NO CARD DETECTED");
    return;
  }

  // Pin setup
  pinMode(WRITING_LED, OUTPUT);
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

  // Get run index from last shut down
  File current = SD.open("/").openNextFile();
  runIndex = 0;

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
  // Bail the loop if an error was encountered
  if (errorEncountered)
    return;
  
  // This clock fixes the issues with millis causing
  // logic errors due to being a long
  if (lastMillis != millis()){
     ticks++;
     lastMillis = millis(); 
  }
  
  // Bail if files are being copied
  if (copyingFiles)
    return;
  
  if (!dataCollectionPaused){
      // ADD FUNCTIONALITY: Collect data
  }
  
  // New run button
  if (newRunButtonPressed() && !newRunButtonIsPressedDown){         
    newRunButtonIsPressedDown = true;
    newRunButtonHoldInitialTime = ticks;
  }
  else if (!newRunButtonPressed() && newRunButtonIsPressedDown){    
    newRunButtonIsPressedDown = false;
    startNewRun(false);
  }

  if (newRunButtonPressed() && newRunButtonIsPressedDown){          
    if (ticks - newRunButtonHoldInitialTime > BAD_DATA_HOLD_TIME){
      startNewRun(true);
      newRunButtonIsPressedDown = false;
    }
  }
  else
    newRunButtonHoldInitialTime = ticks;

  // Logging button
  // IN THE FUTURE, THIS WILL BE DELETED BECAUSE THE LOGGING BUTTON
  // WILL BECOME A PHYSICAL SWITCH
  if (loggingButtonPressed() && !loggingButtonIsPressedDown){
    loggingButtonIsPressedDown = true;
  }
  else if (!loggingButtonPressed() && loggingButtonIsPressedDown){
    loggingButtonIsPressedDown = false;
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
  drawRunScreen();

  // Status LED
  digitalWrite(WRITING_LED, dataCollectionPaused ? LOW : HIGH);
}

void toggleDataCollection(bool set){
  dataCollectionPaused = set;
  drawRunScreen();

    // Status LED
  digitalWrite(WRITING_LED, dataCollectionPaused ? LOW : HIGH);
}

// Pauses data collection and starts a new run
void startNewRun(bool markBadData){
  toggleDataCollection(true);

  if (markBadData){
      copyingFiles = true;

      drawCopyScreen();
      delay(2000);
  
      copyingFiles = false;
  }

  runIndex++;
  drawRunScreen();
}

void drawRunScreen(){
  lcd.clear();

  // Draw time
  char timeLabel[9];
  snprintf(timeLabel, 9, "%s:%s:%s", "12", "34", "56");

  lcd.setCursor(8, 0);
  lcd.print(timeLabel);

  // Draw run index
  char runLabel[8];
  snprintf(runLabel, 8, "RUN %d", runIndex);

  lcd.setCursor(0, 0);
  lcd.print(runLabel);

  // Draw logging status
  char *pauseLabel = (dataCollectionPaused) 
    ? "LOGGING....READY" 
    : "LOGGING..WRITING";

  lcd.setCursor(0, 1);
  lcd.print(pauseLabel);
}

void drawCopyScreen(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TAGGING BAD DATA");
  lcd.setCursor(0, 1);
  lcd.print("DO N0T TURN OFF!");
}
