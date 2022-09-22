// Libraries -------------------------------------------------------
#include <LiquidCrystal.h>
#include <RTClib.h>
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
const String  CODE_VERSION = "0.3.0";
const String  FILE_EXTENSION = ".CSV";
const int     BAD_DATA_HOLD_TIME = 2000;
const int     BUTTON_DEBOUNCE_TIME = 100;

// Devices ---------------------------------------------------------
LiquidCrystal lcd(LCD_RS, LCD_RW, LCD_EN, LCD_DB4, LCD_DB5, LCD_DB6, LCD_DB7);
RTC_PCF8523   rtc;

// Variables -------------------------------------------------------
unsigned long currentMillis;
unsigned long lastMillis;
unsigned long newRunButtonHoldInitialTime;
String        fileAddress;
bool          copyingFiles;
bool          collectingData;
bool          newRunButtonIsPressedDown;
bool          loggingButtonIsPressedDown;
bool          criticalErrorDetected;
File          runFile;
int           sampleRate = 1;
int           runIndex;
int           lastSecond;

// Code ------------------------------------------------------------
void setup() {
  // Serial setup
  Serial.begin(9600);

  // Pin setup
  pinMode(WRITING_LED, OUTPUT);
  pinMode(LOGGING_BUTTON, INPUT);
  pinMode(NEW_RUN_BUTTON, INPUT);
  
  // LCD setup
  lcd.begin(16, 2);
  lcd.noAutoscroll();

  // Start up screen 
  customDrawScreen("HELLO, BAJA", "VERSION " + String(CODE_VERSION));
  delay(2000);

  // Set the sample rate
  if (!newRunButtonPressed()){
    sampleRate = 1;
    customDrawScreen("SAMPLE RATE:", "1000/SECOND..1MS");
  }
  else{
    sampleRate = 10;
    customDrawScreen("SAMPLE RATE:", "100/SECOND..10MS");
  }
  delay(2000);

  // SD initialization
  bool sdCardInitialized = SD.begin(SD_PIN);
  if (!sdCardInitialized){
    customDrawScreen("SD ERROR:", "NO CARD DETECTED");

    // Wait for SD to initialize
    while(!SD.begin(SD_PIN));
  }

  // RTC initialization
  bool rtcInitialized = rtc.begin();
  if (!rtcInitialized){
    customDrawScreen("RTC ERROR:", "NO RTC DETECTED");

    // Wait for RTC to initialize
    while(!rtc.begin());
  }

  if (!rtc.initialized() || rtc.lostPower()){
    customDrawScreen("RTC ERROR:", "RTC LOST POWER");

    // Set the rtc time to when the code was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    rtc.start();

    delay(2000);
  }

  // Cache file address
  fetchAndCacheFileAddress();

  // Start the first run
  startNewRun(false);
}

void loop() {
  currentMillis = millis();
  if (currentMillis - lastMillis < sampleRate || copyingFiles)
    return;
  
  if (collectingData && runFile){
    runFile.print(currentMillis / 1000);
    runFile.print(",");
    
    // Temporary "sensor" -------------
    runFile.println(analogRead(A0));
    // --------------------------------
    
    lastMillis = currentMillis;
  }

  // New run button
  if (newRunButtonPressed() && !newRunButtonIsPressedDown){         
    newRunButtonIsPressedDown = true;
    newRunButtonHoldInitialTime = currentMillis;
  }
  else if (!newRunButtonPressed() && newRunButtonIsPressedDown){    
    newRunButtonIsPressedDown = false;
    startNewRun(false);
  }

  if (newRunButtonPressed() && newRunButtonIsPressedDown){          
    if (currentMillis - newRunButtonHoldInitialTime > BAD_DATA_HOLD_TIME){
      startNewRun(true);
      newRunButtonIsPressedDown = false;
    }
  }

  // Logging button
  if (loggingButtonPressed() && !loggingButtonIsPressedDown){
    loggingButtonIsPressedDown = true;
  }
  else if (!loggingButtonPressed() && loggingButtonIsPressedDown){
    loggingButtonIsPressedDown = false;
    toggleDataCollection();
  }

  if (lastSecond != currentMillis / 1000){
    lastSecond = currentMillis / 1000;
    drawRunScreen();
  }
}

bool newRunButtonPressed(){
  return digitalRead(NEW_RUN_BUTTON) == 1;
}

bool loggingButtonPressed(){
  return digitalRead(LOGGING_BUTTON) == 1;
}

void toggleDataCollection(){
  if (!collectingData){
    if (!SD.exists(fileAddress))
      SD.mkdir(fileAddress);
    
    runFile = SD.open(fileAddress + "RUN" + String(runIndex) + FILE_EXTENSION, FILE_WRITE);
    
    // Write headers if no data is present in the file
    if (runFile.peek() == -1)
      runFile.println("Seconds,String Pot");
  }
  else
    runFile.close();

  collectingData = !collectingData;

  // Refresh screen and status LED
  drawRunScreen();
  digitalWrite(WRITING_LED, collectingData ? HIGH : LOW);
}

void toggleDataCollection(bool set){
  if (set){
    if (!SD.exists(fileAddress))
      SD.mkdir(fileAddress);
    
    runFile = SD.open(fileAddress + "RUN" + String(runIndex) + FILE_EXTENSION, FILE_WRITE);
    
    // Write headers if no data is present in the file
    if (runFile.peek() == -1)
      runFile.println("Seconds,String Pot");
  }
  else
    runFile.close();
  
  collectingData = set;

  // Refresh screen and status LED
  drawRunScreen();
  digitalWrite(WRITING_LED, collectingData ? HIGH : LOW);
}

void startNewRun(bool markBadData){
  toggleDataCollection(false);

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
  DateTime now = rtc.now();

  String rtcHour = "0" + String(now.hour());
  rtcHour = rtcHour.substring(rtcHour.length() - 2, 3);
  
  String rtcMinute = "0" + String(now.minute());
  rtcMinute = rtcMinute.substring(rtcMinute.length() - 2, 3);

  String rtcSecond = "0" + String(now.second());
  rtcSecond = rtcSecond.substring(rtcSecond.length() - 2, 3);
  
  lcd.setCursor(8, 0);
  lcd.print(rtcHour + ':' + rtcMinute + ':' + rtcSecond);

  // Draw run index
  char runLabel[8];
  snprintf(runLabel, 8, "RUN %d", runIndex);

  lcd.setCursor(0, 0);
  lcd.print(runLabel);

  // Draw logging status
  char *pauseLabel = (collectingData) 
    ? "LOGGING..WRITING" 
    : "LOGGING....READY";

  lcd.setCursor(0, 1);
  lcd.print(pauseLabel);
}

void drawCopyScreen(){
  customDrawScreen("TRASHING DATA", "PLEASE WAIT...");
}

void customDrawScreen(String top){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(top);
}

void customDrawScreen(String top, String bottom){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(top);
  lcd.setCursor(0, 1);
  lcd.print(bottom);
}

void fetchAndCacheFileAddress(){
  DateTime now = rtc.now();

  // Year, Month, Day
  String rtcYear = String(now.year());
  rtcYear = rtcYear.substring(2, 4);
  
  String rtcMonth = '0' + String(now.month());
  rtcMonth = rtcMonth.substring(rtcMonth.length() - 2, 3);

  String rtcDay = '0' + String(now.day());
  rtcDay = rtcDay.substring(rtcDay.length() - 2, 3);

  // Hour, Minute, Second
  String rtcHour = '0' + String(now.hour());
  rtcHour = rtcHour.substring(rtcHour.length() - 2, 3);
  
  String rtcMinute = '0' + String(now.minute());
  rtcMinute = rtcMinute.substring(rtcMinute.length() - 2, 3);

  String rtcSecond = '0' + String(now.second());
  rtcSecond = rtcSecond.substring(rtcSecond.length() - 2, 3);
  
  fileAddress = rtcDay + '_' + rtcMonth + '_' + rtcYear + '/' + rtcHour + '_' + rtcMinute + '_' + rtcSecond + '/';
}
