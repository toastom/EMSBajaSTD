# EDAS
Eagle Data Aquisition System (name TBD)

### Background
This repo holds the code and other resources for the Georgia Southern Eagle Motorsports Baja team's onboard testing device. This is a student-designed and built device used for on-vehicle testing of the Baja car.

Some of our testing may include
- Shock displacement measurement with string potentiometers
- CVT RPM measurement with hall effect sensors
- A-arm acceleration with accelerometers

### Project Goals and Requirements
 - No laptop or other large computer on the car
 - Easy user experience (UX)
 	- Anyone on the team should be able to plug in a harness and run a test; no programming, wiring, or computer experience needed.
 	- Buttons and switches and pretty LEDs for user interaction and feedback
 - Data logging to an SD card
 	- It should be easy to get your hands on the data. Just take out the SD card and plug it into any computer so you can actually do something useful with it.
 	- Plug the data into Excel or MATLAB to make analyzing and cleaning data easy.
 - Clean and precise data
 	- Some form of software or hardware signal conditioning should exist in the system.
	- If not on the device itself, some post-processing should be done (i.e. via MATLAB) to eliminate noise and return good data.

### Major Parts
- Controller: Arduino Mega 2560 Rev 3
- Display: HD44780 LCD Screen
- Adafruit Data Logger Shield Rev 3
  - Includes SD card module
  - Includes PCF8523 real-time clock (RTC) module

### Features
- Data logging enable/disable
- Button to move to the next run
- Save data with timestamp to a csv file when logging is turned off
- Hardware noise filtering for analog inputs A2-A5
- Various status LEDs

### Project Roadmap
To do list for implementing basic functionality and new features. Mark as done when the feature has been fully implemented.
 - [X] Basic device status display on the LCD screen
 - [ ] Enable/disable data logging state via a toggle switch
 - [ ] Go to the next run via a button press
 - [ ] Log sensor data to a file on the SD card
 - [ ] Mark a previous run as junk or having bad data


### Best practices for Git and version control
 - When making changes to the code, create a new branch based off of `main` and make your changes
 - When finished, open a pull request (PR) to let others review your code and make edits before merging with `main`
 - Commit messages should be short but descriptive
 	- What was changed? Was a new feature added? How? What bugs were fixed?
 - File a new issue for any feature requests, bugs found, or any other ideas for this project
 
### Coding conventions and style
These rules are simply here to enforce a clean and consistent code style between contributors and to keep memory safety in mind. Most of these are fairly common sense or are already part of a list of standard practices.

If you have any additions or modifications to this list, open a PR!

- Variables and function names should use `camelCase`
- Constants should use `CAPS_LOCK`
- Pin assignments should be constants, and they should have a descriptive name for the pin's function
	- Example: `const int LCD_RW = 23;` or `#define LCD_RW 23`
- Pin assignments should match what is defined in the schematic found on Drive
	- The schematic was made in Eagle. The free download can be found [here](https://www.autodesk.com/products/eagle/free-download).
- Avoid the use of `delay()` during regular operation
	- Instead, use a Timer Interrupt or `millis()`
- If possible, avoid the use of recursion
	- A bug in the logic of a recursive function could cause an infinite loop or a stack overflow, resulting in the microcontroller silently hanging when it should be collecting data.
	- [Even NASA's JPL does this](http://spinroot.com/gerard/pdf/P10.pdf)
- Opt for the memory-safe version of a function if possible
	- This is to help avoid buffer overflows or other memory-corruption issues that can be hard to debug
	- Example: Use `strncat(dest, src, count)` over `strcat(dest, src)`
		- `strncat()` is the memory-safe version of `strcat()`, the function that concatenates two C strings together. 
		- The result should be similar between the two, but the former only copies over `count` number of characters from the `src` string, preventing an overflow of the `dest` string length and overwriting random memory beyond it.
	- In short, be careful when playing with memory.








