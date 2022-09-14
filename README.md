# EDAS
Eagle Data Aquisition System (name TBD)

## Background
This repo holds the code and other resources for the Georgia Southern Eagle Motorsports Baja team's onboard testing device.
This is a student-designed and built device used for on-vehicle testing of various sensors.

Some of our testing may include
- Shock displacement measurement using string potentiometers
- CVT RPM measurement using hall effect sensors
- A-arm acceleration from accelerometers

## Major Parts
- Controller: Arduino Mega 2560 Rev 3
- Display: HD44780 LCD Screen
- Adafruit Data Logger Shield Rev 3
  - Includes SD card module
  - Includes PCF8523 real-time clock (RTC) module

## Features
- Data logging enable/disable
- Button to move to the next run
- Save data to a csv file with timestamp when logging is turned off
- Hardware noise filtering for analog inputs A2-A5
- Various status LEDs



