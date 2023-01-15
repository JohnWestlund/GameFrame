Game Frame Source
=========
This Arduino source code is for use with Game Frame, a pixel display available from LEDSEQ.COM.
It compiles under Arduino 1.0.1, and requires the following libraries:

SPI (Distributed with Arduino IDE)
EEPROM (Distributed with Arduino IDE)
RTClite (available in this repository)
IniFileLite (available in this repository)
Adafruit_NeoPixel (Archived in this repo, originally https://github.com/adafruit/Adafruit_NeoPixel)
SdFat (Archived in this repo, originally https://code.google.com/p/sdfatlib/)
FastLED (Archived in this repo, v3.0.1)

Copy `libraries` directory into Sketchbook directory to build.

Board is an Arduino Uno

Clock is the standard firmware. Game builds slightly too large to be useable.

Other resources:
  OSX Video to GameFrame art conversion:
    https://github.com/ghyde/gfx2gf
