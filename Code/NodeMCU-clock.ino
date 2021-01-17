/*
Copyright (c) 2021 Shajeeb TM

https://github.com/shajeebtm/NodeMCU-Clock-Temperature-Hijridate/
Please refer https://create.arduino.cc/projecthub/shajeeb/nodemcu-clock-with-temperature-hijri-date-and-prayer-times-91e5da for additional details

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <time.h>
#include <NTPClient.h>            // install "NTPClient" library via Arduino IDE Library Manager
#include <FastLED.h>              // install "FastLED" library via Arduino IDE Library Manager 
#include <OpenWeatherMapCurrent.h> // install "ESP8266 Weather Station" library  via Arduino IDE Library Manager 
#include <JsonListener.h>         // install "Json Streaming Parser" library via Arduino IDE Library Manager 
#include <LEDMatrix.h>            // install https://github.com/AaronLiddiment/LEDMatrix 
#include <LEDText.h>              // install https://github.com/AaronLiddiment/LEDText
#include <FontMatrise.h>          // part of https://github.com/AaronLiddiment/LEDText
#include <HijriCalendar.h>        // install https://github.com/shajeebtm/HijriCalendar-Library-For-Arduino/blob/master/src/HijriCalendar.h
#include <PrayerTimes.h>          // install https://github.com/shajeebtm/PrayerTimes-Library-For-Arduino/blob/master/src/PrayerTimes.h
#include <FontNumbers3x7.h>       // install https://github.com/shajeebtm/Fonts-For-Arduino/blob/master/src/FontNumbers3x7.h
#include <FontLetters4x7.h>       // install https://github.com/shajeebtm/Fonts-For-Arduino/blob/master/src/FontLetters4x7.h

#define LED_PIN        3          // this is where the WS2812B display is connected
#define COLOR_ORDER    GRB
#define CHIPSET        WS2812B
#define MATRIX_WIDTH   16  
#define MATRIX_HEIGHT  16  
#define MATRIX_TYPE    HORIZONTAL_ZIGZAG_MATRIX  // Depends on the type of the display matrix

// *** Following values need to be set before running the program ***** //
const char *ssid     = "Your wifi SSID";     
const char *password = "Your wifi password";
String OPEN_WEATHER_MAP_APP_ID = "xxxxxxxxxxxx"; ;  // you need an account on https://openweathermap.org/ for this
String OPEN_WEATHER_MAP_LOCATION_ID = "ccccccc";  // Your location/city ID can be found at https://openweathermap.org/city
double my_latitude = 37.368832;   // For Sunnyvale , CA, USA 
double my_longitude = -122.036346;// For Sunnyvale , CA, USA 
double my_timezone = -8;          // For Sunnyvale , CA, USA 
bool my_dst_on = false;           // True between March to September in USA
String my_calc_method = "ISNA";
String my_asr_method = "Shafii";
String my_high_lats_method = "None";
// *** Above values need to be set before running the program  ***** //

const unsigned char eid_greeting_msg[] = {  EFFECT_HSV_AH "\x00\xff\xff\xff\xff\xff"
                                  EFFECT_SCROLL_LEFT "  EID MUBARAK " };
const unsigned char ramadan_greeting_msg[] = {  EFFECT_HSV_AH "\x00\xff\xff\xff\xff\xff"
                                  EFFECT_SCROLL_LEFT "  RAMADAN MUBARAK " };    
                                  
const byte star_1[]PROGMEM={116,135,139,147,148,151,211,212,214,215,232,236,242};
const byte star_2[]PROGMEM={168,175, 177,182, 201,204,};

const byte moon[]PROGMEM={
        4,9,20,28,34,37,41,44,51,52,59,62,64,66,76,77,81,82,93,95,96,97,109,110,113,113,
        126,127,128,129,158,159,160,162,189,190,193,194,220,221,227,228,249,251
        };
                                         
                                  
void setup() {
        Serial.begin(115200);
      
        if (my_dst_on) { my_timezone += 1; }
        timeClient.begin();
        timeClient.setTimeOffset(my_timezone * 3600); 
        timeClient.update();
        
      
        WiFi.begin(ssid, password);
          while ( WiFi.status() != WL_CONNECTED ) {
          delay ( 500 );
            Serial.print ( "." );
        }
        
        FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds[0], leds.Size());
        FastLED.setBrightness(20);  // limitting brighness of LEDs
        
        ScrollingMsg2.SetFont(FontNumbers3x7Data);
        ScrollingMsg1.Init(&leds, 16, 8 , 0, 9);
        ScrollingMsg1.SetFont(MatriseFontData);
        ScrollingMsg1.SetScrollDirection(SCROLL_LEFT);
        FastLED.clear();    
}


void loop() {
  if (display_all_prayers) {
      showAllPrayerTimes();
  } else {
      showClockWithScrolling();
  }
}


void showClockWithScrolling () {
          if (millis() - lastAnimationMs > 70 ) {   // change this number to adjust the message scrolling speed
              if (ScrollingMsg1.UpdateText() == -1 ) {
                   loadScrollingMessage();
              } else {
                  FastLED.show();
              }
            lastAnimationMs = millis();
          }
          if (millis() - lastTimerMs < 1000) {      // Update clock only on every sec,  blink dots 
              return;
          }

          if (display_all_prayers == LOW ) {
              showHoursMins();
              blinkDots();
              lastTimerMs = millis();
          }  
}


void loadScrollingMessage () {
        static int hijraOrGregorian = 1;
        getDateTime();
        getWeatherTemperature();
        strcpy(dateString,"  ");
      
        switch (hijraOrGregorian) {
          case 1: // Scroll Hijri date
              animate_hijri_date();
              hijraOrGregorian = 2;
              break;
          case 2: // Scroll next Prayer Time
              animate_prayer_time();
              hijraOrGregorian = 3;
              break;
          case 3: // Scroll Gregorian date
              animate_gregorian_date();
              hijraOrGregorian = 4 ;
              break;
          case 4: // Scroll  Temperature
              animate_weather_temperature();
              if (isEidDay()) {                   // display or skip  Eid greeting
                hijraOrGregorian = 5 ;
              } else if (isRamadanMonth()) {
                  hijraOrGregorian = 7;
              } else {
                hijraOrGregorian = 8;
              }
              break;
          case 5: // Scroll Eid greeting
              animate_eid_greeting();
              hijraOrGregorian = 6 ;
              break;
          case 6: // Scroll Eid greeting once again
              animate_eid_greeting();
              hijraOrGregorian = 8 ;
              break;
          case 7: // Scroll ramadan greeting
               animate_ramadan_greeting();
               hijraOrGregorian = 8 ;
               break;
          case 8: // Display all 5 prayer times
              display_all_prayers=HIGH;
              hijraOrGregorian = 9 ;
              break;
          case 9: // Scroll blank
              for (byte i=1; i<=10 ; i++ ) {
                  strcat(dateString," ");
              }
              hijraOrGregorian = 1 ;
              break;
        }     
}

void showHoursMins() {
        
        if (Hour == 1 && Minute == 0) {
          erase_bottom();  // to make transitiion from 12 to 1
        }
         
        char blank[3];
        char string[3];
        itoa(Hour, blank, 10);
        if (Hour > 9) {
          ScrollingMsg2.Init(&leds, 16, 8 , 0, 0);
        } else {
          ScrollingMsg2.Init(&leds, 16, 8 , 4, 0);
        }
      
      
        ScrollingMsg2.SetFont(FontNumbers3x7Data);
        ScrollingMsg2.SetTextColrOptions(COLR_RGB | COLR_SINGLE, 0x20, 0xb2, 0xaa);
        ScrollingMsg2.SetText((unsigned char *)blank, sizeof(blank) - 1);
        ScrollingMsg2.UpdateText();
      
        // blinkDots();
      
        if (Minute < 10) {
          strcpy(string, "0");
        } else {
          strcpy(string, "");
        }
        strcpy(blank, "");
        itoa(Minute, blank, 10);
        strcat(string, blank);
        ScrollingMsg2.Init(&leds, 16, 8 , 9, 0);
        ScrollingMsg2.SetTextColrOptions(COLR_RGB | COLR_SINGLE, 0x20, 0xb2, 0xaa);
        ScrollingMsg2.SetText((unsigned char *)string, sizeof(string) - 1);
        ScrollingMsg2.UpdateText();
      
        // FastLED.show();


}

void animate_hijri_date() {
          char tmp[10];
          gregorianToHijri (Month , Date , Year);
          strcpy(tmp,"");itoa(Year,tmp,10); strcat(dateString,tmp); strcat(dateString,"H ");
          strcpy(tmp,""); itoa(Date,tmp,10); strcat(dateString,tmp); //strcat(dateString," ");
          month_table[Month-1].toCharArray(tmp,month_table[Month-1].length()+1);
          strcat(dateString,tmp);
          strcat(dateString," * ");
          ScrollingMsg1.SetTextColrOptions(COLR_RGB | COLR_SINGLE, 0x00, 0xc8, 0x00);
          ScrollingMsg1.SetText((unsigned char *)dateString, strlen(dateString));
}


void animate_prayer_time() {
          char tmp[5];
          float now = PrayerTimes::time12_to_float_time(Hour, Minute, PM);
          float bufferTime = float(prayerBufferMins)/60 ; 
  
          byte nextPrayer;
          my_prayer_object.get_prayer_times(Year, Month, Date, my_latitude , my_longitude , my_timezone , my_dst_on, my_calc_method, my_asr_method, my_high_lats_method, my_prayer_times );
          if (now > 0.0 && now <= (my_prayer_times[0] + float(prayerBufferMins)/60 )) { nextPrayer = 0; }
           else if (now > (my_prayer_times[0] + bufferTime ) && now <= (my_prayer_times[1] + bufferTime )) { nextPrayer = 1; }
           else if (now > (my_prayer_times[1] + bufferTime ) && now <= (my_prayer_times[2] + bufferTime )) { nextPrayer = 2; }
           else if (now > (my_prayer_times[2] + bufferTime ) && now <= (my_prayer_times[3] + bufferTime )) { nextPrayer = 3; }
           else if (now > (my_prayer_times[3] + bufferTime ) && now <= (my_prayer_times[5] + bufferTime )) { nextPrayer = 5; }
           else if (now > (my_prayer_times[5] + bufferTime ) && now <= (my_prayer_times[6] + bufferTime )) { nextPrayer = 6; }
           else if (now > (my_prayer_times[6] + bufferTime)) { nextPrayer = 0; }      
  
          prayer_table[nextPrayer].toCharArray(tmp,prayer_table[nextPrayer].length()+1);
          strcat(dateString,tmp); strcat(dateString," ");
          strcpy(tmp,"");PrayerTimes::float_time_to_time12 (my_prayer_times[nextPrayer], false, tmp); strcat(dateString,tmp);
          strcat(dateString," * ");
          ScrollingMsg1.SetTextColrOptions(COLR_RGB | COLR_SINGLE, 0x99, 0x4c, 0x00);
          ScrollingMsg1.SetText((unsigned char *)dateString, strlen(dateString));
}

void animate_gregorian_date() {
          char tmp[5];
          strcpy(tmp,""); itoa(Date,tmp,10); strcat(dateString,tmp); strcat(dateString,"");
          e_month_table[Month-1].toCharArray(tmp,e_month_table[Month-1].length()+1);
          strcat(dateString,tmp); strcat(dateString,"");
          strcpy(tmp,""); itoa(Year,tmp,10); strcat(dateString,tmp); strcat(dateString," * ");
          ScrollingMsg1.SetTextColrOptions(COLR_RGB | COLR_SINGLE, 0x33, 0x33, 0xff);
          ScrollingMsg1.SetText((unsigned char *)dateString, strlen(dateString));
}


void animate_weather_temperature() {
          char tmp[10];
          strcat(dateString,"Temp ");
          int Temp=weatherDataMetric.temp;
          itoa(Temp,tmp,10);
          strcat (dateString,tmp);
          strcat(dateString,"C/");

          Temp=weatherDataImperial.temp;
          itoa(Temp,tmp,10);
          strcat (dateString,tmp);
          strcat(dateString,"F");
          strcat(dateString," * ");
          ScrollingMsg1.SetTextColrOptions(COLR_RGB | COLR_SINGLE, 0xff, 0x33, 0x33);
          ScrollingMsg1.SetText((unsigned char *)dateString, strlen(dateString));
}

void animate_eid_greeting() {
        ScrollingMsg1.SetText((unsigned char *)eid_greeting_msg, sizeof(eid_greeting_msg) - 1);
}

void animate_ramadan_greeting() {
        ScrollingMsg1.SetText((unsigned char *)ramadan_greeting_msg, sizeof(ramadan_greeting_msg) - 1);
}


void showAllPrayerTimes() {
  
      if (millis() - lastAnimationMs < 3000 ) {
        return;
      }
      lastAnimationMs = millis();
  
      strcpy(dateString, "");

      char tmp_hour_min[7];
      char tmp_hour[3];
      char *tmp_min;
      static byte nextPrayer=0; 

      if (nextPrayer > 6) {           // No more prayers to show
        nextPrayer = 0;
        display_all_prayers = LOW;    // Stop showing allPrayerTimes
        ScrollingMsg1.SetFont(MatriseFontData);
        ScrollingMsg1.SetText((unsigned char *)dateString, strlen(dateString));
        ScrollingMsg1.UpdateText();
        FastLED.clear();

        if(isFirstOfHijriMonth()) {   // Display moon & star on first of every Hijri month
            draw_moon();
            flash_star();
            delay(200);
            FastLED.clear(true);
        }
        return;
      }


      FastLED.clear(true);
      FastLED.show();


      my_prayer_object.get_prayer_times(Year, Month, Date, my_latitude , my_longitude , my_timezone , my_dst_on, my_calc_method, my_asr_method, my_high_lats_method, my_prayer_times );
      ScrollingMsg1.Init(&leds, 16, 8 , 0, 9);
      ScrollingMsg1.SetFont(FontLetters4x7Data);
      set_prayername_color(nextPrayer);   
      ScrollingMsg1.UpdateText();
      PrayerTimes::float_time_to_time12 (my_prayer_times[nextPrayer], false, tmp_hour_min);  
      int index=strchr(tmp_hour_min,':')-tmp_hour_min;
      strncpy(tmp_hour,tmp_hour_min,index);
      tmp_hour[index]='\0';
      tmp_min = tmp_hour_min + index + 1;

    
      if (atoi(tmp_hour) > 9) {
        ScrollingMsg2.Init(&leds, 16, 8 , 0, 0);
      } else {
        ScrollingMsg2.Init(&leds, 16, 8 , 4, 0);
      }
      ScrollingMsg2.SetFont(FontNumbers3x7Data);
      ScrollingMsg2.SetText((unsigned char *)tmp_hour, sizeof(tmp_hour) - 1);
      ScrollingMsg2.UpdateText();
      ScrollingMsg2.Init(&leds, 16, 8 , 9, 0);
      ScrollingMsg2.SetText((unsigned char *)tmp_min, sizeof(tmp_min) - 1);
      ScrollingMsg2.UpdateText();

      blinkSec = HIGH;
      blinkDots();
      nextPrayer++;
      if (nextPrayer == 4 || nextPrayer == 1) {  // skip Sunset , Shurq time
        nextPrayer++;
      }
      
      FastLED.show();
}



void set_prayername_color(byte nextPrayer ) {
  switch (nextPrayer) {
    case 0 : // Fajr
        ScrollingMsg1.SetText((unsigned char *)FajrNameColor, sizeof(FajrNameColor) - 1);
        break; 
    case 2 : // Duhr
        ScrollingMsg1.SetText((unsigned char *)DuhrNameColor, sizeof(DuhrNameColor) - 1);
        break;
      case 3 : // Asar
        ScrollingMsg1.SetText((unsigned char *)AsarNameColor, sizeof(AsarNameColor) - 1);
        break ; 
      case 5 : // Mgrb
        ScrollingMsg1.SetText((unsigned char *)MgrbNameColor, sizeof(MgrbNameColor) - 1);
        break ; 
      case 6 : // Isha
        ScrollingMsg1.SetText((unsigned char *)IshaNameColor, sizeof(IshaNameColor) - 1);
        break ;
  }
}


bool isEidDay() {
          int h_Month=Month;
          int h_Date=Date;
          int h_Year=Year;
          gregorianToHijri (h_Month , h_Date , h_Year);
          if (h_Month == 10 && h_Date == 1) {return true;}      // Ramzan on Shawwal 1st
          if (h_Month == 12 && h_Date == 10) {return true;}    // Bakrid on Dulhijjah 10th
          return false;
}

bool isFirstOfHijriMonth() {
          int h_Month=Month;
          int h_Date=Date;
          int h_Year=Year;
          gregorianToHijri (h_Month , h_Date , h_Year);
          if (h_Date == 1) {return true;}     
          return false;
}

bool isRamadanMonth() {
          int h_Month=Month;
          int h_Date=Date;
          int h_Year=Year;
          gregorianToHijri (h_Month , h_Date , h_Year);
          if (h_Month == 9) {return true;}     
          return false;
}


void getDateTime () {
            bool Century;
            bool h12;

            timeClient.update();
            unsigned long epochTime = timeClient.getEpochTime();
            struct tm *ptm = gmtime ((time_t *)&epochTime); 
            Year = ptm->tm_year+1900;
            Month = ptm->tm_mon+1;
            Date = ptm->tm_mday;
            Hour = ptm->tm_hour;
            Minute = ptm->tm_min;
            Second = ptm->tm_sec;

            PM=false;
            if(Hour >= 12){
              PM=true; 
            }
            if (Hour > 12){
              Hour = Hour - 12;
            }
            if (Hour == 0) {
              Hour = 12;
            }
}




void getWeatherTemperature(){

        if (millis() - timeSinceLastWUpdate > (1000L*WEATHER_UPDATE_INTERVAL_SECS) || timeSinceLastWUpdate == 0) {
              weatherClient.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
              IS_METRIC = true;
              weatherClient.setMetric(IS_METRIC);
              weatherClient.updateCurrentById(&weatherDataMetric, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID);
              IS_METRIC = false;
              weatherClient.setMetric(IS_METRIC);
              weatherClient.updateCurrentById(&weatherDataImperial, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID);
              timeSinceLastWUpdate = millis();
         }
}



uint16_t XY( uint8_t x, uint8_t y)
{
        uint16_t i;
        if( kMatrixSerpentineLayout == false) {
          i = (y * MATRIX_WIDTH) + x;
        }
      
        if( kMatrixSerpentineLayout == true) {
          if( y & 0x01) {
            // Odd rows run backwards
            uint8_t reverseX = (MATRIX_WIDTH - 1) - x;
            i = (y * MATRIX_WIDTH) + reverseX;
          } else {
            // Even rows run forwards
            i = (y * MATRIX_WIDTH) + x;
          }
        }
        return i;
}


void blinkDots () {
        if (blinkSec) {
          leds( XY( 7, 3) ) = CHSV( 0, 255, 255);
          leds( XY( 8, 3) ) = CHSV( 0, 255, 255);
        } else {
          leds( XY( 7, 3) ) = CRGB::Black;
          leds( XY( 8, 3) ) = CRGB::Black;
        }
        blinkSec = !blinkSec;
      
        FastLED.show(); // added on 17 Nov after Timer  to show the dots
}


void erase_bottom() {
        for (int i = 0 ; i<=128; i++) {
          leds(i)=CRGB::Black;
        }
        FastLED.show();
}



void draw_moon() {
  for (int i=0; i< (sizeof (moon))/2 ; i++) {
   byte c = pgm_read_byte(&moon[i*2]);
   byte  n = pgm_read_byte(&moon[(i*2)+1]);

   for (int k=c; k<=n; k++) {
        leds(k)=CRGB::Yellow;
   }
  }
}

void draw_star(bool draw) {
  for (int i=0; i< (sizeof (star_2))/2 ; i++) {
   byte c = pgm_read_byte(&star_2[i*2]);
   byte n = pgm_read_byte(&star_2[(i*2)+1]);
   for (int k=c; k<=n; k++) {
        if (draw) {
            leds(k)=CRGB::White;
        } else {
            leds(k)=CRGB::Black;
        }
   }
  }

  for (int i=0; i<sizeof star_1; i++) {
    byte  s = pgm_read_byte(&star_1[i]);
    if (draw) {
        leds(s)=CRGB::White;
    } else {
        leds(s)=CRGB::Black;
    }
  }
}


void flash_star() {
  for (int i=1; i<=3; i++ ) {
          draw_star(true);
          FastLED.show();
          delay(500);
          draw_star(false);
          FastLED.show();
          delay(500);
  }
}


