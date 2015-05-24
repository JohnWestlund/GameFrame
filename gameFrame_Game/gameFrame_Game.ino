// ./convert.py data/00system/logo data/00system/logo.frm
// ./convert.py data/00system/firework/ data/00system/firework.frm
// for x in data/00system/*.bmp; do y=`echo "$x" | sed -e 's/\.bmp$/.frm/'`; ./convert.py "$x" "$y"; done
#include <SPI.h>
#include <EEPROM.h>
#include <RTClite.h>
#include <SdFat.h>
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

/***************************************************
  BMP parsing code based on example sketch for the Adafruit 
  1.8" SPI display library by Adafruit.
  http://www.adafruit.com/products/358

  "Probably Random" number generator from:
  https://gist.github.com/endolith/2568571
 ****************************************************/
void setup(void);
void testScreen();
void sdErrorMessage();
void yellowDot(byte x, byte y);
void statusLedFlicker();
void loop();
void nextImage();
bool isFrm(const char *filename);
bool drawFrame();
bool bmpOpen(char *filename);
bool bmpInit();
void bmpDraw(char *filename, uint8_t x=0, uint8_t y=0);
byte getIndex(byte x, byte y);
void buttonDebounce();
uint16_t read16(SdFile& f);
uint32_t read32(SdFile& f);
void printFreeRAM();
int freeRam ();
byte rotl(const byte value, int shift);
void wdtSetup();

#define SD_CS    9  // Chip select line for SD card
SdFat sd; // set filesystem
SdFile myFile; // set filesystem

#define BUFFPIXEL 1

// In the SD card, place 24 bit color BMP files (be sure they are 24-bit!)
// There are examples included

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_RGB     Pixels are wired for RGB bitstream
//   NEO_GRB     Pixels are wired for GRB bitstream
//   NEO_KHZ400  400 KHz bitstream (e.g. FLORA pixels)
//   NEO_KHZ800  800 KHz bitstream (e.g. High Density LED strip)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(256, 6, NEO_GRB + NEO_KHZ800);

//Random Number Generator
byte sample = 0;
boolean sample_waiting = false;
byte current_bit = 0;
byte randomResult = 0;

//Button setup
const uint8_t buttonNextPin = 4; // "Next" button
const uint8_t buttonSetupPin = 5;  // "Setup" button

#define STATUS_LED 3

//Enable prints?
const boolean debugMode = true;

//System Setup
boolean
  buttonPressed = false, // control button check
  buttonEnabled = true, // debounce guard
  setupActive = false, // set brightness, playback mode, etc.
  verboseOutput = false, // output extra info to LEDs
  statusLedState = false; // flicker tech
byte
  playMode = 0, // 0 = sequential, 1 = random, 2 = pause animations
  brightness = 4, // LED brightness
  brightnessMultiplier = 10, // DO NOT CHANGE THIS
  fpShield = 0, // button false positive shield
  setupMode = 0, // 0 = brightmess, 1 = play mode, 2 = cycle time
  lowestMem = 250, // storage for lowest number of available bytes
  logoPlayed = 0; // hack for playing logo correctly reardless of playMode
int
  numFolders = 0, // number of folders on sd
  folderIndex = 0, // current folder
  offsetBufferX = 0, // for storing offset when entering menu
  offsetBufferY = 0; // for storing offset when entering menu
unsigned long
  holdTime = 200, // millisecods to hold each .frm frame
  swapTime = 0, // system time to advance to next frame
  baseTime = 0, // system time logged at start of each new image sequence
  buttonTime = 0, // time the last button was pressed (debounce code)
  setupEndTime = 0, // pause animation while in setup mode
  setupEnterTime = 0; // time we enter setup

RTC_DS1307 rtc;
DateTime now;

void setup(void) {
  // debug LED setup
  pinMode(STATUS_LED, OUTPUT);
  analogWrite(STATUS_LED, 100);

  wdtSetup();
 
  pinMode(buttonNextPin, INPUT);    // button as input
  pinMode(buttonSetupPin, INPUT);    // button as input
  digitalWrite(buttonNextPin, HIGH); // turns on pull-up resistor after input
  digitalWrite(buttonSetupPin, HIGH); // turns on pull-up resistor after input
  
  if (debugMode == true)
  {
    Serial.begin(57600);
    printFreeRAM();
  }
  
  // init clock and begin counting seconds
  rtc.begin();
  rtc.adjust(DateTime(2014, 1, 1, 0, 0, 0));

  byte output = 0;

  // load last settings
  // read brightness setting from EEPROM
  output = EEPROM.read(0);
  if (output >= 1 && output <= 7) brightness = output;
  
  // read playMode setting from EEPROM
  output = EEPROM.read(1);
  if (output >= 0 && output <= 2) playMode = output;

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  strip.setBrightness(brightness * brightnessMultiplier);

  // run burn in test if both buttons held on boot
  if ((digitalRead(buttonNextPin) == LOW) && (digitalRead(buttonSetupPin) == LOW))
  {
    // max brightness
    strip.setBrightness(7 * brightnessMultiplier);
    while (true)
    {
      testScreen();
    }
  }

  // revert to these values if setup button held on boot
  if (digitalRead(buttonSetupPin) == LOW)
  {
    brightness = 1;
    strip.setBrightness(brightness * brightnessMultiplier);
    playMode = 0;
  }
  
  // show test screens and folder count if next button held on boot
  if (digitalRead(buttonNextPin) == LOW)
  {
    verboseOutput = true;
    testScreen();
  }

  Serial.print(F("Init SD: "));
  if (!sd.begin(SD_CS, SPI_FULL_SPEED)) {
    Serial.println(F("fail"));
    // SD error message
    sdErrorMessage();
    return;
  }
  Serial.println(F("OK!"));

  char folder[9];
  
  // file indexes appear to loop after 2048
  for (int fileIndex=0; fileIndex<2048; fileIndex++)
  {
    // XXX
    myFile.open(sd.vwd(), fileIndex, O_READ);
    numFolders++;
    myFile.close();
  }
  Serial.print(numFolders);
  Serial.println(F(" folders found."));

  nextImage();
}

void testScreen()
{
  // white
  for (int i=0; i<256; i++)
  {
    strip.setPixelColor(i, strip.Color(255, 255, 255));
  }
  strip.show();
  delay(2000);

  // red
  for (int i=0; i<256; i++)
  {
    strip.setPixelColor(i, strip.Color(255, 0, 0));
  }
  strip.show();
  delay(2000);

  // green
  for (int i=0; i<256; i++)
  {
    strip.setPixelColor(i, strip.Color(0, 255, 0));
  }
  strip.show();
  delay(2000);

  // blue
  for (int i=0; i<256; i++)
  {
    strip.setPixelColor(i, strip.Color(0, 0, 255));
  }
  strip.show();
  delay(2000);
}

void sdErrorMessage()
{
  // red bars
  for (int index=64; index<80; index++)
  {
    strip.setPixelColor(index, strip.Color(255, 0, 0));
  }
  for (int index=80; index<192; index++)
  {
    strip.setPixelColor(index, strip.Color(0, 0, 0));
  }
  for (int index=192; index<208; index++)
  {
    strip.setPixelColor(index, strip.Color(255, 0, 0));
  }
  // S
  yellowDot(7, 6);
  yellowDot(6, 6);
  yellowDot(5, 6);
  yellowDot(4, 7);
  yellowDot(5, 8);
  yellowDot(6, 8);
  yellowDot(7, 9);
  yellowDot(6, 10);
  yellowDot(5, 10);
  yellowDot(4, 10);

  // D
  yellowDot(9, 6);
  yellowDot(10, 6);
  yellowDot(11, 7);
  yellowDot(11, 8);
  yellowDot(11, 9);
  yellowDot(10, 10);
  yellowDot(9, 10);
  yellowDot(9, 7);
  yellowDot(9, 8);
  yellowDot(9, 9);

  strip.setBrightness(brightness * brightnessMultiplier);
  strip.show();
  
  while (true)
  {
    for (int i=255; i>=0; i--)
    {
      analogWrite(STATUS_LED, i);
      delay(1);
    }
    for (int i=0; i<=254; i++)
    {
      analogWrite(STATUS_LED, i);
      delay(1);
    }
  }
}

void yellowDot(byte x, byte y)
{
  strip.setPixelColor(getIndex(x, y), strip.Color(255, 255, 0));
}

void statusLedFlicker()
{
  if (statusLedState == false)
  {
    statusLedState = true;
    analogWrite(STATUS_LED, 254);
  }
  else
  {
    statusLedState = false;
    analogWrite(STATUS_LED, 255);
  }
}

void advanceImage()
{
  // currently playing images?
  if (!setupActive && !myFile.isOpen())
  {
    // If no image is open, look for the next image.
    nextImage();
    return;
  }

  // If it's not time to show the next frame yet, don't do anything.
  if (millis() < swapTime)
    return;

  // Draw the next frame.
  if(drawFrame())
    return;

  Serial.println("finished video");
  // The image is finished playing.  If we're in setup, just loop the image.  Otherwise,
  // look for the next image.
  if(setupActive)
  {
    myFile.seekSet(0);
    bmpInit();
    drawFrame();
  }
  else
  {
    nextImage();
  }
}


void loop()
{
  buttonDebounce();
  now = rtc.now();

  // next button
  if (digitalRead(buttonNextPin) == LOW && buttonPressed == false && buttonEnabled == true)
  {
    buttonPressed = true;
    if (setupActive == false)
    {
      nextImage();
    }
    else
    {
      setupEndTime = millis() + 3000;

      // adjust brightness
      if (setupMode == 0)
      {
        brightness += 1;
        if (brightness > 7) brightness = 1;
        char brightChar[2];
        char brightFile[23];
        strcpy_P(brightFile, PSTR("/00system/bright_"));
        itoa(brightness, brightChar, 10);
        strcat(brightFile, brightChar);
        strcat(brightFile, ".frm");
        strip.setBrightness(brightness * brightnessMultiplier);
        bmpDraw(brightFile, 0, 0);
      }
      
      // adjust play mode
      else if (setupMode == 1)
      {
        playMode++;
        if (playMode > 2) playMode = 0;
        char playChar[2];
        char playFile[21];
        strcpy_P(playFile, PSTR("/00system/play_"));
        itoa(playMode, playChar, 10);
        strcat(playFile, playChar);
        strcat(playFile, ".frm");
        bmpDraw(playFile, 0, 0);
      }
    }
  }

  // setup button
  else if (digitalRead(buttonSetupPin) == LOW && buttonPressed == false && buttonEnabled == true)
  {
    buttonPressed = true;
    setupEndTime = millis() + 3000;
    
    if (setupActive == false)
    {
      setupActive = true;
      setupEnterTime = millis();
      offsetBufferX = 0;
      offsetBufferY = 0;
      if (myFile.isOpen()) myFile.close();
    }
    else
    {
      setupMode++;
      setupMode %= 2;
    }
    if (setupMode == 0)
    {
      char brightChar[2];
      char brightFile[23];
      strcpy_P(brightFile, PSTR("/00system/bright_"));
      itoa(brightness, brightChar, 10);
      strcat(brightFile, brightChar);
      strcat(brightFile, ".frm");
      bmpDraw(brightFile, 0, 0);
    }
    else if (setupMode == 1)
    {
      char playChar[2];
      char playFile[21];
      strcpy_P(playFile, PSTR("/00system/play_"));
      itoa(playMode, playChar, 10);

      strcat(playFile, playChar);
      strcat(playFile, ".frm");
      bmpDraw(playFile, 0, 0);
    }
  }

  if (((digitalRead(buttonSetupPin) == HIGH) && digitalRead(buttonNextPin) == HIGH) && buttonPressed == true)
  {
    buttonPressed = false;
    buttonEnabled = false;
    buttonTime = millis();
  }
  
  // time to exit setup mode?
  if (setupActive == true)
  {
    if (millis() > setupEndTime)
    {
      setupActive = false;
      
      // save any new settings to EEPROM
      if (EEPROM.read(0) != brightness)
      {
        EEPROM.write(0, brightness);
      }
      if (EEPROM.read(1) != playMode)
      {
        EEPROM.write(1, playMode);
      }
      
      // return to brightness setup next time
      setupMode = 0;

      swapTime = swapTime + (millis() - setupEnterTime);
      baseTime = baseTime + (millis() - setupEnterTime);
      if ((holdTime != -1 || playMode != 2))
      {
        nextImage();
      }
    }
  }
  
  advanceImage();
}


void nextImage()
{
  int skip = 0;
  Serial.println(F("---"));
  Serial.println(F("Next Folder..."));
  if (myFile.isOpen()) myFile.close();
  baseTime = millis();
  holdTime = 0;
  char folder[13];
  if (logoPlayed < 2) logoPlayed++;
  
 
    // Getting next folder
    // shuffle playback using "probably_random" code
    // https://gist.github.com/endolith/2568571
    if (playMode != 0) // check we're not in a sequential play mode
    {
      if (sample_waiting == true)
      {
        randomResult = rotl(randomResult, 1); // Spread randomness around
        randomResult ^= sample; // XOR preserves randomness
     
        current_bit++;
        if (current_bit > 7)
        {
          current_bit = 0;
        }
        
        while (randomResult > numFolders)
        {
          randomResult = randomResult - numFolders;
        }
      }
  
      int targetFolder = randomResult;
      
      // don't repeat the same image, please.
      if (targetFolder <= 0 or targetFolder == numFolders or targetFolder == numFolders - 1)
      {
        // Repeat image detected! Incrementing targetFolder.
        targetFolder = targetFolder + 2;
      }
  
      Serial.print(F("Randomly advancing "));
      Serial.print(targetFolder);
      Serial.println(F(" folder(s)."));
      skip = targetFolder;
    }
  
    for(int i = 0; i < numFolders; ++i)
    {
      int idx = (folderIndex + i) % numFolders;
      if(!myFile.open(sd.vwd(), idx, O_READ))
        continue;

      bool isDir = myFile.isDir();
      myFile.getFilename(folder);
      Serial.print(F("Checking file "));
      Serial.println(folder);
      
      // Ignore files that don't end in ".frm".
      if(!isFrm(folder) || isDir || skip--)
      {
        myFile.close();
        continue;
      }

      Serial.print(F("Opening file: "));
      Serial.println(folder);

      // Initialize the image.  The file is already open, so we don't need to close it
      // and reopen it.
      if(!bmpInit()) {
        // Opening the image failed.  Keep looking for a new image.
        continue;
      }

      // We successfully found and opened an image.  Try to draw the first frame.
      Serial.println(F("Drawing first frame"));
      drawFrame();
      Serial.println(F("Drew first frame"));
      break;
    }
}

bool isFrm(const char *filename)
{
  char *ext = strchr(filename, '.');
  if(ext == NULL)
    return false;
  if(ext[1] != 'f' && ext[1] != 'F')
    return false;
  if(ext[2] != 'r' && ext[2] != 'R')
    return false;
  if(ext[3] != 'm' && ext[3] != 'M')
    return false;
  return true;
}

bool bmpOpen(char *filename) {
  myFile.close();

  Serial.print(F("Loading image '"));
  Serial.print(filename);
  Serial.println('\'');

  // Open requested file on SD card
  if (!myFile.open(filename, O_READ)) {
    Serial.println(F("File open failed"));
    sdErrorMessage();
    return false;
  }

  return bmpInit();
}

// Initialize the currently open FRM file.  It must be seeked to the beginning of the file.
bool bmpInit() {
  if (!myFile.isOpen())
    return false;

  uint32_t magic = read32(myFile);
  if(magic != 0x11221212)
  {
    Serial.println(F("File open failed: unrecognized magic"));
    sdErrorMessage();
    return false;
  }

  // Read the file header size.  There is currently no supported header data, so we just skip
  // the file header.
  int fileHeaderSize = read32(myFile);
  myFile.seekCur(fileHeaderSize);

  return true;
}

void bmpDraw(char *filename, uint8_t x, uint8_t y) {
  if(!bmpOpen(filename))
    return;
  
  drawFrame();
}

bool drawFrame()
{
  statusLedFlicker();

  if (!myFile.isOpen())
    return false;

  // Read the header size.
  int frameHeaderSize = read32(myFile);

  // Read the duration of this frame.
  if(frameHeaderSize >= 4)
  {
    holdTime = read32(myFile);
    frameHeaderSize -= 4;
  }
  else
    holdTime = 100;

  // Skip any remaining header data.
  myFile.seekCur(frameHeaderSize);

  // Read the frame.
  uint8_t *p = strip.getPixels();
  int bytesRead = myFile.read(p, 768);
  if(bytesRead < 768)
  {
    Serial.println(F("Closing image"));
    myFile.close();
    return false;
  }

  // Apply brightness.
  int brightnessFactor = (brightness * brightnessMultiplier) + 1;
  for(uint8_t *value = p; value < p + 768; ++value)
      *value = ((*value) * brightnessFactor) >> 8;

  // Adjust the hold time down to compensate for clock drift, such as the issue mentioned below.
  // The value of 8 was found by comparing to an external render at 30 FPS.  It's as accurate as
  // we can get with only 1ms granularity and has some drift, but it's pretty close.
  swapTime = millis() + holdTime - 8;

  // NOTE: strip.show() halts all interrupts, including the system clock.
  // Each call results in about 6825 microseconds lost to the void.
  strip.show();
  return true;
}


byte getIndex(byte x, byte y)
{
  byte index;
  if (y == 0)
  {
    index = 15 - x;
  }
  else if (y % 2 != 0)
  {
    index = y * 16 + x;
  }
  else
  {
    index = (y * 16 + 15) - x;
  }
  return index;
}

void buttonDebounce()
{
  // button debounce -- no false positives
  if (((digitalRead(buttonSetupPin) == HIGH) && digitalRead(buttonNextPin) == HIGH) && buttonPressed == true)
  {
    buttonPressed = false;
    buttonEnabled = false;
    buttonTime = millis();
  }
  if ((buttonEnabled == false) && buttonPressed == false)
  {
    if (millis() > buttonTime + 50) buttonEnabled = true;
  }
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(SdFile& f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(SdFile& f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

// available RAM checker
void printFreeRAM()
{
  Serial.print(F("FreeRam: "));
  Serial.println(freeRam());
  if (freeRam() < lowestMem) lowestMem = freeRam();
  Serial.print(F("Lowest FreeRam: "));
  Serial.println(lowestMem);
}

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

// Random Number Generation ("probably_random")
// https://gist.github.com/endolith/2568571
// Rotate bits to the left
// https://en.wikipedia.org/wiki/Circular_shift#Implementing_circular_shifts
byte rotl(const byte value, int shift) {
  if ((shift &= sizeof(value)*8 - 1) == 0)
    return value;
  return (value << shift) | (value >> (sizeof(value)*8 - shift));
}
 
// Setup of the watchdog timer.
void wdtSetup() {
  cli();
  MCUSR = 0;
  
  /* Start timed sequence */
//  WDTCSR |= _BV(WDCE) | _BV(WDE);
 
  /* Put WDT into interrupt mode */
  /* Set shortest prescaler(time-out) value = 2048 cycles (~16 ms) */
//  WDTCSR = _BV(WDIE);
  WDTCSR = 0;
 
  sei();
}
 
// Watchdog Timer Interrupt Service Routine
ISR(WDT_vect)
{
  sample = TCNT1L; // Ignore higher bits
  sample_waiting = true;
}
