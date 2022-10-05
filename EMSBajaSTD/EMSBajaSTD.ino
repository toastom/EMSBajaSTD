#include <LiquidCrystal.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>

// Constants -------------------------------------------------------
#define       LOGGING_BUTTON       2
#define       NEW_RUN_BUTTON       3
#define       LOGGING_LED          4
#define       HALT_LED             5
#define       SD_SC                10
#define       LCD_RS               22
#define       LCD_RW               23
#define       LCD_EN               24
#define       LCD_DB4              25
#define       LCD_DB5              26
#define       LCD_DB6              27
#define       LCD_DB7              28

// Configuration ---------------------------------------------------
const String  CODE_VERSION         = "0.4.2";
const String  FILE_EXTENSION       = ".CSV";
const String  TRASH_FOLDER_ADDRESS = "TRASH/";
const String  RUN_FILE_HEADER      = "TIME ms, A";
const int     BAD_DATA_HOLD_TIME   = 2000;
const int     BUTTON_DEBOUNCE_TIME = 100;
const int     LCD_WIDTH            = 16;
const int     LCD_HEIGHT           = 2;

// Devices ---------------------------------------------------------
LiquidCrystal lcd(LCD_RS, LCD_RW, LCD_EN, LCD_DB4, LCD_DB5, LCD_DB6, LCD_DB7);
RTC_PCF8523   rtc;

// Variables -------------------------------------------------------
unsigned long currentMillis;
unsigned long lastMillis;
unsigned long newRunButtonHoldInitialTime;
unsigned long initialMillisecondOfDay;
String        fileAddress;
File          runFile;
bool          copyingFiles;
bool          collectingData;
bool          newRunButtonIsPressedDown;
bool          loggingButtonIsPressedDown;
int           sampleRate;
int           lastSecond;
int           runIndex;

// Code ------------------------------------------------------------
void setup() {
  // Serial setup
  // Serial.begin(9600);

  // Pin setup
  pinMode(LOGGING_LED, OUTPUT);
  pinMode(HALT_LED, OUTPUT);
  pinMode(LOGGING_BUTTON, INPUT);
  pinMode(NEW_RUN_BUTTON, INPUT);
  
  // Start up screen 
  customDrawScreen("HELLO, EMS BAJA", "VERSION " + CODE_VERSION);
  delay(2000);

  // Set the sample rate
  if (!digitalRead(NEW_RUN_BUTTON)){
    sampleRate = 1;
    customDrawScreen("SAMPLE RATE:", "1000/SECOND..1MS");
  }
  else{
    sampleRate = 10;
    customDrawScreen("SAMPLE RATE:", "100/SECOND..10MS");
  }
  delay(2000);

  // SD initialization
  if (!sdCardMounted()){
    customDrawScreen("SD ERROR:", "NO CARD DETECTED");
    digitalWrite(HALT_LED, HIGH);
    while(!sdCardMounted());
    digitalWrite(HALT_LED, LOW);
  }

  // RTC initialization
  if (!rtc.begin()){
    customDrawScreen("RTC ERROR:", "NO RTC DETECTED");
    digitalWrite(HALT_LED, HIGH);
    while(!rtc.begin());
    digitalWrite(HALT_LED, LOW);
  }

  if (rtc.lostPower()){
    customDrawScreen("RTC ERROR:", "RTC LOST POWER");

    // Set the rtc time to when the code was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    rtc.start();

    digitalWrite(HALT_LED, HIGH);
    delay(2000);
    digitalWrite(HALT_LED, LOW);
  }

  // Get current time
  DateTime now = rtc.now();

  String rtcYearStr = String(now.year());
  rtcYearStr = rtcYearStr.substring(2, 4);
  
  String rtcMonthStr = '0' + String(now.month());
  rtcMonthStr = rtcMonthStr.substring(rtcMonthStr.length() - 2, 3);

  String rtcDayStr = '0' + String(now.day());
  rtcDayStr = rtcDayStr.substring(rtcDayStr.length() - 2, 3);

  int rtcHour = now.hour();
  int rtcMinute = now.minute();
  int rtcSecond = now.second();
  
  String rtcHourStr = '0' + String(rtcHour);
  rtcHourStr = rtcHourStr.substring(rtcHourStr.length() - 2, 3);
  
  String rtcMinuteStr = '0' + String(rtcMinute);
  rtcMinuteStr = rtcMinuteStr.substring(rtcMinuteStr.length() - 2, 3);

  String rtcSecondStr = '0' + String(rtcSecond);
  rtcSecondStr = rtcSecondStr.substring(rtcSecondStr.length() - 2, 3);
  
  // Cache file address
  fileAddress = rtcMonthStr + '-' + rtcDayStr + '-' + rtcYearStr + '/' + rtcHourStr + '-' + rtcMinuteStr + '-' + rtcSecondStr + '/';

  // Cache exact millisecond of the day
  initialMillisecondOfDay = (3600000 * rtcHour) + (60000 * rtcMinute) + (1000 * rtcSecond);

  // Start the first run
  startNewRun(false);
}

void loop() {
  currentMillis = millis();
  if (currentMillis - lastMillis < sampleRate || copyingFiles)
    return;
  
  if (collectingData && runFile){
    runFile.print(initialMillisecondOfDay + currentMillis);
    runFile.print(',');
    runFile.println(analogRead(A0));
    
    lastMillis = currentMillis;
  }

  bool newRunButtonPressed = digitalRead(NEW_RUN_BUTTON);
  bool loggingButtonPressed = digitalRead(LOGGING_BUTTON);

  // New run button
  if (newRunButtonPressed && !newRunButtonIsPressedDown){         
    newRunButtonIsPressedDown = true;
    newRunButtonHoldInitialTime = currentMillis;
  }
  else if (!newRunButtonPressed && newRunButtonIsPressedDown){    
    newRunButtonIsPressedDown = false;
    startNewRun(false);
  }

  if (newRunButtonPressed && newRunButtonIsPressedDown){          
    if (currentMillis - newRunButtonHoldInitialTime > BAD_DATA_HOLD_TIME){
      startNewRun(true);
      newRunButtonIsPressedDown = false;
    }
  }

  // Logging button
  if (loggingButtonPressed && !loggingButtonIsPressedDown)
    loggingButtonIsPressedDown = true;
  else if (!loggingButtonPressed && loggingButtonIsPressedDown){
    loggingButtonIsPressedDown = false;
    
    if (!collectingData)
      startDataCollection();
    else
      stopDataCollection();
  }

  if (lastSecond != currentMillis / 1000){
    lastSecond = currentMillis / 1000;
    drawRunScreen();
  }
}

void startDataCollection(){
  if (collectingData)
    return;

  // Verify an sd card is mounted
  if (!sdCardMounted()){
    customDrawScreen("SD ERROR:", "NO CARD DETECTED");
    digitalWrite(HALT_LED, HIGH);
    while(!sdCardMounted());
    digitalWrite(HALT_LED, LOW);
  }

  if (!SD.exists(fileAddress))
      SD.mkdir(fileAddress);

  runFile = SD.open(fileAddress + "RUN" + String(runIndex) + FILE_EXTENSION, FILE_WRITE);
  
  // Write headers if no data is present in the file
  if (runFile.peek() == -1)
    runFile.println(RUN_FILE_HEADER);
  
  collectingData = true;

  // Refresh screen and status LED
  drawRunScreen();
  digitalWrite(LOGGING_LED, HIGH);
}

void stopDataCollection(){
  if (!collectingData)
    return;

  // Verify an sd card is mounted
  if (!sdCardMounted()){
    customDrawScreen("SD ERROR:", "NO CARD DETECTED");
    digitalWrite(HALT_LED, HIGH);
    while(!sdCardMounted());
    digitalWrite(HALT_LED, LOW);
  }

  if (runFile)
    runFile.close();
  
  collectingData = false;

  // Refresh screen and status LED
  drawRunScreen();
  digitalWrite(LOGGING_LED, LOW);
}

void startNewRun(bool trashLastRun){
  // Verify an sd card is mounted
  if (!sdCardMounted()){
    customDrawScreen("SD ERROR:", "NO CARD DETECTED");
    digitalWrite(HALT_LED, HIGH);
    while(!sdCardMounted());
    digitalWrite(HALT_LED, LOW);
  }

  stopDataCollection();

  if (trashLastRun){
    customDrawScreen("TRASHING DATA", "PLEASE WAIT...");
    digitalWrite(HALT_LED, HIGH);
    copyingFiles = true;

    // Create trash folder if not created already
    if (!SD.exists(TRASH_FOLDER_ADDRESS + fileAddress))
      SD.mkdir(TRASH_FOLDER_ADDRESS + fileAddress);

    String currentRunFileAddress = fileAddress + "RUN" + String(runIndex) + FILE_EXTENSION;
    String duplicateRunFileAddress = TRASH_FOLDER_ADDRESS + currentRunFileAddress;
    
    File currentRunFile = SD.open(currentRunFileAddress, FILE_READ);
    File duplicateRunFile = SD.open(duplicateRunFileAddress, FILE_WRITE);

    size_t data;
    uint8_t buf[64];
    while ((data = currentRunFile.read(buf, sizeof(buf))) > 0) 
      duplicateRunFile.write(buf, data);
    
    currentRunFile.close();
    duplicateRunFile.close();
    SD.remove(currentRunFileAddress);

    digitalWrite(HALT_LED, LOW);
    copyingFiles = false;
  }

  runIndex++;
  drawRunScreen();
}

void drawRunScreen(){
  lcd.begin(LCD_WIDTH, LCD_HEIGHT);

  // Draw run index
  lcd.setCursor(0, 0);
  lcd.print("RUN " + String(runIndex));

  // Draw time
  DateTime now = rtc.now();

  String rtcHour = '0' + String(now.hour());
  rtcHour = rtcHour.substring(rtcHour.length() - 2, 3);
  
  String rtcMinute = '0' + String(now.minute());
  rtcMinute = rtcMinute.substring(rtcMinute.length() - 2, 3);

  String rtcSecond = '0' + String(now.second());
  rtcSecond = rtcSecond.substring(rtcSecond.length() - 2, 3);
  
  lcd.setCursor(8, 0);
  lcd.print(rtcHour + ':' + rtcMinute + ':' + rtcSecond);

  // Draw logging status
  lcd.setCursor(0, 1);
  lcd.print((collectingData) ? "LOGGING..WRITING" : "LOGGING....READY");
}

void customDrawScreen(String top, String bottom){
  lcd.begin(LCD_WIDTH, LCD_HEIGHT);
  lcd.setCursor(0, 0);
  lcd.print(top);
  lcd.setCursor(0, 1);
  lcd.print(bottom);
}

bool sdCardMounted(){
  return SD.begin(SD_SC);
  customDrawScreen("SD ERROR:", "NO CARD DETECTED");
  while(SD.begin(SD_SC));
}