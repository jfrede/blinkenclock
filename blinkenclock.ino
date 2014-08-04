
/*
 * blinkenclock - multiprupose LED wall clock
 * version 0.1 alpha
 * Copyright by Bjoern Knorr 2013
 * 
 * http://netaddict.de/blinkenlights:blinkenclock
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
 * 
 * */

#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Time.h>  
#include <DS1307RTC.h>

// define something
#define LED_PIN 6 // LED strip pin
#define BUTTON_PIN 18 // push button pin number

Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, LED_PIN, NEO_GRB + NEO_KHZ800);

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

// alert counter
int alertcounter = 0;

// redraw flag
boolean redraw = 1;

// time cache
unsigned long currenttime = 0;
unsigned long lasttime = 0;
unsigned long alerttime = 0;

// last second
uint8_t lastsecond = 0;

// strip color (ambient)
uint32_t color_ambient;

// initialize everything
void setup() {
  Serial.begin(9600);
  setSyncProvider(RTC.get);
  setSyncInterval(1);
  strip.begin();
  strip.show();
  lasttime = millis();
  currenttime = millis();
  lastsecond = second();
  color_ambient = strip.Color(0, 180, 255);
  pinMode(A0, INPUT);
  pinMode(BUTTON_PIN, INPUT);
  digitalWrite(BUTTON_PIN, HIGH);
  delay(20);
}

// main loop
void loop() {  

  // timing
  currenttime = millis();

  // check for button
  if(digitalRead(BUTTON_PIN) == LOW) {
    mode++;
    if (mode>3) {
      mode = 0;  
    }
    delay(250);
  }

  // if enough serial data available, process it
  if(Serial.available()) {
    serialMessage();
  }
  
  // select mode and show blinken pixelz!
  // show clock
  if (mode==0) {
    if(currenttime - lasttime > 45) {
      clockMode();
      redraw = 1;
      lasttime = currenttime;
    }
  }

  // demo mode - show rgb cycle
  else if (mode==1) {
    if(currenttime - lasttime > 50) {
      
        // reset counter
      if (counter >= 256) {
        counter = 0;
      }
    
      for(uint16_t i=0; i< strip.numPixels(); i++) {
        strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + counter) & 255));
      }
      redraw = 1;
      counter++;
      lasttime = currenttime;
    }
  }

  // music mode
  else if (mode==2) {
    if(currenttime - lasttime > 5) {
      int sensorvalue = map(analogRead(A0), 300, 900, 0, 255);
      if (sensorvalue<0) {
        sensorvalue = 0;
      }
      lightPixels(strip.Color(1*sensorvalue, 1*sensorvalue, 1*sensorvalue));
      redraw = 1;
      lasttime = currenttime;
    }
  }
  
  else if (mode==3) {
    lightPixels(color_ambient);
    redraw = 1;
  }
  
  // alert - overrides everything
  if (alert && (currenttime - alerttime > 20)) {
    if (alertcounter > 59) {
      alertcounter = 0;  
    }
    alertcounter++;
    redraw = 1;
    alerttime = millis();
  }
  if (alert==1) {
    drawCycle(alertcounter, strip.Color(25, 20, 0));
  }
  if (alert==2) {
    drawCycle(alertcounter, strip.Color(25, 0, 0));
  }
    
  // redraw if needed
  if(redraw) {
    strip.show();
    redraw = 0;
  }
}

// clock mode
void clockMode() {
  time_t t = now();
  uint8_t analoghour = hour(t);
  uint8_t currentsecond = second(t);
  
  if (analoghour > 12) {
    analoghour=(analoghour-12);
  }
  analoghour = analoghour*5+(minute(t)/12);

  lightPixels(strip.Color(2, 2, 2));
  
  if (coptionfivemin) {
    for (uint8_t i=0; i<60; i += 5) {
      strip.setPixelColor(i,strip.Color(10, 10, 10));
    }
  }
  
  strip.setPixelColor(pixelCheck(analoghour-1),strip.Color(50, 0, 0));
  strip.setPixelColor(pixelCheck(analoghour),strip.Color(255, 0, 0));
  strip.setPixelColor(pixelCheck(analoghour+1),strip.Color(50, 0, 0));
  
  strip.setPixelColor(minute(t),strip.Color(0, 0, 255));
  
  if (coptionfade) {
    // reset counter
    if(counter>25) {
      counter = 0;
    }
    else if (lastsecond != currentsecond) {
      lastsecond = second();
      counter = 0;  
    }
    
    // if Colors overlap merge them 
    int blue = 0;
    if ((second(t)+1) == minute(t)) {
      blue = 255;
    }
    int red = 0;
    if ((second(t)+1) == analoghour-1) {
      red = 50;
    }
    if ((second(t)+1) == analoghour) {
      red = 255;
    }
    if ((second(t)+1) == analoghour+1) {
      red = 50;
    } 
    strip.setPixelColor(pixelCheck(second(t)+1),strip.Color(red, counter*10, blue));  
    
    blue = 0;
    if (second(t) == minute(t)) {
      blue = 255;
    }
    red = 0;
    if (second(t) == analoghour-1) {
      red = 50;
    }
    if (second(t) == analoghour) {
      red = 255;
    }
    if (second(t) == analoghour+1) {
      red = 50;
    }
    strip.setPixelColor(second(t),strip.Color(red, 255-(counter*10), blue));
    counter++; 
  }
  else {
    int blue = 0;
    if (second(t) == minute(t)) {
      blue = 255;
    }
    int red = 0;
    if (second(t) == analoghour-1) {
      red = 50;
    }
    if (second(t) == analoghour) {
      red = 255;
    }
    if (second(t) == analoghour+1) {
      red = 50;
    }
    strip.setPixelColor(second(t),strip.Color(red, 255, blue));
  }
}

// cycle mode
void drawCycle(int i, uint32_t c) {
  for(uint8_t ii=5; ii>0; ii--) {
    strip.setPixelColor(pixelCheck(i-ii),c);
  }
}

// show a progress bar - assuming that the input-value is based on 100%
void progressBar(int i) {
  map(i, 0, 100, 0, 59);
  lightPixels(strip.Color(0, 0, 0));
  for (uint8_t ii=0; ii<i; ii++) {
    strip.setPixelColor(ii,strip.Color(5, 0, 5));
  }
}

// light all pixels with given values
void lightPixels(uint32_t c) {
  for (uint8_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i,c);
  }
}

// set the correct pixels
int pixelCheck(int i) {
  if (i>59) {
    i = i - 60;
  }
  if (i<0) {
    i = i +60;
  }
  return i;
}

void serialMessage() {
  if(Serial.available()){
    char sw = Serial.read(); 
    switch (sw) {

      // set time
    case 'T': 
      {
        delay(50);
        time_t pctime = 0;
        while(Serial.available() >=  10 ){       
          for(int i=0; i < 10; i++){   
            char c = Serial.read();    
            Serial.print("x");       
            if( c >= '0' && c <= '9'){   
              pctime = (10 * pctime) + (c - '0');    
            }
          }   
        }
        setTime(pctime);
        RTC.set(pctime);
        Serial.println("OK - Time set");
        break;
      }

      //demo mode (shows rgb cycle)
    case 'D': 
      {
        mode = 1;
        Serial.println("OK - Demo mode.");
        break;
      }

      //clock mode (shows time)
    case 'C': 
      {
        mode = 0;
        Serial.println("OK - Clock mode.");
        break;
      }

      //music mode (clock shows bouncing colors)
    case 'M': 
      {
        mode = 2;
        Serial.println("OK - Music mode. Turn up the volume!11");
        break;
      }

      //ambient mode (clock shows defined color)
    case 'L': 
      {
        mode = 3;
        Serial.println("OK - Ambient light mode. Chill!");
        break;
      } 
       
      //Print diag time
    case 'P': 
      {
        Serial.println("OK - Ambient light mode. Chill!");
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
      
       //clock option five minute dots
    case '5': 
      {
        if (coptionfivemin) {
          coptionfivemin = 0;  
        }
        else {
          coptionfivemin = 1;  
        }
        Serial.println("OK - Tongled clock five minute dots.");
        break;
      } 
      
       //clock option fade seconds
    case 'F': 
      {
        if (coptionfade) {
          coptionfade = 0;  
        }
        else {
          coptionfade = 1;  
        }
        Serial.println("OK - Tongled clock fade mode.");
        break;
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } 
  else if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } 
  else {
    WheelPos -= 170;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
