
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
 *   Adafruit (NeoPixel + RTC Library)
 *
 * 07 Nov 2013 - initial release
 * 04 Aug 2014 - Make Colors mix on overlap
 * 29 Dec 2014 - Remove most delay and replaye RTC Library
 * 30 Dec 2014 - Add Daylight Saving Time
 * 16 Jan 2015 - Use SQ Pin on RTC to detect new second
 * 16 Jan 2015 - Fix some Problems calculating with unit8 valus
 * 02 Feb 2015 - Final check mixing around 0

 * todo: Fancy animation for Noon and Midnight
 *       Make set RTC working on non initiated RTC
 *
 * */

#include "Adafruit_NeoPixel.h"
#include <Wire.h>
#include "RTClib.h"

// define something
#define LED_PIN 6 // LED strip pin
#define SQ_PIN 7  // Squarewave pin from RTC

// init LED Stripe
Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, LED_PIN, NEO_GRB + NEO_KHZ800);

// init RTC
RTC_DS1307 rtc;

// default mode is clock mode
uint8_t mode = 0;

//  fader_count
uint8_t fader_count = 0;

// Daylight Saving Time
uint8_t  dst = 0;

// Init RGB Variables
uint8_t red = 0;
uint8_t green = 0;
uint8_t blue = 0;

// init Clock Variables
uint8_t analoghour = 0;
uint8_t lastanaloghour = 0;
uint8_t nextanaloghour = 0;
uint8_t analogminute = 0;
uint8_t analogsecond = 0;
uint8_t lastanalogsecond = 0;

// redraw flag
boolean redraw = 1;

// last second
uint8_t lastsecond = 0;

// strip color (ambient)
uint32_t color_ambient;

// sqwarewave State
boolean sqState = 0;
boolean lastState = 1;

// Init global Date
DateTime now;

// initialize everything
void setup() {
  Serial.begin(9600);
  strip.begin();
  strip.setBrightness(200);       // Dimmer 
  strip.show();
  Wire.begin();
  rtc.begin();
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));    //FIXME This seems not to work
  }
  pinMode(SQ_PIN, INPUT);
  rtc.writeSqwPinMode(SquareWave1HZ);   // Set SQ to 1HZ
  now = rtc.now();
  DaylightSavingTime();                 // have we DaylightSavingTime?
  pinMode(SQ_PIN, INPUT);          

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

    for (uint16_t i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + fader_count) & 255));
    }
    redraw = 1;
    fader_count++;
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

  sqState = digitalRead(SQ_PIN);

  if ((sqState == 0) && (lastState == 1)) {
    fader_count = 0;
    now = rtc.now();
    analoghour = now.hour();
    analogminute = now.minute();
    analogsecond = now.second();
    lastanalogsecond = pixelCheck(analogsecond - 1);

    if (((analoghour == 2) || (analoghour == 3)) && analogminute == 0 &&  analogsecond == 0) {
      DaylightSavingTime();
    }

    if ((analoghour + dst) > 12) {
      analoghour = ((analoghour + dst) % 12);
    }

    analoghour = analoghour * 5 + (analogminute / 12);
    
    lastanaloghour = pixelCheck(analoghour - 1);
    nextanaloghour = pixelCheck(analoghour + 1);
    
    lastState = 0;
  }
  else if (sqState == 1) {
    lastState = 1;
  }

  if (fader_count < 255) {
    fader_count++;
  }
  else {
    return;
  }

  if (analogminute == 0 && analogsecond == 0) {     // every Hour blink red
    if ( fader_count <= 127 ) {
      red = fader_count;
    }
    else {
      red = 128 - (fader_count - 127);
    }

    lightPixels(strip.Color(red, 0, 0));
    for (uint8_t i = 0; i < 60; i += 5) {
      strip.setPixelColor(i, strip.Color((15 + red), 15, 15));
    }
  }
  else {
    lightPixels(strip.Color(0, 0, 0));
    for (uint8_t i = 0; i < 60; i += 5) {
      strip.setPixelColor(i, strip.Color(15, 15, 15));
    }
  }

  strip.setPixelColor(lastanaloghour, strip.Color(50, 0, 0));
  strip.setPixelColor(analoghour, strip.Color(255, 0, 0));
  strip.setPixelColor(nextanaloghour, strip.Color(50, 0, 0));

  red = 0;

  if (analogminute == lastanaloghour) {
    red = 50;
  }
  if (analogminute == analoghour) {
    red = 255;
  }
  if (analogminute == nextanaloghour) {
    red = 50;
  }

  strip.setPixelColor(analogminute, strip.Color(red, 0, 255));

  red = 0;                               // Init color variables
  green = fader_count;
  blue = 0;

  if ((now.second() % 5) == 0) {   // make sure 5 Minute is brighter
    blue = 15;
    red = 15;
    if (( 15 + green ) <= 255 ) {
      green = green + 15;
    }
    else {
      green = 255;
    }
  }
  if (analogsecond == analogminute) {     // when second is on minute
    blue = 255;
  }
  if (analogsecond == lastanaloghour) {   // when second is on hour
    red = 50;
  }
  if (analogsecond == analoghour) {
    red = 255;
  }
  if (analogsecond == nextanaloghour) {
    red = 50;
  }

  strip.setPixelColor(analogsecond, strip.Color(red, green, blue));    // update Second on LED stripe

  blue = 0;                             // all color variables back to start
  red = 0;
  green = 255 - fader_count;

  if (((analogsecond - 1)  % 5) == 0) {        // make sure 5 Minute is brighter
    blue = 15;
    red = 15;
    if (( 15 + green ) <= 255 ) {
      green = green + 15;
    }
    else {
      green = 255;
    }
  }
  if (lastanalogsecond == analogminute) {         // when second -1  is on minute
    blue = 255 ;
  }
  if (lastanalogsecond == lastanaloghour) {       // when second -1  is on hour
    red = 50;
  }
  if (lastanalogsecond == analoghour) {
    red = 255;
  }
  if (lastanalogsecond == nextanaloghour) {
    red = 50;
  }

  strip.setPixelColor(lastanalogsecond, strip.Color(red, green, blue));  //update lastSecond on LED stripe
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
uint8_t pixelCheck(uint8_t i) {
  if (i >= 200) {
    i = 60 - (256 - i) ; 
  }
  if (i >= 60) {
    i = i - 60;
  }
  return i;
}

void serialMessage() {
  if (Serial.available()) {
    char sw = Serial.read();
    switch (sw) {
      //demo mode (shows rgb cycle)
      case 'D': {
        mode = 1;
        Serial.println("OK - Demo mode.");
        break;
      }
      //clock mode (shows time)
      case 'C': {
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
