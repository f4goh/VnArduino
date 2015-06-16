#VNA software with an Arduino #
F4GOH Anthony f4goh@orange.fr <br>

June 2015

  Hardware :
- Arduino or Nano
- AD9851
- AD8302

All schematics soon on https://hamprojects.wordpress.com/ 

Use this program freely with Arduino 1.0.6

## Installation ##
To use the VnArduino program:  
- Go to https://github.com/f4goh/VnArduino, click the [Download ZIP](https://github.com/f4goh/VnArduino/archive/master.zip) button and save the ZIP file to a convenient location on your PC.
- Uncompress the downloaded file.  This will result in a folder containing the program, that has a name that includes the branch name, usually VnArduino-master.
- Rename the folder to  VnArduino.
- Copy the renamed folder to the Arduino sketchbook\.

- you must add Arduino SPI library for AD9850 : F4GOJ Christophe f4goj@free.fr <br>
  Go to https://github.com/F4GOJ/AD9850SPI

## Usage notes ##


To use 4x20 characters LCD Display, the LiquidCrystal_I2C and WIRE libraries must also be included.


```c++
#include <avr/pgmspace.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <AD9850SPI.h>
#include <SPI.h>
#include <EEPROM.h>
```
