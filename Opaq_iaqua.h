/*
 *  Opaq is an Open AQuarium Controller firmware. It has been developed for
 *  supporting several aquarium devices such as ligh dimmers, power management
 *  outlets, water sensors, and peristaltic pumps. The main purpose is to
 *  control fresh and salt water aquariums.
 *
 *    Copyright (c) 2016 Andre Pedro. All rights reserved.
 *
 *  This file is part of opaq firmware for aquarium controllers.
 *
 *  opaq firmware is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  opaq firmware is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with opaq firmware.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

 /*
 * IAQUA CREDITS TO:
 *   Dan Cunningham, aka AnotherHobby @ plantedtank.net
 *   Ryan Truss, aka MrMan
 */
 
#ifndef OPAQ_IAQUA_H
#define OPAQ_IAQUA_H

#include <Arduino.h>
#include "src/Time/Time.h"

#include "FS.h"
#include "gfx.h"

#define CENTER 0
#define VGA_WHITE RGB(255,255,255)
#define VGA_BLACK RGB(0,0,0)



class Lcd_Hal
{
  uint_least16_t color;
  uint8_t h;
  
  public:

  void setFont(uint8_t* font);
  void setColor(byte r, byte g, byte b);
  void setColor(const uint_least16_t&);
  void drawLine(int x1, int y1, int x2, int y2);
  void print(const __FlashStringHelper* st, int x, int y, int deg=0);
  void print(const char* st, int x, int y, int deg=0);

  void fillRect(int x1, int y1, int x2, int y2);
  void drawRect(int x1, int y1, int x2, int y2);
  void drawRoundRect(int x1, int y1, int x2, int y2);

  void clrScr();
  
};

class Files_Hal
{
  
  public:
  
  void load(int x, int y, int sx, int sy, char *filename, int bufmult=4);
  
};

class Storage_Hal
{
  public:
  
  uint8_t read(uint8_t id);
  void write(uint8_t id, uint8_t data);
};



enum touch_evt {TOUCH_UP, TOUCH_DOWN, TOUCH_MOVE, TOUCH_CLICK};
typedef enum touch_evt touch_evt_type;

class Opaq_iaqua
{
protected:
  Files_Hal   myFiles;
  Lcd_Hal     myGLCD;
  Storage_Hal EEPROM;

private:

  uint8_t* arial_bold = (uint8_t*)0x1; // dummy variable
  uint8_t* Sinclair_S = (uint8_t*)0x1; // dummy variable

  byte dispScreen=0;


  byte scheduleItem; // track which item is being scheduled
  byte currentLightMode=0;  //0=high sun, 1=mid sun, 2=low sun, 3=moon, 4=transition, 5=unknown
  byte lightEditing=0;  // track if we are actively editing lights
  
  // the next 5 variables will be set at the start of a lighting fade
  int fadeDurationSeconds = 0; 
  unsigned long fadeStartingSeconds = 0;
  unsigned long fadeTimeLeft;
  boolean fadeInProgress = false;
  byte fadeFromMode = 0;  //0=high sun, 1=mid sun, 2=low sun, 3=moon
  byte fadeToMode = 0;  //0=high sun, 1=mid sun, 2=low sun, 3=moon

  struct RGBW // for storing light intensity values
  {
    unsigned int chan1;
    unsigned int chan2;
    unsigned int chan3;
    unsigned int chan4;
    unsigned int chan5;
    unsigned int chan6;
  };
  typedef struct RGBW LightColor;

  LightColor currentColor = {
    0,0,0,0,0,0}; // The current color of the light (used for fading)
  LightColor lastColor = {
    0,0,0,0,0,0};   // The previous color of the light (used for fading)
  LightColor targetColor = {
    0,0,0,0,0,0}; // The target color of the light (used for fading)
  LightColor tempColor = {
    0,0,0,0,0,0}; // The temporary color of the light (used for updating sat+)
  
  LightColor lightHighSun = {
    0,0,0,0,0,0}; // store the RGBW values for the CS+ M1 button
  LightColor lightMidSun = {
    0,0,0,0,0,0}; // store the RGBW values for the CS+ M2 button 
  LightColor lightLowSun = {
    0,0,0,0,0,0}; // store the RGBW values for the CS+ M3 button 
  LightColor lightMoon = {
    0,0,0,0,0,0}; // store the RGBW values for the CS+ M4 button 
    
  //red, green, blue, white, yellow, violet, aqua
  const unsigned int VGAColor[7] = {0xF800, 0x07E0, 0x001F, 0xFFFF, 0xFFE0, 0xF81F, 0x07FF};
  
  //store colours for bargraphs
  byte barColors[6] = {0,1,2,3,4,5};
  byte selectedChan = 0; // used for tracking which channel is selected when changing colors

  // used for time
  tmElements_t prevRTC, saveRTC;
  boolean displayIn12Hr = false;

  // used for storing power states of relays
  struct PWR
  {  
    byte pwrLight1;
    byte pwrLight2;
    byte pwrFilter;
    byte pwrCirc;
    byte pwrHeat;
    byte pwrCO2;
    byte pwrAux1;
    byte pwrAux2;
  } 
  feedPower, preFeedPower, globalPower;

  // holds the schedule for power relays and light ramping
  struct PWRSCHED
  {  
    byte active;
    byte onHour;
    byte onMinute;
    byte offHour;
    byte offMinute;
  } 
  schedLights1,schedLights1s2,schedLights2,schedLights2s2,schedCirc,schedCo2,schedAux1,schedAux2,ramp1,ramp2,ramp3,ramp4,ramp5,ramp6;

  // holds the schedulin for 2 dosing pumps
  struct PUMPSCHED
  {  
    byte onHour;
    byte onMinute;
    byte Sunday;
    byte Monday;
    byte Tuesday;
    byte Wednesday;
    byte Thursday;
    byte Friday;
    byte Saturday;
  } 
  pump1, pump2, pump3;

  //Feeding variables
  boolean feedingActive=false; // track if feeding is currently active
  byte feedingMins=0; // stores how long the feeding should be
  time_t lastFeedingTime; //time of last feeding
  time_t startFeedingTime;

  // Sensor variables
  boolean displayInC = true;
  boolean sensor1Enabled = true; // water
  boolean sensor2Enabled = false; // heatsink
  boolean sensor3Enabled = false; // internal
  boolean displaySensor1 = true;
  boolean displaySensor2 = true;
  boolean displaySensor3 = false;
  byte sensorToDisplay = 1;
  float temperature = 0;  // water temperature
  float temperature2 = 0;  // heatsink temperature
  float temperature3 = 0; // internal temperature
  float sensor1Calibration = 0.0; // calibration offset for sensor 1 (water)
  float sensor2Calibration = 0.0; // calibration offset for sensor 2 (heatsink)
  float sensor3Calibration = 0.0; // calibration offset for sensor 3 (internal)
  boolean fan1status = false;
  boolean fan2status = false;
  byte internalFanOnTemp = 45;
  byte internalFanOffTemp = 40;
  byte heatsinkFanOnTemp = 40;
  byte heatsinkFanOffTemp = 45;

  //heater variables
  boolean heaterWarning=false; // keeps track if there is an active overheating issue
  boolean heaterWarningCleared=true; // keeps track if we clear the warning, impacts home screen icon
  byte heatOffTemp = 30;
  byte heatOnTemp = 25;
  
  //dosing pump variables
  boolean pump1State = false;
  boolean pump2State = false;
  boolean pump3State = false;
  unsigned long pump1StartMillis = 0;
  unsigned long pump2StartMillis = 0;
  unsigned long pump3StartMillis = 0;
  unsigned long pump1millis = 0;
  unsigned long pump2millis = 0;
  unsigned long pump3millis = 0;
  
  //ATO variables
  boolean ATOEnabled = true;
  boolean ResSwitchEnabled = false;
  boolean WaterLevel = LOW;
  boolean ReservoirLevel = LOW;
  boolean ATOAlarm = false;
  boolean ATOPumpState = LOW;
  time_t ATOStartTime = 0;
  time_t ATOPumpStartTime = 0;
  unsigned long ATORunTime = 120;    //seconds to run ATO pump for before flagging an alarm
  unsigned long ATOWaitTime = 60; //time to wait in minutes
  
  // various millis to keep track of touch timing
  unsigned long prevMillisTouch = 0;
  unsigned int touchWaitTime = 350;
  #define LONG_WAIT 500
  #define MEDIUM_WAIT 150
  #define SHORT_WAIT 25
  
  // time variables to track clock updating, dimming timing and time to return home from a menu
  time_t lastUpdateTime = 0; // track 5 seconds for refreshing clock and temp
  time_t lastCheckTime = 0; // track 1 second for checking ATO and feeding
  time_t dimTime = 0; // used for brightness adjustment
  time_t homeTime = 0; // used for returning home after configured time
  
  // screen settings corresponding to eeprom values 28-31
  byte screenRetHome, screenDimLevel, screenDimSec, screenBrightMem;
  
  byte backLight = 255;  // startup brightness to 100%
  boolean backlightTouch = true; // initial setting of true to start the screen bright after boot
  
  // if you have a Current Satellite Plus, this is true
  // if you are controlling your lights directly with PWM, this is false
  boolean lightCSP = true;
  int maxIR = 100; //using e-series
  
  //If PCA9865 is installed this is true, otherwise it is false to use Arduino PWM pins
  boolean PCA9685Installed = true;



  // ####################################################################################
  // FILENAMES
  // ####################################################################################
  
  // selected lights mode buttons for lights screen
  char *lightModeS[4] = {"5hsunS.raw","5msunS.raw","5lsunS.raw","5moonS.raw"};
  // off lights mode buttons for lights screen
  char *lightModeF[3] = {"5hmsunF.raw","5lsunF.raw","5moonF.raw"};
  // neutral lights mode buttons for lights screen
  char *lightMode[4] = {"5hsun.raw","5msun.raw","5lsun.raw","5moon.raw"};
  
  // editing buttons for the lights screen, disabled and enabled
  char *lightEdit[2] = {"5editF.raw", "5editN.raw"};
  char *lightSave[2] = {"5saveF.raw", "5saveN.raw"};
  char *lightResync[2] = {"5resynF.raw", "5resynN.raw"};
  char *lightCancel[2] = {"5canF.raw", "5canN.raw"};
    
  // lights adjustment buttons for lights screen (RGBW up and down) 
  char *lightWhite[2] = {"5Wup.raw", "5Wdown.raw"};
  char *lightRed[2] = {"5Rup.raw", "5Rdown.raw"};
  char *lightGreen[2] = {"5Gup.raw", "5Gdown.raw"};
  char *lightBlue[2] = {"5Bup.raw", "5Bdown.raw"};
  char *lightGray[2] = {"5Fup.raw", "5Fdown.raw"}; // disabled button
  
  // large power buttons for the power screen and the feeding configuration screen, off and on
  char *pwrLightIcon[2] = {"3light_F.raw","3light_N.raw"};
  char *pwrFilterIcon[2] = {"3filt_F.raw","3filt_N.raw"};
  char *pwrCircIcon[2] = {"3circ_F.raw","3circ_N.raw"};
  char *pwrHeatIcon[2] = {"3heat_F.raw","3heat_N.raw"};
  char *pwrCO2Icon[2] = {"3co2_F.raw","3co2_N.raw"};
  char *pwrAux1Icon[2] = {"3aux1_F.raw","3aux1_N.raw"};
  char *pwrAux2Icon[2] = {"3aux2_F.raw","3aux2_N.raw"};
  
  // on off power dot under each power button on the power screen and feeding config screen
  char *pwrDot[2] = {"3dotR.raw","3dotG.raw"};

  // small power icons for the home screen, off and on
  char *pwrLightIconS[2] = {"1lightF.raw","1lightN.raw"};
  char *pwrFilterIconS[2] = {"1filtF.raw","1filtN.raw"};
  char *pwrCircIconS[2] = {"1circF.raw","1circN.raw"};
  char *pwrHeatIconS[2] = {"1heatF.raw","1heatN.raw"};
  char *pwrCO2IconS[2] = {"1co2F.raw","1co2N.raw"};
  char *pwrAux1IconS[2] = {"1aux1F.raw","1aux1N.raw"};
  char *pwrAux2IconS[2] = {"1aux2F.raw","1aux2N.raw"};
  
  // small light mode icons for home screen
  char *lightModeSm[4] = {"1hsun.raw","1msun.raw","1lsun.raw","1moon.raw"};
    
  //moon images 
  // 0 = new moon, 1 = New Crescent, 2 = First Quarter, 3 = Waxing Gibbous, 4 = Full Moon, 
  // 5 = Waning Gibbous, 6 = Last Quarter, 7 = Old Crescent
  char *moonImages[8] = {"21new.raw","21ncres.raw","21firstq.raw","21waxg.raw","21full.raw","21wang.raw","21lastq.raw","21ocres.raw"};
  
  // 24 pixel up and down arrow buttons used on several screens
  char *arrowButton[2] = {"24whUp.raw", "24whDn.raw"};
  
  // enabled or not enabled small check boxes for the power schedule screen
  char *schedActive[2] = {"11dis.raw","11enab.raw"};
  // enabled or not enabled large check boxes for the power item schedule screen
  char *schedActiveB[2] = {"11disB.raw","11enabB.raw"};
  
  // high/low water level display icon
  char *WaterIcon[2] = {"1wlow.raw","1whigh.raw"};



  public:
  
  void screenHome();
  void screenFeeding();
  void screenPower();
  void screenSettings();
  void screenLights();
  void screenClock();
  void screenFeedSettings();
  void screenHeater();
  void screenSchedule();
  void screenDosing(byte pumpNo);
  void screenScreen();
  void screenATO();
  void screenSensors();
  void screenLunar();
  void screenColor(byte selectedChan);
  void screenGraphLEDs();
  void screenLightsIR();

  byte getLunarCycleDay();
  void graphChannel(int lowSunVal,int midSunVal,int highSunVal,int moonVal,byte channel,int x,int y,int dx,int dy);
  void drawLEDBarGraph(byte barNum, int value);
  void drawATO();
  void drawFeeding();
  void drawTemp();

  void checkDosing();
  void checkLighting();
  
  void updateTimeDate(boolean updateTime);
  void printDate(int x, int y);
  void printTime24Hr(int thour, int tminute, int posx, int posy);
  void printTime(int thour, int tminute, byte ampm, int posx, int posy);
  void printValueUpdate();

  // events
  void homescreen_e(unsigned x, unsigned y, touch_evt_type id);
  void feeding_screen_e(unsigned x, unsigned y, touch_evt_type id);
  // feeding screen auxiliary functions
  void feedingStop();
  int calcFeeding();
  void power_screen_e(unsigned x, unsigned y, touch_evt_type id);
  void settings_screen_e(unsigned x, unsigned y, touch_evt_type id);
  void service(unsigned x, unsigned y, unsigned z);


  // alarm functions
  void AlarmPwrLight1_On();
  void AlarmPwrLight2_On();
  void AlarmPwrCO2_On();
  void AlarmPwrCirc_On();
  void AlarmPwrFilter_On();
  void AlarmPwrHeat_On();
  void AlarmPwrAux1_On();
  void AlarmPwrAux2_On();
  void AlarmPwrLight1_Off();
  void AlarmPwrLight2_Off();
  void AlarmPwrCO2_Off();
  void AlarmPwrCirc_Off();
  void AlarmPwrFilter_Off();
  void AlarmPwrHeat_Off();
  void AlarmPwrAux1_Off();
  void AlarmPwrAux2_Off();

  // interface functions
  //int now(); // time related functions


  // functions to treat
  /*int second();
  int second(time_t&);
  int hour();
  int minute();
  int minute(time_t&);
  int day();
  int weekday();
  int month();
  int hourFormat12();
  int hourFormat12(time_t&);
  bool isPM();
  const char* monthShortStr(int);
  const char* dayShortStr(int);
  time_t makeTime(tmElements_t);*/
  // end functions to treat
};



#endif // OPAQ_IAQUA_H

