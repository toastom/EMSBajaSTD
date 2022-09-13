// Libraries -------------------------------------------------------
#include <SPI.h>
#include <SD.h>

// Constants -------------------------------------------------------
#define BUTTON_ONE_PIN 1
#define BUTTON_TWO_PIN 2
#define SD_PIN         4

// Configuration ---------------------------------------------------
const int BAD_DATA_HOLD_TIME = 1000;

// Variables -------------------------------------------------------
bool dataCollectionPaused = false;
bool buttonOneLastStatus = false;
bool buttonTwoLastStatus = false;
bool copyingFiles = false;
int buttonOneHoldElapsed = 0;
int runIndex = 0;
File runFile;

// Code ------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  SD.begin(SD_PIN);

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
  
  if (buttonOneLastStatus){
    if (buttonOneHoldElapsed >= BAD_DATA_HOLD_TIME){
      startNewRun(true);
      return;
    }
    buttonOneHoldElapsed++;
    
    if (!buttonOnePressed())
      startNewRun(false);
  }
  else
    buttonOneHoldElapsed = 0;
  
  buttonOneLastStatus = buttonOnePressed();
  buttonTwoLastStatus = buttonTwoPressed();
}

// Button One toggles next run or marks a run as bad data
bool buttonOnePressed(){
  
}

// Button Two pauses and resumes data collection
bool buttonTwoPressed(){
  
}

// Pauses/unpauses the data collection
void toggleDataCollection(){
  dataCollectionPaused = !dataCollectionPaused;
}

// Pauses data collection and starts a new run
void startNewRun(bool markBadData){
  // Reset variables
  buttonOneLastStatus = false;
  buttonTwoLastStatus = false;
  dataCollectionPaused = true;
  buttonOneHoldElapsed = 0;
  
  if (runFile){
    if (markBadData){
      copyingFiles = true;
      
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
  
      copyingFiles = false;
    }
    else
      runFile.close();
  }

  runIndex++;
  runFile = SD.open("Run " + runIndex, FILE_WRITE);
}
