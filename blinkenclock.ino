
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
 * 06 Jun 2015 - Change Set Clock
 * 07 Jun 2015 - Reimplement Alarm
 * 20 Sep 2015 - Use DS3232RTC

 * todo: Fancy animation for Noon and Midnight
 *       Make set RTC working on non initiated RTC
 *
 * */

#include "Adafruit_NeoPixel.h"
#include <DS3232RTC.h>        //http://github.com/JChristensen/DS3232RTC
#include <Time.h>             //http://playground.arduino.cc/Code/Time
#include <Wire.h>             //http://arduino.cc/en/Reference/Wire

// define something
#define LED_PIN 6 // LED strip pin
#define SQ_PIN 7  // Squarewave pin from RTC

// init LED Stripe
Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, LED_PIN, NEO_GRB + NEO_KHZ800);

// default mode is clock mode
uint8_t mode = 0;

//  fader_count
uint8_t fader_count = 0;
uint8_t alert_count = 0;

// Daylight Saving Time
boolean  dst = 0;

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

// sqwarewave State
boolean sqState = 0;
boolean lastState = 1;

uint8_t alert = 0;

// initialize everything
void setup() {
  Serial.begin(9600);
  strip.begin();
  strip.setBrightness(192);       // Dimmer
  strip.show();

  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  if (timeStatus() != timeSet)
    Serial.println("Unable to sync with the RTC");
  else
    Serial.println("RTC has set the system time");

  //boolean setTime = 0;
  //if ( setTime == 1 ) {
  //  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  //  //Serial.println(DateTime(F(__DATE__), F(__TIME__)));
  //}

  pinMode(SQ_PIN, INPUT);
  RTC.squareWave(SQWAVE_1_HZ);
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
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

  // alert - overrides everything
  if (alert == 1) {
    drawCycle((fader_count / 4.25), strip.Color(255, 255, 0));
  }
  if (alert == 2) {
    drawCycle((fader_count / 4.25), strip.Color(255, 0, 0));
  }

  // redraw if needed
  if (redraw) {
    strip.show();
    redraw = 0;
  }
  delay(1);
}

// clock mode
void clockMode() {

  sqState = digitalRead(SQ_PIN);

  if ((sqState == 0) && (lastState == 1)) {
    fader_count = 0;
    setSyncProvider(RTC.get);   // the function to get the time from the RTC
    analoghour = hour();
    analogminute = minute();
    analogsecond = second();
    lastanalogsecond = pixelCheck(analogsecond - 1);

    //analoghour = ((analoghour + dst) % 12);
    analoghour = ((analoghour) % 12);
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

  if ((second() % 5) == 0) {   // make sure 5 Minute is brighter
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
      //alert mode - green alert (clock flashes orange)
      case 'G':
        {
          alert = 0;
          Serial.println("OK - Green Alert.");
          break;
        }

      //alert mode - orange alert (clock flashes orange)
      case 'O':
        {
          alert = 1;
          Serial.println("OK - Orange Alert.");
          break;
        }

      //alert mode - red alert (clock flashes red)
      case 'R':
        {
          alert = 2;
          Serial.println("OK - Red Alert - Shields up! Arm the phasers!");
          break;
        }
      //ambient mode (clock shows defined color)
      case 'A':
        {
          mode = 3;
          Serial.println("OK - Ambient light mode. Chill!");
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
