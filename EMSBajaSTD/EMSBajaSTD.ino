// Libraries -------------------------------------------------------
#include <LiquidCrystal.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>

// Constants -------------------------------------------------------
#define       LOGGING_BUTTON       2
#define       NEW_RUN_BUTTON       3
#define       LOGGING_LED          4
#define       HALT_LED             5
#define       SD_CS                10
#define       LCD_RS               22
#define       LCD_RW               23
#define       LCD_EN               24
#define       LCD_DB4              25
#define       LCD_DB5              26
#define       LCD_DB6              27
#define       LCD_DB7              28

// Configuration ---------------------------------------------------
const String  CODE_VERSION         = "0.4.5";
const String  FILE_EXTENSION       = ".CSV";
const String  TRASH_FOLDER_ADDRESS = "TRASH/";
const String  RUN_FILE_HEADER      = "TIME ms, A";
const int     BAD_DATA_HOLD_TIME   = 2500;
const int     LCD_WIDTH            = 16;
const int     LCD_HEIGHT           = 2;

// Devices ---------------------------------------------------------
LiquidCrystal lcd(LCD_RS, LCD_RW, LCD_EN, LCD_DB4, LCD_DB5, LCD_DB6, LCD_DB7);
RTC_PCF8523   rtc;

// Variables -------------------------------------------------------
unsigned long currentMillis;
unsigned long lastMillis;
unsigned long prevHoldTime;
unsigned long initialMillisecondOfDay;
String        fileAddress;
File          runFile;
bool          copyingFiles;
volatile bool collectingData;
bool          currentRunButtonState;
bool          currentLoggingButtonState;
volatile bool forceScreenDraw;
int           sampleRate;
int           lastSecond;
int           runIndex;

// Code ------------------------------------------------------------
void setup() {
  // Serial setup
  //Serial.begin(9600);

  // Pin setup
  pinMode(LOGGING_LED, OUTPUT);
  pinMode(HALT_LED, OUTPUT);
  pinMode(LOGGING_BUTTON, INPUT);
  pinMode(NEW_RUN_BUTTON, INPUT);
  
  // Start up screen 
  customDrawScreen("HELLO, EMS BAJA", "VERSION " + CODE_VERSION);
  delay(2000);

  // Set the sample rate
  if (digitalRead(NEW_RUN_BUTTON)){
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

  // Cache file address
  DateTime now = rtc.now();

  // Year, Month, Day
  String rtcYearStr = String(now.year());
  rtcYearStr = rtcYearStr.substring(2, 4);
  
  String rtcMonthStr = '0' + String(now.month());
  rtcMonthStr = rtcMonthStr.substring(rtcMonthStr.length() - 2, 3);

  String rtcDayStr = '0' + String(now.day());
  rtcDayStr = rtcDayStr.substring(rtcDayStr.length() - 2, 3);

  // Hour, Minute, Second
  int rtcHour = now.hour();
  int rtcMinute = now.minute();
  int rtcSecond = now.second();
  
  String rtcHourStr = '0' + String(rtcHour);
  rtcHourStr = rtcHourStr.substring(rtcHourStr.length() - 2, 3);
  
  String rtcMinuteStr = '0' + String(rtcMinute);
  rtcMinuteStr = rtcMinuteStr.substring(rtcMinuteStr.length() - 2, 3);

  String rtcSecondStr = '0' + String(rtcSecond);
  rtcSecondStr = rtcSecondStr.substring(rtcSecondStr.length() - 2, 3);
  
  fileAddress = rtcMonthStr + '-' + rtcDayStr + '-' + rtcYearStr + '/' + rtcHourStr + '-' + rtcMinuteStr + '-' + rtcSecondStr + '/';

  // Cache exact millisecond of the day
  initialMillisecondOfDay = (3600000 * rtcHour) + (60000 * rtcMinute) + (1000 * rtcSecond);

  collectingData = false;
  forceScreenDraw = false;

  // Start the first run
  startNewRun(false);
}

void loop() {
  currentMillis = millis();
  
  // Update clock every second
  if (lastSecond != currentMillis / 1000 || forceScreenDraw){
    lastSecond = currentMillis / 1000;
    forceScreenDraw = false;
    drawRunScreen();
  }

  // Switches are active low
  bool isRunButtonPressed = !digitalRead(NEW_RUN_BUTTON);
  bool isLoggingButtonPressed = !digitalRead(LOGGING_BUTTON);
  
  // Next run button
  if (isRunButtonPressed && !currentRunButtonState){
    currentRunButtonState = true;
    prevHoldTime = millis();
  }
  else if (!isRunButtonPressed && currentRunButtonState && !collectingData){
    currentRunButtonState = false;
    runIndex++;
    startNewRun(false);
  }

  // Hold the run button to clear the last run
  if (isRunButtonPressed && currentRunButtonState){
    if ((currentMillis - prevHoldTime) > BAD_DATA_HOLD_TIME){
      startNewRun(true);
      currentRunButtonState = false;
    }
  }

  // Logging button
  if (isLoggingButtonPressed && !currentLoggingButtonState){
    currentLoggingButtonState = true;
  }
  else if (!isLoggingButtonPressed && currentLoggingButtonState){
    currentLoggingButtonState = false;

    if(collectingData){
      stopDataCollection();
      collectingData = false;
    }
    else{
      startDataCollection();
      collectingData = true;
    }

    digitalWrite(LOGGING_LED, collectingData);
    forceScreenDraw = true;
  }

  // Write data to sd card
  if (collectingData && runFile && !copyingFiles){
    if (currentMillis - lastMillis <= sampleRate)
      return;

    runFile.print(initialMillisecondOfDay + currentMillis);
    runFile.print(',');
    runFile.println(analogRead(A2));
    
    lastMillis = currentMillis;
  }
}

void startDataCollection(){
  // Verify an sd card is mounted
  if (!sdCardMounted()){
    customDrawScreen("SD ERROR:", "NO CARD DETECTED");
    digitalWrite(HALT_LED, HIGH);
    while(!sdCardMounted());
    digitalWrite(HALT_LED, LOW);
  }

  // Ensure the file address exists
  if (!SD.exists(fileAddress))
      SD.mkdir(fileAddress);

  // Open/create run file
  runFile = SD.open(fileAddress + "RUN" + String(runIndex) + FILE_EXTENSION, FILE_WRITE);
  
  // Write headers if no data is present in the file
  if (runFile.peek() == -1)
    runFile.println(RUN_FILE_HEADER);
}

void stopDataCollection(){
  // Verify an sd card is mounted
  if (!sdCardMounted()){
    customDrawScreen("SD ERROR:", "NO CARD DETECTED");
    digitalWrite(HALT_LED, HIGH);
    while(!sdCardMounted());
    digitalWrite(HALT_LED, LOW);
  }

  // Close the run file if one exists
  if (runFile)
    runFile.close();
}

void startNewRun(bool trashLastRun){
  // Stop collecting data
  stopDataCollection();

  if (trashLastRun){
    customDrawScreen("TRASHING DATA", "PLEASE WAIT...");
    digitalWrite(HALT_LED, HIGH);
    copyingFiles = true;

    // Re-open run file in read mode
    String currentRunFileAddress = fileAddress + "RUN" + String(runIndex) + FILE_EXTENSION;
    File currentRunFile = SD.open(currentRunFileAddress, FILE_READ);

    // Create trash folder if not created already
    if (!SD.exists(TRASH_FOLDER_ADDRESS + fileAddress))
      SD.mkdir(TRASH_FOLDER_ADDRESS + fileAddress);

    // Create new duplicate run file in trash folder
    String duplicateRunFileAddress = TRASH_FOLDER_ADDRESS + currentRunFileAddress;
    File duplicateRunFile = SD.open(duplicateRunFileAddress, FILE_WRITE);

    // Copy and paste data from original file to duplicate
    size_t data;
    uint8_t buf[64];
    while ((data = currentRunFile.read(buf, sizeof(buf))) > 0)
      duplicateRunFile.write(buf, data);
    
    // Close all files
    currentRunFile.close();
    duplicateRunFile.close();
    SD.remove(currentRunFileAddress);

    digitalWrite(HALT_LED, LOW);
    copyingFiles = false;
  }

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
  return SD.begin(SD_CS);
}