
/*
 * blinkenclock - multiprupose LED wall clock
 * version 1.0
 * Copyright by Bjoern Knorr 2013
 *
 * https://github.com/jfrede/blinkenclock
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * credits to
 *   Adafruit (NeoPixel Library)
 *
 * 07 Nov 2013 - initial release
 * 04 Aug 2014 - Make Colors mix on overlap
 * 29 Dec 2014 - Remove most delay and replaye RTC Library
 * 30 Dec 2014 - Add Daylight Saving Time
 *
 * */

#include "Adafruit_NeoPixel.h"
#include <Wire.h>
#include "RTClib.h"

// define something
#define LED_PIN 6 // LED strip pin
#define SQ_PIN 7  // Squarewave pin from RTC

Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, LED_PIN, NEO_GRB + NEO_KHZ800);
RTC_DS1307 rtc;

// default mode is clock mode
uint8_t mode = 0;

// alert mode is per default green
uint8_t alert = 0;

// submode
uint8_t submode = 0;

// clock option show five minute dots
uint8_t coptionfivemin = 1;

// clock option invert colors
boolean coptioninvert = 0;

// clock option fade seconds
boolean coptionfade = 1;

// multiprupose counter
int counter = 0;

// Daylight Saving Time
int dst = 0;

// redraw flag
boolean redraw = 1;

// time cache
unsigned long currenttime = 0;

// last second
uint8_t lastsecond = 0;

// strip color (ambient)
uint32_t color_ambient;

// initialize everything
void setup() {
  Serial.begin(9600);
  strip.begin();
  strip.show();
#ifdef AVR
  Wire.begin();
#else
  Wire1.begin(); // Shield I2C pins connect to alt I2C bus on Arduino Due
#endif
  rtc.begin();
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  pinMode(SQ_PIN, INPUT);
  rtc.writeSqwPinMode(SquareWave1HZ);
  DateTime now = rtc.now();
  lastsecond = now.second();
  DaylightSavingTime();
  color_ambient = strip.Color(0, 180, 255);
}

// main loop
void loop() {

  // if enough serial data available, process it
  if (Serial.available()) {
    serialMessage();
  }

  // select mode and show blinken pixelz!
  // show clock
  if (mode == 0) {
    clockMode();
    redraw = 1;
  }

  // demo mode - show rgb cycle
  else if (mode == 1) {


    // reset counter
    if (counter >= 256) {
      counter = 0;
    }

    for (uint16_t i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + counter) & 255));
    }
    redraw = 1;
    counter++;
  }

  else if (mode == 3) {
    lightPixels(color_ambient);
    redraw = 1;
  }

  // redraw if needed
  if (redraw) {
    strip.show();
    redraw = 0;
  }
}

// clock mode
void clockMode() {
  DateTime now = rtc.now();
  uint8_t analoghour = now.hour();
  uint8_t currentsecond = now.second();

  if (((now.hour() == 2) || (now.hour() == 3)) && now.minute() == 0 &&  now.second() == 0) {
    DaylightSavingTime();
  }

  if (analoghour + dst > 12) {
    analoghour = (analoghour + dst - 12);
  }
  analoghour = analoghour * 5 + (now.minute() / 12);

  lightPixels(strip.Color(1, 1, 1));

  if (coptionfivemin) {
    for (uint8_t i = 0; i < 60; i += 5) {
      strip.setPixelColor(i, strip.Color(15, 15, 15));
    }
  }

  strip.setPixelColor(pixelCheck(analoghour - 1), strip.Color(50, 1, 1));
  strip.setPixelColor(pixelCheck(analoghour), strip.Color(255, 1, 1));
  strip.setPixelColor(pixelCheck(analoghour + 1), strip.Color(50, 1, 1));

  int red = 1;
  if (now.minute() == analoghour - 1) {
    red = 50;
  }
  if (now.minute() == analoghour) {
    red = 255;
  }
  if (now.minute() == analoghour + 1) {
    red = 50;
  }
  strip.setPixelColor(now.minute(), strip.Color(red, 1, 255));

  // reset counter
  if (counter > 255) {
    counter = 0;
  }
  else if (lastsecond != currentsecond) {
    lastsecond = now.second();
    counter = 5;
  }

  red = 1 ;                               // Init color variables
  int green = counter;
  int blue = 1;

  if (((now.second() + 1) % 5) == 0) {   // make sure 5 Minute is brighter
    blue = 15;
    red = 15;
    if (( 15 + green ) <= 255 ) {
      green = green + 15;
    }
    else {
      green = 255;
    }
  }
  if ((now.second() + 1) == now.minute()) {     // when second +1  is on minute
    blue = 255;
  }
  if ((now.second() + 1) == analoghour - 1) {   // when second +1  is on hour
    red = 50;
  }
  if ((now.second() + 1) == analoghour) {
    red = 255;
  }
  if ((now.second() + 1) == analoghour + 1) {
    red = 50;
  }

  strip.setPixelColor(pixelCheck(now.second() + 1), strip.Color(red, green, blue));    // update +1 Second on LED stripe

  blue = 1;                             // all color variables back to start
  red = 1;
  green = 255 - counter;

  if ((now.second() % 5) == 0) {        // make sure 5 Minute is brighter
    blue = 15;
    red = 15;
    if (( 15 + green ) <= 255 ) {
      green = green + 15;
    }
    else {
      green = 255;
    }
  }
  if (now.second() == now.minute()) {         // when second +1  is on minute
    blue = 255 ;
  }
  if (now.second() == analoghour - 1) {       // when second +1  is on hour
    red = 50;
  }
  if (now.second() == analoghour) {
    red = 255;
  }
  if (now.second() == analoghour + 1) {
    red = 50;
  }
  strip.setPixelColor(now.second(), strip.Color(red, green, blue));
  if (counter <= 254) {
    counter++;
  }
}

// cycle mode
void drawCycle(int i, uint32_t c) {
  for (uint8_t ii = 5; ii > 0; ii--) {
    strip.setPixelColor(pixelCheck(i - ii), c);
  }
}

// show a progress bar - assuming that the input-value is based on 100%
void progressBar(int i) {
  map(i, 0, 100, 0, 59);
  lightPixels(strip.Color(0, 0, 0));
  for (uint8_t ii = 0; ii < i; ii++) {
    strip.setPixelColor(ii, strip.Color(5, 0, 5));
  }
}

// light all pixels with given values
void lightPixels(uint32_t c) {
  for (uint8_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
  }
}

// set the correct pixels
int pixelCheck(int i) {
  if (i > 59) {
    i = i - 60;
  }
  if (i < 0) {
    i = i + 60;
  }
  return i;
}

void serialMessage() {
  if (Serial.available()) {
    char sw = Serial.read();
    switch (sw) {

      case 'D':   //demo mode (shows rgb cycle)
        {
          mode = 1;
          Serial.println("OK - Demo mode.");
          break;
        }

      case 'C': //clock mode (shows time)
        {
          mode = 0;
          Serial.println("OK - Clock mode.");
          break;
        }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if (WheelPos < 85) {
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  else if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  else {
    WheelPos -= 170;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

/******************************************************************************************/
/***  Determine the Daylight Saving Time DST. In Germany it is the last Sunday in March and in October  ***/
/***  In March at 2:00am the time will be turned forward to 3:00am and in  ***/
/***  October at 3:00am it will be turned back to 2:00am and repeated as 2A and 2B  ***/
/******************************************************************************************/
void DaylightSavingTime() {
  DateTime now = rtc.now();
  /***    Generally checking the full month and determine the DST flag is an easy job  ***/
  if ( now.month() <= 2 || now.month() >= 11) {
    dst = 0;                                   // Winter months
  }
  if (now.month() >= 4 && now.month() <= 9) {
    dst = 1;                                    // Summer months
  }
  if ((now.month() == 3) && (now.day() - now.dayOfWeek() >= 25)) {
    if (now.hour() >= 3 - 1) { // MESZ – 1 hour
      dst = 1;
    }
  }
  /***  Still summer months time DST beginning of October, so easy to determine  ***/
  if (now.month() == 10 && now.day() - now.dayOfWeek() < 25) {
    dst = 1;    // Summer months anyway until 24th of October
  }
  /***  Test the begin of the winter time in October and set DST = 0  ***/
  if (now.month() == 10 && now.day() - now.dayOfWeek() >= 25) {     // Test the begin of the winter time
    if (now.hour() >= 3 - 1) { // -1 since the RTC is running in GMT+1 only
      dst = 0;
    }
    else {
      dst = 1;
    }
  }
}
