
#include "Opaq_iaqua.h"

#include <Time.h>


using namespace Opaq_iaqua_device_HAL;


// #### EVENTS

void Opaq_iaqua::service(unsigned x, unsigned y, unsigned z)
{
  static touch_evt_type touch_state = TOUCH_UP;
  static int n_touch = 0;
  static bool touching = false;
  
  bool touch = z > 10;
  

  // detecting touch events
  if ( touch && ( touch_state == TOUCH_UP || touch_state == TOUCH_CLICK) )
  {
    touch_state = TOUCH_DOWN;
    touching = true;
    n_touch = 0;
  }
  else if ( touch && ( touch_state == TOUCH_DOWN || touch_state == TOUCH_MOVE) )
  {
    touch_state = TOUCH_MOVE;
    n_touch++;
  }
  else if ( !touch )
  {
    touch_state = TOUCH_UP;
    touching = false;

    if ( n_touch > 0 )
    {
      touch_state = TOUCH_CLICK;
    }
  }

  Serial.println(touch_state);
  Serial.println(n_touch);
  
  
  switch (dispScreen) 
  {              
     case 1:  // home screen
     homescreen_e(x, y, touch_state);
     break;
     
     case 2:  // feeding screen
     feeding_screen_e(x, y, touch_state);
     break;
     
     case 3:  // power screen
     power_screen_e(x, y, touch_state);
     break;
     
     case 4:  // settings screen
     settings_screen_e(x, y, touch_state);
     break;
  }

  if(touch_state == TOUCH_CLICK)
  {
    n_touch = 0;
    touch_state = TOUCH_UP;
  }
  
}

void Opaq_iaqua::homescreen_e(unsigned x, unsigned y, touch_evt_type id)
{
  if ( id != TOUCH_CLICK )
     return;

  // home screen
    
    if ((x>=30)&&(x<=90)&&(y>=35)&&(y<=86)) // pressed the thermometer to clear a warning
    {
      if(heaterWarningCleared == false)
      {
        heaterWarningCleared=true; // clear warning
        screenHome();
      }
    }
    if ((x>=150)&&(x<=210)&&(y>=50)&&(y<=105)) // pressed the ATO warning to clear
    {
      if(ATOAlarm)
      {
        Serial.print(F("ATO Alarm cleared.\n"));
        ATOAlarm=false; // clear warning
        EEPROM.write(44,0);
        myGLCD.setColor(VGA_BLACK);
        myGLCD.fillRect(121,30,239,109);
        drawATO();
      }
    }
    if ((x>=10)&&(x<=58)&&(y>=266)&&(y<=314)) // home dock icon
    {
      screenHome();
//      smartStartupPower();
    }
    if ((x>=67)&&(x<=115)&&(y>=266)&&(y<=314)) // feeding dock icon
    {
      screenFeeding();
    }

    // coordinates of the power putton
    if ((x>=124)&&(x<=172)&&(y>=266)&&(y<=314)) // power dock icon
    {
      screenPower();
    }

    // coordinates of the exras button
    if ((x>=181)&&(x<=229)&&(y>=266)&&(y<=314)) // settings dock icon
    {
      screenSettings();
    }

}


void Opaq_iaqua::feeding_screen_e(unsigned x, unsigned y, touch_evt_type id)
{
  if ( id != TOUCH_CLICK )
     return;
     
  // feeding screen

    if ((x>=67)&&(x<=115)&&(y>=223)&&(y<=271))  // stop button  
    {     
      feedingStop();
    }

    if ((x>=124)&&(x<=172)&&(y>=223)&&(y<=271))  // restart button  
    {     
      feedingActive=false;
      screenFeeding();
    }

    if ((x>=107)&&(x<=129)&&(y>=294)&&(y<=318))  // home button  
    {     
      screenHome();
    }
}

void Opaq_iaqua::feedingStop()
{
  if (preFeedPower.pwrFilter==1) AlarmPwrFilter_On();
  else if (preFeedPower.pwrFilter==0) AlarmPwrFilter_Off();
  
  if (preFeedPower.pwrCirc==1) AlarmPwrCirc_On();
  else if (preFeedPower.pwrCirc==0) AlarmPwrCirc_Off();
  
  if (preFeedPower.pwrAux1==1) AlarmPwrAux1_On();
  else if (preFeedPower.pwrAux1==0) AlarmPwrAux1_Off();
  
  if (preFeedPower.pwrAux2==1) AlarmPwrAux2_On();
  else if (preFeedPower.pwrAux2==0) AlarmPwrAux2_Off();

  feedingActive=false; // stop feeding cycle
  lastFeedingTime = now();
  
  if(dispScreen == 1)  //clear previous data
  {
    myGLCD.setColor(0, 0, 0);
    myGLCD.fillRect(1,111,55,171);
    drawFeeding();
  }
  if(dispScreen==2) screenHome(); // Return to the home screen
}


void Opaq_iaqua::power_screen_e(unsigned x, unsigned y, touch_evt_type id)
{
  if ( id != TOUCH_CLICK )
     return;
     
   // power screen

    if ((x>=69)&&(x<=118)&&(y>=77)&&(y<=125))        // all on  
    {
      // turn on all power outputs
      AlarmPwrLight1_On();
      AlarmPwrLight2_On();
      AlarmPwrFilter_On();
      AlarmPwrCirc_On();
      AlarmPwrHeat_On();
      AlarmPwrCO2_On();
      AlarmPwrAux1_On();
      AlarmPwrAux2_On();
      screenPower(); // redraw screen
    }
    else if ((x>=125)&&(x<=174)&&(y>=77)&&(y<=125))    // all off
    {
      // turn off all power outputs
      AlarmPwrLight1_Off();
      AlarmPwrLight2_Off();
      AlarmPwrFilter_Off();
      AlarmPwrCirc_Off();
      AlarmPwrHeat_Off();
      AlarmPwrCO2_Off();
      AlarmPwrAux1_Off();
      AlarmPwrAux2_Off();
      screenPower(); // redraw screen
    }

    else if ((x>=107)&&(x<=129)&&(y>=294)&&(y<=318))  // home button  
    {     
      screenHome();
    }

    else if ((x>=15)&&(x<=64)&&(y>=139)&&(y<=187))    // Front lights power
    {
      //toggle power
      if (globalPower.pwrLight1==0) AlarmPwrLight1_On();
      else if (globalPower.pwrLight1==1) AlarmPwrLight1_Off();
      // draw icons
      myFiles.load(15, 139, 48, 48, pwrLightIcon[globalPower.pwrLight1]);
      myFiles.load(34, 192, 10, 11, pwrDot[globalPower.pwrLight1]);
    }
    else if ((x>=69)&&(x<=118)&&(y>=139)&&(y<=187))    // Back lights power
    {
      // toggle power
      if (globalPower.pwrLight2==0) AlarmPwrLight2_On();
      else if (globalPower.pwrLight2==1) AlarmPwrLight2_Off();
      // draw icons
      myFiles.load(69, 139, 48, 48, pwrLightIcon[globalPower.pwrLight2]);
      myFiles.load(88, 192, 10, 11, pwrDot[globalPower.pwrLight2]);
    }
    else if ((x>=124)&&(x<=173)&&(y>=139)&&(y<=187))    // Filter power
    {
      // toggle power
      if (globalPower.pwrFilter==0) AlarmPwrFilter_On();
      else if (globalPower.pwrFilter==1)AlarmPwrFilter_Off();
      // draw icons
      myFiles.load(124, 139, 48, 48, pwrFilterIcon[globalPower.pwrFilter]);
      myFiles.load(143, 192, 10, 11, pwrDot[globalPower.pwrFilter]);
    }
    else if ((x>=178)&&(x<=227)&&(y>=139)&&(y<=187))    // Circ power
    {
      // toggle power
      if (globalPower.pwrCirc==0) AlarmPwrCirc_On();
      else if (globalPower.pwrCirc==1) AlarmPwrCirc_Off();
      // draw icons
      myFiles.load(178, 139, 48, 48, pwrCircIcon[globalPower.pwrCirc] );
      myFiles.load(197, 192, 10, 11, pwrDot[globalPower.pwrCirc]);
    }

    else if ((x>=15)&&(x<=64)&&(y>=198)&&(y<=246))    // Heat power
    {
      // toggle power
      if (globalPower.pwrHeat==0) AlarmPwrHeat_On();
      else if (globalPower.pwrHeat==1)AlarmPwrHeat_Off();
      // draw icons
      myFiles.load(15, 212, 48, 48, pwrHeatIcon[globalPower.pwrHeat] );
      myFiles.load(34, 264, 10, 11, pwrDot[globalPower.pwrHeat]);
    }
    else if ((x>=69)&&(x<=118)&&(y>=198)&&(y<=246))    // CO2 power
    {
      // toggle power
      if (globalPower.pwrCO2==0) AlarmPwrCO2_On();
      else if (globalPower.pwrCO2==1) AlarmPwrCO2_Off();
      // draw icons
      myFiles.load(69, 212, 48, 48, pwrCO2Icon[globalPower.pwrCO2] );
      myFiles.load(88, 264, 10, 11, pwrDot[globalPower.pwrCO2]);
    }
    else if ((x>=124)&&(x<=173)&&(y>=198)&&(y<=246))    // aux 1 power
    {
      // toggle power
      if (globalPower.pwrAux1==0) AlarmPwrAux1_On();
      else if (globalPower.pwrAux1==1) AlarmPwrAux1_Off();
      // draw icons
      myFiles.load(124, 212, 48, 48, pwrAux1Icon[globalPower.pwrAux1] );
      myFiles.load(143, 264, 10, 11, pwrDot[globalPower.pwrAux1]);
    }
    else if ((x>=178)&&(x<=227)&&(y>=198)&&(y<=246))    // aux 2 power
    {
      // toggle power
      if (globalPower.pwrAux2==0) AlarmPwrAux2_On();
      else if (globalPower.pwrAux2==1) AlarmPwrAux2_Off();
      // draw icons  
      myFiles.load(178, 212, 48, 48, pwrAux2Icon[globalPower.pwrAux2] );
      myFiles.load(197, 264, 10, 11, pwrDot[globalPower.pwrAux2]);
    }
}


void Opaq_iaqua::settings_screen_e(unsigned x, unsigned y, touch_evt_type id)
{
  if ( id != TOUCH_CLICK )
     return;
     
// settings screen

    touchWaitTime = LONG_WAIT;
    if ((x>=107)&&(x<=129)&&(y>=294)&&(y<=318))  // home button  
    {
      screenHome();
    }

    if ((x>=10)&&(x<=58)&&(y>=50)&&(y<=113))
    {
      // only respond to the lights button if they are turned on or PWM
      if ((globalPower.pwrLight1==1)||(lightCSP==false))
      {
        if(lightCSP==false)screenLights();
        if(lightCSP==true)screenLightsIR();
      }
    }
    else if ((x>=67)&&(x<=115)&&(y>=50)&&(y<=113))
    {
      screenClock();
    }
    else if ((x>=124)&&(x<=172)&&(y>=50)&&(y<=113))
    {
      screenFeedSettings();
    }
    else if ((x>=181)&&(x<=229)&&(y>=50)&&(y<=113))
    {
      screenSchedule();
    }
    else if ((x>=10)&&(x<=58)&&(y>=118)&&(y<=181))
    {
      screenSensors();
    }
    else if ((x>=67)&&(x<=115)&&(y>=118)&&(y<=181))
    {
      screenDosing(1);
    }
    else if ((x>=124)&&(x<=172)&&(y>=118)&&(y<=181))
    {
      screenScreen();
    }
    else if ((x>=181)&&(x<=229)&&(y>=118)&&(y<=181))
    {
      screenATO();
    }
    else if ((x>=10)&&(x<=58)&&(y>=186)&&(y<=249))
    {
      screenLunar();
    }
    else if ((x>=67)&&(x<=115)&&(y>=186)&&(y<=249))
    {
      if (lightCSP==false)
      {
        selectedChan = 0;
        screenColor(selectedChan);
      }
    }
    else if ((x>=124)&&(x<=172)&&(y>=186)&&(y<=249))
    {
      screenGraphLEDs();
    }
}


// ####################################################################################
// ####################################################################################

void Opaq_iaqua::screenWelcome()
{
  tft.fillScreen(ILI9341_BLACK);
  myFiles.load(26, 110, 188, 72, "opaq.raw");
}

void Opaq_iaqua::screenHome()  // draw main home screen showing overview info
{ 
  dispScreen=1;  // set screen so we can know what screen was touched later

  myGLCD.clrScr();
  updateTimeDate(true);

  // draw dock, home icon, and header
  myFiles.load(0, 254, 240, 66, "dock.raw",4);
  myFiles.load(2, 2, 30, 30, "1home.raw",4);
  
  myGLCD.setFont(arial_bold);
  myGLCD.setColor(34, 81, 255);
  myGLCD.print(F("HOME"), 36, 12);

  // draw lines to divide screen
  myGLCD.setColor(130, 130, 130);
  myGLCD.drawLine(40, 31, 239, 31); // under header  
  myGLCD.drawLine(0, 110, 239, 110); // across screen below temp
  myGLCD.drawLine(56, 110, 56, 237); // 1st cutting into 4ths
  myGLCD.drawLine(105, 110, 105, 237); // 2nd cutting into 4ths
  myGLCD.drawLine(168, 110, 168, 237); // 3rd cutting into 4ths
  myGLCD.drawLine(120, 31, 120, 110); // cutting top section in half for ATO

  myGLCD.drawLine(0, 237, 239, 237); // across above dock

  // draw temperature to screen
  drawTemp();

  // display feeding info
  drawFeeding();
  myFiles.load(5, 172, 46, 46, "1feed.raw",4);

  // display lighting info
  checkLighting();

  // get remainding doses
  checkDosing();
  
  // display ATO status
  drawATO();

  // draw power status of outputs
  myFiles.load(178, 121, 24, 24, pwrLightIconS[globalPower.pwrLight1],4);
  myFiles.load(206, 121, 24, 24, pwrLightIconS[globalPower.pwrLight2],4);
  myFiles.load(178, 149, 24, 24, pwrFilterIconS[globalPower.pwrFilter],4);
  myFiles.load(206, 149, 24, 24, pwrCircIconS[globalPower.pwrCirc],4);
  myFiles.load(178, 177, 24, 24, pwrHeatIconS[globalPower.pwrHeat],4);
  myFiles.load(206, 177, 24, 24, pwrCO2IconS[globalPower.pwrCO2],4);
  myFiles.load(178, 205, 24, 24, pwrAux1IconS[globalPower.pwrAux1],4);
  myFiles.load(206, 205, 24, 24, pwrAux2IconS[globalPower.pwrAux2],4);
}

void Opaq_iaqua::screenFeeding()  // draw the feeding screen
{ 
  dispScreen=2;
  //touchWaitTime = LONG_WAIT;
  
  myGLCD.clrScr();
  updateTimeDate(true);

  // draw header and footer
  myGLCD.setColor(130, 130, 130);
  myGLCD.drawLine(40, 31, 239, 31); // under header
  myGLCD.drawLine(0, 307, 239, 307); // at footer
  myFiles.load(2, 2, 30, 30, "2feed.raw",2);
  myGLCD.setFont(arial_bold);
  myGLCD.setColor(0, 184, 19);
  myGLCD.print(F("FEEDING"), 36, 12);
  myFiles.load(107, 294, 26, 26, "foothome.raw",2);

  myGLCD.setFont(arial_bold);
  myGLCD.setColor(0, 184, 19);
  myGLCD.print(F("TIME REMAINING"), CENTER, 60);
  myGLCD.setColor(240, 240, 255);

  // buttons to stop and restart feeding
  myFiles.load(67, 223, 48, 48, "2stop.raw",2);
  myFiles.load(124, 223, 48, 48, "2restart.raw",2);

  // picture of fish eating
  myFiles.load(74, 110, 92, 92, "2feeding.raw",2);

  //checkFeeding();
}

void Opaq_iaqua::screenPower()  // draw the screen to turn power outputs on/off
{ 
  dispScreen=3;
  //touchWaitTime = LONG_WAIT;

  myGLCD.clrScr();
  updateTimeDate(true);

  // draw footer
  myGLCD.setColor(130, 130, 130);
  myGLCD.drawLine(40, 31, 239, 31); // under header
  myGLCD.drawLine(0, 307, 104, 307); // left footer
  myGLCD.drawLine(136, 307, 239, 307); // right footer
  myFiles.load(107, 294, 26, 26, "foothome.raw",2);

  // draw header
  myFiles.load(2, 2, 30, 30, "3power.raw",2);
  myGLCD.setFont(arial_bold);
  myGLCD.setColor(222, 8, 51);
  myGLCD.print(F("POWER"), 36, 12);

  myGLCD.setColor(255, 255, 255);
  myGLCD.print(F("MASTER"), CENTER, 52);

  // all on and all off buttons
  myFiles.load(73, 77, 40, 40, "3allon.raw",2);
  myFiles.load(128, 77, 40, 40, "3alloff.raw",2);

  // load all power icons and power dots
  myFiles.load(15, 139, 48, 48, pwrLightIcon[globalPower.pwrLight1],4);
  myFiles.load(34, 192, 10, 11, pwrDot[globalPower.pwrLight1],4);
  myFiles.load(69, 139, 48, 48, pwrLightIcon[globalPower.pwrLight2],4);
  myFiles.load(88, 192, 10, 11, pwrDot[globalPower.pwrLight2],4);
  myFiles.load(124, 139, 48, 48, pwrFilterIcon[globalPower.pwrFilter],4);
  myFiles.load(143, 192, 10, 11, pwrDot[globalPower.pwrFilter],4);
  myFiles.load(178, 139, 48, 48, pwrCircIcon[globalPower.pwrCirc],4);
  myFiles.load(197, 192, 10, 11, pwrDot[globalPower.pwrCirc],4);
  myFiles.load(15, 212, 48, 48, pwrHeatIcon[globalPower.pwrHeat],4);
  myFiles.load(34, 264, 10, 11, pwrDot[globalPower.pwrHeat],4);
  myFiles.load(69, 212, 48, 48, pwrCO2Icon[globalPower.pwrCO2],4);
  myFiles.load(88, 264, 10, 11, pwrDot[globalPower.pwrCO2],4);
  myFiles.load(124, 212, 48, 48, pwrAux1Icon[globalPower.pwrAux1],4);
  myFiles.load(143, 264, 10, 11, pwrDot[globalPower.pwrAux1],4);
  myFiles.load(178, 212, 48, 48, pwrAux2Icon[globalPower.pwrAux2],4);
  myFiles.load(197, 264, 10, 11, pwrDot[globalPower.pwrAux2],4);
}

void Opaq_iaqua::screenSettings()  // draw the screen that has all of the extra settings apps
{ 
  dispScreen=4;
  //touchWaitTime = LONG_WAIT;

  myGLCD.clrScr();
  updateTimeDate(true);
  
  // draw header
  myFiles.load(2, 2, 30, 30, "4extras.raw",2);
  myGLCD.setFont(arial_bold);
  myGLCD.setColor(255, 77, 0);
  myGLCD.print(F("SETTINGS  "), 36, 12);

  myGLCD.setColor(130, 130, 130);
  myGLCD.drawLine(40, 31, 239, 31); // under header
  myGLCD.drawLine(0, 307, 104, 307); // left footer
  myGLCD.drawLine(136, 307, 239, 307); // right footer

  myFiles.load(107, 294, 26, 26, "foothome.raw",2);  // footer home button

  if ((globalPower.pwrLight1==1)||(lightCSP==false))
  {
    myFiles.load(10, 50, 48, 63, "4lights.raw",2);
  }
  else
  {
    myFiles.load(10, 50, 48, 63, "4lightsF.raw",2);
  }
  myFiles.load(67, 50, 48, 63, "4clock.raw",2);
  myFiles.load(124, 50, 48, 63, "4feeding.raw",2);
  myFiles.load(181, 50, 48, 63, "4sched.raw",2);
  myFiles.load(10, 118, 48, 63, "4sensors.raw",2);
  myFiles.load(67, 118, 48, 63, "4dosing.raw",2);
  myFiles.load(124, 118, 48, 63, "4screen.raw",2);
  myFiles.load(181, 118, 48, 63, "4ato.raw",2);
  myFiles.load(10, 186, 48, 63, "4lunar.raw",2);
  if (lightCSP==true) myFiles.load(67, 186, 48, 63, "4colorF.raw",2);//gray out icon for IR lights
  if (lightCSP==false)myFiles.load(67, 186, 48, 63, "4color.raw",2);
  myFiles.load(124, 186, 48, 63, "4graph.raw",2);
}


void Opaq_iaqua::drawLEDBarGraph(byte barNum, int value)
{ 
  int x,y;
  char char5[5];
  
  Serial.print(F("Bar:"));
  Serial.println(barNum);
  Serial.print(F("Percent:"));
  Serial.println(value);
    
  //determine xy coordinates based on bar being displayed
  if(barNum == 1){x=10;y=185;}
  if(barNum == 2){x=48;y=185;}
  if(barNum == 3){x=86;y=185;}
  if(barNum == 4){x=124;y=185;}
  if(barNum == 5){x=162;y=185;}
  if(barNum == 6){x=200;y=185;}
  
  //erase old bargraph & percentage
  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(x-3,y-10,x+28,y-1);
  myGLCD.fillRect(x+1,y+1,x+27,y+99);
  
  //draw new bargraph
  myGLCD.setColor(VGAColor[barColors[barNum-1]]);
  myGLCD.drawRect(x,y,x+28,y+100);
  myGLCD.fillRect(x,(y+100-value),x+28,(y+100));
  
  //write percentage
  myGLCD.setFont(Sinclair_S);
  myGLCD.setColor(VGA_WHITE);
  itoa(value, char5, 10);
  strcat(char5,"%");
  if(value < 100) x = x + 8;
  if(value < 10) x = x + 8;
  myGLCD.print(char5,x-3,y-10);
}

void Opaq_iaqua::screenLights() // draw the screen for configuring the lights
{ 
  dispScreen=5; 
  //touchWaitTime = LONG_WAIT;

  myGLCD.clrScr();
  updateTimeDate(true);

  // draw header
  myFiles.load(2, 2, 30, 30, "5lights.raw",2);
  myGLCD.setFont(arial_bold);
  myGLCD.setColor(185, 55, 255);
  myGLCD.print(F("LIGHT MODES"), 36, 12);

  myGLCD.setColor(130, 130, 130);
  myGLCD.drawLine(40, 31, 239, 31); // under header
  myGLCD.drawLine(0, 307, 104, 307); // left footer
  myGLCD.drawLine(136, 307, 239, 307); // right footer

  myGLCD.drawLine(0, 94, 239, 94); // vertical center line
  myGLCD.drawLine(0, 168, 239, 168); // vertical center line

  myFiles.load(107, 294, 26, 26, "footextr.raw",2); // footer button

  // draw buttons based on current mode (either selected or not selected)
  if (currentLightMode==0)// high sun
  {
    myFiles.load(10, 39, 48, 48, lightModeS[0],2);
    myFiles.load(10, 39, 48, 48, lightMode[0],2);
    myFiles.load(67, 39, 48, 48, lightMode[1],2);
    myFiles.load(124, 39, 48, 48, lightMode[2],2);
    myFiles.load(181, 39, 48, 48, lightMode[3],2);
    myFiles.load(10, 101, 48, 63, lightEdit[1],2);
    myFiles.load(181, 101, 48, 63, lightResync[1],2);    

    // get RGBW for high sun
    currentColor.chan1=EEPROM.read(400);
    currentColor.chan2=EEPROM.read(401);
    currentColor.chan3=EEPROM.read(402);
    currentColor.chan4=EEPROM.read(403);
    currentColor.chan5=EEPROM.read(404);
    currentColor.chan6=EEPROM.read(405);

    
    // draw bargraphs
    drawLEDBarGraph(1, currentColor.chan1);
    drawLEDBarGraph(2, currentColor.chan2);
    drawLEDBarGraph(3, currentColor.chan3);
    drawLEDBarGraph(4, currentColor.chan4);
    drawLEDBarGraph(5, currentColor.chan5);
    drawLEDBarGraph(6, currentColor.chan6);
    
    if(lightCSP == false)
    {
      if(PCA9685Installed == true)  //if using PCA9685 scale values to 12-bit
      {
        currentColor.chan1 = map(currentColor.chan1,0,100,0,4095);
        currentColor.chan2 = map(currentColor.chan2,0,100,0,4095);
        currentColor.chan3 = map(currentColor.chan3,0,100,0,4095);
        currentColor.chan4 = map(currentColor.chan4,0,100,0,4095);
        currentColor.chan5 = map(currentColor.chan5,0,100,0,4095);
        currentColor.chan6 = map(currentColor.chan6,0,100,0,4095);
      } else  //otherwise scale to 8-bit
      {
        currentColor.chan1 = map(currentColor.chan1,0,100,0,255);
        currentColor.chan2 = map(currentColor.chan2,0,100,0,255);
        currentColor.chan3 = map(currentColor.chan3,0,100,0,255);
        currentColor.chan4 = map(currentColor.chan4,0,100,0,255);
        currentColor.chan5 = map(currentColor.chan5,0,100,0,255);
        currentColor.chan6 = map(currentColor.chan6,0,100,0,255);
      }
    }
  }  
  else if (currentLightMode==1) // mid sun
  {
    myFiles.load(10, 39, 48, 48, lightMode[0],2);
    myFiles.load(67, 39, 48, 48, lightModeS[1],2);
    myFiles.load(124, 39, 48, 48, lightMode[2],2);
    myFiles.load(181, 39, 48, 48, lightMode[3],2);
    myFiles.load(10, 101, 48, 63, lightEdit[1],2);
    myFiles.load(181, 101, 48, 63, lightResync[1],2);    

    // get RGBW for mid sun
    currentColor.chan1=EEPROM.read(410);
    currentColor.chan2=EEPROM.read(411);
    currentColor.chan3=EEPROM.read(412);
    currentColor.chan4=EEPROM.read(413);
    currentColor.chan5=EEPROM.read(414);
    currentColor.chan6=EEPROM.read(415);

    // draw bargraphs
    drawLEDBarGraph(1, currentColor.chan1);
    drawLEDBarGraph(2, currentColor.chan2);
    drawLEDBarGraph(3, currentColor.chan3);
    drawLEDBarGraph(4, currentColor.chan4);
    drawLEDBarGraph(5, currentColor.chan5);
    drawLEDBarGraph(6, currentColor.chan6);
    
    if(lightCSP == false)
    {
      if(PCA9685Installed == true)  //if using PCA9685 scale values to 12-bit
      {
        currentColor.chan1 = map(currentColor.chan1,0,100,0,4095);
        currentColor.chan2 = map(currentColor.chan2,0,100,0,4095);
        currentColor.chan3 = map(currentColor.chan3,0,100,0,4095);
        currentColor.chan4 = map(currentColor.chan4,0,100,0,4095);
        currentColor.chan5 = map(currentColor.chan5,0,100,0,4095);
        currentColor.chan6 = map(currentColor.chan6,0,100,0,4095);
      } else  //otherwise scale to 8-bit
      {
        currentColor.chan1 = map(currentColor.chan1,0,100,0,255);
        currentColor.chan2 = map(currentColor.chan2,0,100,0,255);
        currentColor.chan3 = map(currentColor.chan3,0,100,0,255);
        currentColor.chan4 = map(currentColor.chan4,0,100,0,255);
        currentColor.chan5 = map(currentColor.chan5,0,100,0,255);
        currentColor.chan6 = map(currentColor.chan6,0,100,0,255);
      }
    }
  }
  else if (currentLightMode==2) // low sun
  {
    myFiles.load(10, 39, 48, 48, lightMode[0],2);
    myFiles.load(67, 39, 48, 48, lightMode[1],2);
    myFiles.load(124, 39, 48, 48, lightModeS[2],2);
    myFiles.load(181, 39, 48, 48, lightMode[3],2);
    myFiles.load(10, 101, 48, 63, lightEdit[1],2);
    myFiles.load(181, 101, 48, 63, lightResync[1],2);    

    // get RGBW for low sun
    currentColor.chan1=EEPROM.read(420);
    currentColor.chan2=EEPROM.read(421);
    currentColor.chan3=EEPROM.read(422);
    currentColor.chan4=EEPROM.read(423);
    currentColor.chan5=EEPROM.read(424);
    currentColor.chan6=EEPROM.read(425);

    // draw bargraphs
    drawLEDBarGraph(1, currentColor.chan1);
    drawLEDBarGraph(2, currentColor.chan2);
    drawLEDBarGraph(3, currentColor.chan3);
    drawLEDBarGraph(4, currentColor.chan4);
    drawLEDBarGraph(5, currentColor.chan5);
    drawLEDBarGraph(6, currentColor.chan6);
    
    if(lightCSP == false)
    {
      if(PCA9685Installed == true)  //if using PCA9685 scale values to 12-bit
      {
        currentColor.chan1 = map(currentColor.chan1,0,100,0,4095);
        currentColor.chan2 = map(currentColor.chan2,0,100,0,4095);
        currentColor.chan3 = map(currentColor.chan3,0,100,0,4095);
        currentColor.chan4 = map(currentColor.chan4,0,100,0,4095);
        currentColor.chan5 = map(currentColor.chan5,0,100,0,4095);
        currentColor.chan6 = map(currentColor.chan6,0,100,0,4095);
      } else  //otherwise scale to 8-bit
      {
        currentColor.chan1 = map(currentColor.chan1,0,100,0,255);
        currentColor.chan2 = map(currentColor.chan2,0,100,0,255);
        currentColor.chan3 = map(currentColor.chan3,0,100,0,255);
        currentColor.chan4 = map(currentColor.chan4,0,100,0,255);
        currentColor.chan5 = map(currentColor.chan5,0,100,0,255);
        currentColor.chan6 = map(currentColor.chan6,0,100,0,255);
      }
    }
  }
  else if (currentLightMode==3) // moon
  {
    myFiles.load(10, 39, 48, 48, lightMode[0],2);
    myFiles.load(67, 39, 48, 48, lightMode[1],2);
    myFiles.load(124, 39, 48, 48, lightMode[2],2);
    myFiles.load(181, 39, 48, 48, lightModeS[3],2);
    myFiles.load(10, 101, 48, 63, lightEdit[1],2);
    myFiles.load(181, 101, 48, 63, lightResync[1],2);

    // get RGBW for moon
    currentColor.chan1=EEPROM.read(430);
    currentColor.chan2=EEPROM.read(431);
    currentColor.chan3=EEPROM.read(432);
    currentColor.chan4=EEPROM.read(433);
    currentColor.chan5=EEPROM.read(434);
    currentColor.chan6=EEPROM.read(435);

    // draw bargraphs
    drawLEDBarGraph(1, currentColor.chan1);
    drawLEDBarGraph(2, currentColor.chan2);
    drawLEDBarGraph(3, currentColor.chan3);
    drawLEDBarGraph(4, currentColor.chan4);
    drawLEDBarGraph(5, currentColor.chan5);
    drawLEDBarGraph(6, currentColor.chan6);
    
    if(lightCSP == false)
    {
      if(PCA9685Installed == true)  //if using PCA9685 scale values to 12-bit
      {
        currentColor.chan1 = map(currentColor.chan1,0,100,0,4095);
        currentColor.chan2 = map(currentColor.chan2,0,100,0,4095);
        currentColor.chan3 = map(currentColor.chan3,0,100,0,4095);
        currentColor.chan4 = map(currentColor.chan4,0,100,0,4095);
        currentColor.chan5 = map(currentColor.chan5,0,100,0,4095);
        currentColor.chan6 = map(currentColor.chan6,0,100,0,4095);
      } else  //otherwise scale to 8-bit
      {
        currentColor.chan1 = map(currentColor.chan1,0,100,0,255);
        currentColor.chan2 = map(currentColor.chan2,0,100,0,255);
        currentColor.chan3 = map(currentColor.chan3,0,100,0,255);
        currentColor.chan4 = map(currentColor.chan4,0,100,0,255);
        currentColor.chan5 = map(currentColor.chan5,0,100,0,255);
        currentColor.chan6 = map(currentColor.chan6,0,100,0,255);
      }
    }
  }
  else if ((currentLightMode==4)||(currentLightMode==5)) // lights in transition or unknown
  {
    myFiles.load(10, 39, 48, 48, lightMode[0],2);
    myFiles.load(67, 39, 48, 48, lightMode[1],2);
    myFiles.load(124, 39, 48, 48, lightMode[2],2);
    myFiles.load(181, 39, 48, 48, lightMode[3],2);
    myFiles.load(10, 101, 48, 63, lightEdit[0],2);
    myFiles.load(181, 101, 48, 63, lightResync[1],2);
    
    if(lightCSP == true)
    {
      // draw bargraphs
      drawLEDBarGraph(1, currentColor.chan1);
      drawLEDBarGraph(2, currentColor.chan2);
      drawLEDBarGraph(3, currentColor.chan3);
      drawLEDBarGraph(4, currentColor.chan4);
      drawLEDBarGraph(5, 0);
      drawLEDBarGraph(6, 0);
    }else
    {
      byte temp1,temp2,temp3,temp4,temp5,temp6;
      if(PCA9685Installed == true)  //if using PCA9685 scale values to 12-bit
      {
        temp1 = map(currentColor.chan1,0,4095,0,100);
        temp2 = map(currentColor.chan2,0,4095,0,100);
        temp3 = map(currentColor.chan3,0,4095,0,100);
        temp4 = map(currentColor.chan4,0,4095,0,100);
        temp5 = map(currentColor.chan5,0,4095,0,100);
        temp6 = map(currentColor.chan6,0,4095,0,100);
      }else
      {
        temp1 = map(currentColor.chan1,0,255,0,100);
        temp2 = map(currentColor.chan2,0,255,0,100);
        temp3 = map(currentColor.chan3,0,255,0,100);
        temp4 = map(currentColor.chan4,0,255,0,100);
        temp5 = map(currentColor.chan5,0,255,0,100);
        temp6 = map(currentColor.chan6,0,255,0,100);
      }
      // draw bargraphs
      drawLEDBarGraph(1, temp1);
      drawLEDBarGraph(2, temp2);
      drawLEDBarGraph(3, temp3);
      drawLEDBarGraph(4, temp4);
      drawLEDBarGraph(5, temp5);
      drawLEDBarGraph(6, temp6);
    }
  }

  // draw the rest of the buttons disabled until the edit button is pressed
  myFiles.load(67, 101, 48, 63, lightSave[0],2);
  myFiles.load(124, 101, 48, 63, lightCancel[0],2);
  
  tempColor = currentColor;
  myGLCD.setFont(Sinclair_S);
  myGLCD.setColor(255, 255, 255);
}

void Opaq_iaqua::screenClock()  // draw the screen for setting the date/time
{ 
  dispScreen=6; 

  myGLCD.clrScr();
  updateTimeDate(true);

  // draw header
  myFiles.load(2, 2, 30, 30, "6clock.raw",2);
  myGLCD.setFont(arial_bold);
  myGLCD.setColor(47, 168, 208);
  myGLCD.print(F("CLOCK"), 36, 12);

  myGLCD.print(F("TIME"), CENTER, 46);
  myGLCD.print(F("DATE"), CENTER, 167);

  myGLCD.setColor(130, 130, 130);
  myGLCD.drawLine(40, 31, 239, 31); // under header
  myGLCD.drawLine(0, 155, 239, 155); // center line

  myGLCD.setFont(Sinclair_S);
  myGLCD.setColor(255, 255, 255);

  // draw clock buttons and text
  myGLCD.print("24H", 12, 72);
  myFiles.load(12, 89, 24, 24, arrowButton[0],2);
  myFiles.load(12, 119, 24, 24, arrowButton[1],2);
  myGLCD.print("M", 91, 72);
  myFiles.load(83, 89, 24, 24, arrowButton[0],2);
  myFiles.load(83, 119, 24, 24, arrowButton[1],2);
  myGLCD.print("S", 172, 72);
  myFiles.load(164, 89, 24, 24, arrowButton[0],2);
  myFiles.load(164, 119, 24, 24, arrowButton[1],2);

  // draw date buttons and text
  myGLCD.print("M", 20, 194);
  myFiles.load(12, 211, 24, 24, arrowButton[0],2);
  myFiles.load(12, 241, 24, 24, arrowButton[1],2);
  myGLCD.print("D", 91, 194);
  myFiles.load(83, 211, 24, 24, arrowButton[0],2);
  myFiles.load(83, 241, 24, 24, arrowButton[1],2);
  myGLCD.print("Y", 172, 194);
  myFiles.load(164, 211, 24, 24, arrowButton[0],2);
  myFiles.load(164, 241, 24, 24, arrowButton[1],2);

  myGLCD.setFont(arial_bold);
  myGLCD.setColor(255, 77, 0);

  saveRTC.Hour = hour();
  saveRTC.Minute = minute(); 
  saveRTC.Second = 0; // always have 0 seconds
  saveRTC.Day = day();
  saveRTC.Month = month();
  saveRTC.Year = (year()-1970);

  char char3[3];
  char char3t[3];

  // draw the date and time to the screen

  itoa(saveRTC.Hour, char3, 10);
  if (saveRTC.Hour>=0 && saveRTC.Hour<=9) // add a zero
  {
    itoa(0, char3t, 10); //make char3t 0
    strcat(char3t, char3);
    strcpy (char3,char3t);
  }
  myGLCD.print(char3, 45, 108);

  itoa(saveRTC.Minute, char3, 10);
  if (saveRTC.Minute>=0 && saveRTC.Minute<=9) // add a zero
  {
    itoa(0, char3t, 10); //make char3t 0
    strcat(char3t, char3);
    strcpy (char3,char3t);
  }
  myGLCD.print(char3, 123, 108);

  myGLCD.print("00", 201, 108);

  itoa(saveRTC.Month, char3, 10);
  if (saveRTC.Month>=0 && saveRTC.Month<=9) // add a zero
  {
    itoa(0, char3t, 10); //make char3t 0
    strcat(char3t, char3);
    strcpy (char3,char3t);
  }
  myGLCD.print(char3, 45, 230);

  itoa(saveRTC.Day, char3, 10);
  if (saveRTC.Day>=0 && saveRTC.Day<=9) // add a zero
  {
    itoa(0, char3t, 10); //make char3t 0
    strcat(char3t, char3);
    strcpy (char3,char3t);
  }
  myGLCD.print(char3, 123, 230);

  //saveRTC.Year is offset from 1970 so we subtract 30 from number to get offset from 2000
  itoa((saveRTC.Year-30), char3, 10);
  if (((saveRTC.Year-30) >= 0) && ((saveRTC.Year-30) <= 9)) // add a zero
  {
    itoa(0, char3t, 10); //make char3t 0
    strcat(char3t, char3);
    strcpy (char3,char3t);
  }
  myGLCD.print(char3, 201, 230);

  // buttons to save or cancel
  myFiles.load(12, 286, 84, 26, "6cancel.raw",2);
  myFiles.load(144, 286, 84, 26, "6set.raw",2);

}

void Opaq_iaqua::screenFeedSettings() // screen for setting the minutes and power settings for feeding
{ 
  dispScreen=7;
  //touchWaitTime = LONG_WAIT;

  myGLCD.clrScr();
  updateTimeDate(true);

  // draw header
  myFiles.load(2, 2, 30, 30, "2feed.raw",2);
  myGLCD.setFont(arial_bold);
  myGLCD.setColor(0, 184, 19);
  myGLCD.print(F("FEED CONFIG"), 36, 12);
  myGLCD.print(F("MINUTES"), 24, 65);

  // buttons for changing minutes
  myFiles.load(158, 46, 24, 24, arrowButton[0],2);
  myFiles.load(158, 76, 24, 24, arrowButton[1],2);

  myGLCD.setColor(255, 77, 0);

  // display current setting for minutes
  char char3[3];
  itoa(feedingMins, char3, 10);
  myGLCD.print(char3, 190, 65);

  myGLCD.setColor(222, 8, 51);
  myGLCD.print(F("POWER SETUP"), CENTER, 121);

  myGLCD.setColor(130, 130, 130);

  myGLCD.drawLine(40, 31, 239, 31); // under header
  myGLCD.drawLine(0, 129, 32, 129); // LEFT OF POWER
  myGLCD.drawLine(208, 129, 239, 129); // Right of power
  myGLCD.drawLine(0, 307, 104, 307); // left footer
  myGLCD.drawLine(136, 307, 239, 307); // right footer
  myFiles.load(107, 294, 26, 26, "footextr.raw",2);

  // Feeding settings stored in EEPROM
  // 6 // feeding settings saved (0 for no, 1 for yes)
  // 7 // feeding minutes setting
  // 8 // feeding  light 1 (0 off, 1 on)
  // 9 // feeding pwr light 2 (0 off, 1 on)
  // 10 // feeding pwr filter (0 off, 1 on)
  // 11 // feeding pwr circ (0 off, 1 on)
  // 12 // feeding pwr heat (0 off, 1 on)
  // 13 // feeding pwr co2 (0 off, 1 on)
  // 14 // feeding pwr aux 1 (0 off, 1 on)
  // 15 // feeding pwr aux 2 (0 off, 1 on)

  // load power icons based on saved settings
  myFiles.load(69, 151, 48, 48, pwrFilterIcon[feedPower.pwrFilter],2);
  myFiles.load(88, 204, 10, 11, pwrDot[feedPower.pwrFilter],2);
  myFiles.load(124, 151, 48, 48, pwrCircIcon[feedPower.pwrCirc],2);
  myFiles.load(143, 204, 10, 11, pwrDot[feedPower.pwrCirc],2);
  myFiles.load(69, 224, 48, 48, pwrAux1Icon[feedPower.pwrAux1],2);
  myFiles.load(88, 276, 10, 11, pwrDot[feedPower.pwrAux1],2);
  myFiles.load(124, 224, 48, 48, pwrAux2Icon[feedPower.pwrAux2],2);
  myFiles.load(143, 276, 10, 11, pwrDot[feedPower.pwrAux2],2);
}

void Opaq_iaqua::screenHeater() // screen for choosing temp settings for the heater
{ 
  dispScreen=8;
  
  char char6[6]; // used to convernt numbers to char

  myGLCD.clrScr();
  updateTimeDate(true);

  // draw header
  myFiles.load(2, 2, 30, 30, "8heat.raw",2);
  myGLCD.setFont(arial_bold);
  myGLCD.setColor(34, 81, 255);
  myGLCD.print(F("HEATER SETUP"), 36, 12);

  myGLCD.setColor(130, 130, 130);  
  myGLCD.drawLine(40, 31, 239, 31); // under header
  myGLCD.drawLine(0, 307, 104, 307); // left footer
  myGLCD.drawLine(136, 307, 239, 307); // right footer
  myFiles.load(107, 294, 26, 26, "footextr.raw",2);
  
  // lines to divide screen
  myGLCD.drawLine(0, 97, 239, 97); 
  myGLCD.drawLine(0, 163, 239, 163);
  myGLCD.drawLine(0, 229, 239, 229);
  
  // buttons for adjusting off temp
  myFiles.load(162, 37, 24, 24, arrowButton[0],2);
  myFiles.load(162, 67, 24, 24, arrowButton[1],2);

  // buttons for adjusting the on temp
  myFiles.load(162, 103, 24, 24, arrowButton[0],2);
  myFiles.load(162, 133, 24, 24, arrowButton[1],2);
  
  // buttons for adjusting calibration
  myFiles.load(136, 169, 24, 24, arrowButton[0],2);
  myFiles.load(136, 199, 24, 24, arrowButton[1],2);

  // off icon
  myFiles.load(17, 40, 48, 48, "8off.raw",2);
  myGLCD.setColor(222, 8, 51);
  myGLCD.print(F("HEATER"), 64, 42);
  myGLCD.print(F("OFF"), 88, 69);

  // get the setting for heat off and print it
  heatOffTemp = (EEPROM.read(17)); // 17 // heater off temp
  itoa(heatOffTemp, char6, 10);
  myGLCD.setColor(255, 77, 0);
  myGLCD.print(char6, 193, 56);
  if(displayInC == true) myFiles.load(226, 58, 14, 12, "c.raw",2);
  if(displayInC == false)myFiles.load(226, 58, 14, 12, "f.raw",2);

  // draw the on icon
  myFiles.load(17, 106, 48, 48, "8on.raw",2);
  myGLCD.setColor(222, 8, 51);
  myGLCD.print(F("HEATER"), 64, 108);
  myGLCD.print(F("ON"), 96, 135);

  // get the setting for heat on and print it
  heatOnTemp = (EEPROM.read(18));
  itoa(heatOnTemp, char6, 10); // 18 // heater on temp
  myGLCD.setColor(255, 77, 0);
  myGLCD.print(char6, 195, 122);
  if(displayInC == true) myFiles.load(226, 124, 14, 12, "c.raw",2);
  if(displayInC == false)myFiles.load(226, 124, 14, 12, "f.raw",2);

  // calibration offset
  myGLCD.setColor(VGA_WHITE);
  myGLCD.print(F("Sensor"),10,174);
  myGLCD.print(F("Offset"),10,201);
  
  // display value to screen
  byte storedVal = EEPROM.read(58);
  byte dx = 176;
  if(storedVal < 128)dx = 160;
  sensor1Calibration = ((float)storedVal - 128) / 10;
  dtostrf(sensor1Calibration, 3, 1, char6);  //convert to string
  myGLCD.setColor(255, 77, 0);
  myGLCD.print(char6, dx, 188);
  
  // display units on screen
  if(displayInC == false)myFiles.load(226, 190, 14, 12, "f.raw",2);
  if(displayInC == true) myFiles.load(226, 190, 14, 12, "c.raw",2);
  
  // enable/disable display of sensor
  myGLCD.setColor(VGA_WHITE);
  myGLCD.print(F("Display?"),10,254);
  myFiles.load(170, 247, 30, 30, schedActiveB[displaySensor1],2);
}

void Opaq_iaqua::screenSchedule() // screen with icons to allow setting schedules
{ 
  dispScreen=9;
  //touchWaitTime = LONG_WAIT;
  
  myGLCD.clrScr();
  updateTimeDate(true);

  // draw header
  myFiles.load(2, 2, 30, 30, "9sched.raw",2);
  myGLCD.setFont(arial_bold);
  myGLCD.setColor(238, 0, 145);
  myGLCD.print(F("SCHEDULE"), 36, 12);

  myGLCD.setColor(130, 130, 130);  
  myGLCD.drawLine(40, 31, 239, 31); // under header
  myGLCD.drawLine(0, 307, 104, 307); // left footer
  myGLCD.drawLine(136, 307, 239, 307); // right footer
  myFiles.load(107, 294, 26, 26, "footextr.raw",2);

  // draw schedule icons
  myFiles.load(10, 50, 48, 63, "9power.raw",2);
  myFiles.load(67, 50, 48, 63, "4dosing.raw",2);
  myFiles.load(124, 50, 48, 63, "4lights.raw",2);
}

void Opaq_iaqua::screenDosing(byte pumpNo)  // screen to configure the 3 dosing pumps
{ 
  scheduleItem=pumpNo;
  dispScreen=10;
  //touchWaitTime = LONG_WAIT;
  
  char char5[5];

  myGLCD.clrScr();
  updateTimeDate(true);

  // draw header
  myFiles.load(2, 2, 30, 30, "10dose.raw",2);
  myGLCD.setFont(arial_bold);
  myGLCD.setColor(138, 93, 35);
  myGLCD.print(F("DOSING SETUP"), 36, 12);

  myGLCD.setColor(130, 130, 130);  
  myGLCD.drawLine(40, 31, 239, 31); // under header
  myGLCD.drawLine(0, 307, 104, 307); // left footer
  myGLCD.drawLine(136, 307, 239, 307); // right footer
  myFiles.load(107, 293, 26, 26, "footextr.raw",2);

  // 3 horizontal separater lines
  myGLCD.drawLine(0, 97, 239, 97);
  myGLCD.drawLine(0, 165, 239, 165);
  myGLCD.drawLine(0, 233, 239, 233);
  
  //Display pump selectors
  myFiles.load(12, 40, 46, 48,"15pump1.raw",2);
  myFiles.load(70, 40, 46, 48,"15pump2.raw",2);
  myFiles.load(128, 40, 46, 48,"15pump3.raw",2);
  
  //Highlight selected pump
  myGLCD.setColor(VGA_WHITE); 
  if(pumpNo == 1) myGLCD.drawRoundRect(10,38,60,90);//x1,y1,x2,y2
  if(pumpNo == 2) myGLCD.drawRoundRect(68,38,118,90);//x1,y1,x2,y2
  if(pumpNo == 3) myGLCD.drawRoundRect(126,38,176,90);//x1,y1,x2,y2

  // DOSE AMOUNT
  myGLCD.setColor(255, 255, 255);
  myGLCD.print(F("DOSE"), 4, 109);
  myGLCD.print(F("AMOUNT"), 4, 135);

  // buttons to adjust dose amount
  myFiles.load(145, 105, 24, 24, arrowButton[0],2);
  myFiles.load(145, 135, 24, 24, arrowButton[1],2);

  // read dose amount setting and print it
  byte doseAmt;
  if (pumpNo==1)
  {
    doseAmt=EEPROM.read(20); // 20 // pump1 dose in mL
  }
  else if (pumpNo==2)
  {
    doseAmt=EEPROM.read(21); // 21 // pump2 dose in mL
  }
  else if (pumpNo==3)
  {
    doseAmt=EEPROM.read(22); // 22 // pump3 dose in mL
  }
  itoa(doseAmt, char5, 10);
  myGLCD.setColor(255, 77, 0);
  myGLCD.print(char5, 175, 125);

  //PUMP SPEED
  myGLCD.setColor(255, 255, 255);
  myGLCD.print(F("PUMP"), 4, 180);
  myGLCD.print(F("SPEED"), 4, 207);
  
  // buttons to adjust pump time
  myFiles.load(145, 173, 24, 24, arrowButton[0],2);
  myFiles.load(145, 203, 24, 24, arrowButton[1],2);

  // read in pump time and display it in seconds
  int pumpTime;
  int doseCap;
  if (pumpNo==1)
  {
    pumpTime=EEPROM.read(23); // 23 // pump 1 sec/ml
  }
  else if (pumpNo==2)
  {
    pumpTime=EEPROM.read(24); // 24 // pump 1 sec/ml
  }
  else if (pumpNo==3)
  {
    pumpTime=EEPROM.read(25); // 25 // pump 1 sec/ml
  }
  pumpTime=pumpTime*10;
  itoa(pumpTime, char5, 10);
  myGLCD.setColor(255, 77, 0);
  myGLCD.setFont(arial_bold);
  myGLCD.print(char5, 175, 192);  // 175 if 4 chars

  // PUMP test button and calibration
  myFiles.load(92, 175, 48, 48, "10test.raw",2);

  //Reservoir size
  myGLCD.setFont(arial_bold);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print(F("FULL"),5,244);
  myGLCD.print(F("VOL."),5,270);

  if (pumpNo==1)
  {
    doseCap=EEPROM.read(26); // 26 // dosing 1 resevior capacity in mL;
  }
  if (pumpNo==2)
  {
    doseCap=EEPROM.read(261); // 27 // dosing 2 resevior capacity in mL;
  } 
  if (pumpNo==3)
  {
    doseCap=EEPROM.read(263); // 28 // dosing 3 resevior capacity in mL;    
  }
  doseCap=doseCap*10; // saved value is ^10
  itoa(doseCap, char5, 10);
  myGLCD.setColor(255, 77, 0);
  myGLCD.setFont(arial_bold);
  myGLCD.print(char5, 175, 259);
  
  // buttons to adjust reservoir
  myFiles.load(145, 240, 24, 24, arrowButton[0],2);
  myFiles.load(145, 270, 24, 24, arrowButton[1],2);
  
  // fill reservoir icon
  myFiles.load(92, 242, 48, 48, "10fill.raw",2);
  
  // units of measurement labels
  myGLCD.setFont(Sinclair_S);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print(F("mL"), 195, 150);
  myGLCD.print(F("mL"), 195, 280);
  myGLCD.print(F("ms/mL)"), 195, 215);
}

void Opaq_iaqua::screenScreen() // this is the screen for setting brightness settings
{
  dispScreen=16; 
  char char3[3]; // used to convernt numbers to char
  char char4[4]; // used to convernt numbers to char

  myGLCD.clrScr();
  updateTimeDate(true);

  // draw header
  myFiles.load(2, 2, 30, 30, "16screen.raw");
  myGLCD.setFont(arial_bold);
  myGLCD.setColor(255, 77, 0);
  myGLCD.print(F("SCREEN"), 36, 12);

  myGLCD.setColor(130, 130, 130);  
  myGLCD.drawLine(40, 31, 239, 31); // under header

  // 3 horizontal separater lines
  myGLCD.drawLine(0, 102, 239, 102);
  myGLCD.drawLine(0, 170, 239, 170);
  myGLCD.drawLine(0, 238, 239, 238);

  // draw return home labels
  myGLCD.setColor(255, 255, 255);
  myGLCD.print(F("RETURN"), 4, 49);
  myGLCD.print(F("TO HOME"), 4, 70);
  myGLCD.setFont(Sinclair_S);
  myGLCD.print(F("AFTER LAST TOUCH"), 8, 90);

  // return to home up/down buttons
  myFiles.load(145, 42, 24, 24, arrowButton[0]);
  myFiles.load(145, 72, 24, 24, arrowButton[1]);

  // draw return home setting
  myGLCD.setFont(arial_bold);
  itoa(screenRetHome, char3, 10);
  myGLCD.setColor(255, 77, 0);
  myGLCD.print(char3, 191, 59);

  // auto dim labels
  myGLCD.setColor(255, 255, 255);
  myGLCD.print(F("AUTO-DIM"), 4, 112);
  myGLCD.print(F("LEVEL"), 4, 133);
  myGLCD.setFont(Sinclair_S);
  myGLCD.print(F("0 TO 10 (0=OFF)"), 6, 154);

  // auto dim up/down
  myFiles.load(145, 110, 24, 24, arrowButton[0]);
  myFiles.load(145, 140, 24, 24, arrowButton[1]);

  // auto dim setting
  itoa(screenDimLevel, char3, 10);
  myGLCD.setColor(255, 77, 0);
  myGLCD.setFont(arial_bold);
  myGLCD.print(char3, 191, 127);

  // auto dim seconds labels
  myGLCD.setColor(255, 255, 255);
  myGLCD.print(F("AUTO-DIM"), 4, 181);
  myGLCD.print(F("SECONDS"), 4, 202);
  myGLCD.setFont(Sinclair_S);
  myGLCD.print(F("AFTER LAST TOUCH"), 6, 223);

  // auto dim seconds up/down
  myFiles.load(145, 178, 24, 24, arrowButton[0]);
  myFiles.load(145, 208, 24, 24, arrowButton[1]);

  // auto dim seconds setting
  itoa(screenDimSec, char4, 10);
  myGLCD.setColor(255, 77, 0);
  myGLCD.setFont(arial_bold);
  myGLCD.print(char4, 191, 197);

  // brightness labels
  myGLCD.setColor(255, 255, 255);
  myGLCD.print(F("BRIGHT"), 4, 249);
  myGLCD.setFont(Sinclair_S);
  myGLCD.print(F("IF NOT USING"), 6, 270);
  myGLCD.print(F("AUTO-DIM"), 6, 283);

  // brightness up/down
  myFiles.load(145, 245, 24, 24, arrowButton[0]);
  myFiles.load(145, 275, 24, 24, arrowButton[1]);

  // brightness setting
  itoa(screenBrightMem, char4, 10);
  myGLCD.setColor(255, 77, 0);
  myGLCD.setFont(arial_bold);
  myGLCD.print(char4, 187, 264);

  // labels under each setting
  myGLCD.setFont(Sinclair_S);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print(F("MIN"), 195, 83);
  myGLCD.print(F("5 AVG"), 187, 151);
  myGLCD.print(F("SEC"), 195, 221);
  myGLCD.print(F("1-255"), 187, 288);

  myGLCD.setColor(130, 130, 130);  
  myGLCD.drawLine(0, 307, 104, 307); // left footer
  myGLCD.drawLine(136, 307, 239, 307); // right footer
  myFiles.load(107, 294, 26, 26, "footextr.raw");
}

void Opaq_iaqua::screenATO() // screen for editing the ATO settings
{
  dispScreen=17;
  
  myGLCD.clrScr();
  updateTimeDate(true);

  char char3[4];
  char char3t[4];

  // draw header
  myFiles.load(2, 2, 30, 30, "17ATO.raw",2);
  myGLCD.setFont(arial_bold);
  myGLCD.setColor(238, 0, 145);
  myGLCD.print(F("ATO Settings"), 36, 12);

  myGLCD.setColor(130, 130, 130);  
  myGLCD.drawLine(40, 31, 239, 31); // under header
  myGLCD.drawLine(0, 307, 104, 307); // left footer
  myGLCD.drawLine(136, 307, 239, 307); // right footer
  myFiles.load(107, 294, 26, 26, "footextr.raw",2);
  
  // Low level time buttons
  myFiles.load(150, 42, 24, 24, arrowButton[0],2);
  myFiles.load(150, 72, 24, 24, arrowButton[1],2);
  
  // draw low level time label
  myGLCD.setFont(arial_bold);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print(F("LOW LEVEL"), 2, 49);
  myGLCD.print(F("TIME"), 4, 70);
  myGLCD.setFont(Sinclair_S);
  myGLCD.print(F("BEFORE TOP OFF"), 8, 90);
  myGLCD.setFont(arial_bold);
  
  // Max run time buttons
  myFiles.load(150, 110, 24, 24, arrowButton[0],2);
  myFiles.load(150, 140, 24, 24, arrowButton[1],2);
  
  myGLCD.setFont(arial_bold);
  myGLCD.print(F("MAX RUN"), 2, 112);
  myGLCD.print(F("TIME"), 4, 133);

  //Labels for enable buttons
  myGLCD.print(F("ATO SYSTEM"), 8, 188);
  myGLCD.print(F("ENABLED"), 8, 209);
  
  myGLCD.print(F("RESERVOIR"), 8, 244);
  myGLCD.print(F("SWITCH"), 8, 265);
  
  //display low level wait time
  byte waitTime=EEPROM.read(42); // 42 // Low level wait time
  byte dx = 0;
  if(waitTime < 9) dx += 16;
  if(waitTime < 99) dx += 16;
  itoa(waitTime, char3, 10);
  myGLCD.setColor(255, 77, 0);
  myGLCD.print(char3, 180+dx, 59);
  
  //display low level wait time
  int maxRunTime=EEPROM.read(43); // 43 // Max run-time
  maxRunTime=maxRunTime*10;
  dx = 0;
  if(maxRunTime < 9)  dx += 16;
  if(maxRunTime < 99) dx += 16;
  itoa(maxRunTime, char3, 10);
  //myGLCD.setColor(255, 77, 0);
  myGLCD.print(char3, 180+dx, 127);
  
  byte enableATO = EEPROM.read(40);
  byte enableSwitch = EEPROM.read(41);
  myFiles.load(180, 196, 30, 30, schedActiveB[enableATO],2); //ATO Enabled
  myFiles.load(180, 249, 30, 30, schedActiveB[enableSwitch],2); //Reservoir switch enabled
  
  //draw units
  myGLCD.setFont(Sinclair_S);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print(F("MIN"), 190, 83);
  myGLCD.print(F("SEC"), 190, 152);
}

void Opaq_iaqua::screenSensors()
{ 
  dispScreen = 18;
  touchWaitTime = LONG_WAIT;
  
  myGLCD.clrScr();
  updateTimeDate(true);
  
  // draw header
  myFiles.load(2, 2, 30, 30, "18sense.raw",2);
  myGLCD.setFont(arial_bold);
  myGLCD.setColor(238, 0, 145);
  myGLCD.print(F("Sensors"), 36, 12);
  
  myGLCD.setColor(130, 130, 130);
  myGLCD.drawLine(40, 31, 239, 31); // under header
  myGLCD.drawLine(0, 307, 104, 307); // left footer
  myGLCD.drawLine(136, 307, 239, 307); // right footer
  myFiles.load(107, 294, 26, 26, "foothome.raw",2);
  
  // Sensor 1-3 names
  myGLCD.setFont(arial_bold);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print(F("Sensor 1"), 2, 55);
  myGLCD.print(F("Sensor 2"), 2, 108);
  myGLCD.print(F("Sensor 3"), 2, 161);
  myGLCD.print(F("Display units"), 2, 214);
  
  // Sensor descriptions
  //myGLCD.setFont(Sinclair_S);
  myGLCD.setColor(255, 0, 77);
  myGLCD.print(F("Water"), 15, 75);
  myGLCD.print(F("Heatsink"), 15, 128);
  myGLCD.print(F("Internal"), 15, 181);
  
  sensor1Enabled = EEPROM.read(50);
  sensor2Enabled = EEPROM.read(51);
  sensor3Enabled = EEPROM.read(52);
  displayInC = EEPROM.read(57);
  // Enable/disable buttons
  myFiles.load(170, 55, 30, 30, schedActiveB[sensor1Enabled],2);
  myFiles.load(170, 108, 30, 30, schedActiveB[sensor2Enabled],2);
  myFiles.load(170, 161, 30, 30, schedActiveB[sensor3Enabled],2);
  
  //display units
  myFiles.load(100, 259, 14, 12, "c.raw",2);
  myFiles.load(180, 259, 14, 12, "f.raw",2);
  myFiles.load(60, 250, 30, 30, schedActiveB[displayInC],2);
  myFiles.load(140, 250, 30, 30, schedActiveB[!displayInC],2);
  
  //settings buttons
  if(sensor1Enabled) myFiles.load(210, 58, 26, 26, "footextr.raw",2);
  if(sensor2Enabled) myFiles.load(210, 111, 26, 26, "footextr.raw",2);
  if(sensor3Enabled) myFiles.load(210, 164, 26, 26, "footextr.raw",2);
}

void Opaq_iaqua::screenLunar()
{
  dispScreen=21;

  myGLCD.clrScr();
  updateTimeDate(true);

  // draw header
  myFiles.load(2, 2, 30, 30, "21moon.raw");
  myGLCD.setFont(arial_bold);
  myGLCD.setColor(185, 55, 255);
  myGLCD.print(F("Lunar Cycle"), 36, 12);

  myGLCD.setColor(130, 130, 130);
  myGLCD.drawLine(40, 31, 239, 31); // under header
  myGLCD.drawLine(0, 307, 104, 307); // left footer
  myGLCD.drawLine(136, 307, 239, 307); // right footer

  myFiles.load(107, 294, 26, 26, "footextr.raw"); // footer button
  
  myGLCD.setFont(arial_bold);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print(F("Enabled"), 39, 62);
  
  // Enable/disable buttons
  byte lunarEnabled = EEPROM.read(70);
  myFiles.load(190, 55, 30, 30, schedActiveB[lunarEnabled]);
  
  myGLCD.print(F("Current cycle:"), 8, 100);
  byte moonType = 0; 
  // 0 = new moon, 1 = New Crescent, 2 = First Quarter, 3 = Waxing Gibbous, 4 = Full Moon, 
  // 5 = Waning Gibbous, 6 = Last Quarter, 7 = Old Crescent
  byte lunarCycleDay = getLunarCycleDay();
  if ((lunarCycleDay >= 0) && (lunarCycleDay <= 2))
  {
    moonType = 0;
    myGLCD.print(F("New moon"), 56, 200);
  }
  if ((lunarCycleDay >= 3) && (lunarCycleDay <= 5))
  {
    moonType = 1;
    myGLCD.print(F("New Crescent"), 24, 200);
  }
  if ((lunarCycleDay >= 6) && (lunarCycleDay <= 9))
  {
    moonType = 2;
    myGLCD.print(F("First Quarter"), 16, 200);
  }
  if ((lunarCycleDay >= 10) && (lunarCycleDay <= 12))
  {
    moonType = 3;
    myGLCD.print(F("Waxing Gibbous"), 8, 200);
  }
  if ((lunarCycleDay >= 13) && (lunarCycleDay <= 16))
  {
    moonType = 4;
    myGLCD.print(F("Full moon"), 48, 200);
  }
  if ((lunarCycleDay >= 17) && (lunarCycleDay <= 20)) 
  {
    moonType = 5;
    myGLCD.print(F("Waning Gibbous"), 8, 200);
  }
  if ((lunarCycleDay >= 21) && (lunarCycleDay <= 23))
  {
    moonType = 6;
    myGLCD.print(F("Last Quarter"), 24, 200);
  }
  if ((lunarCycleDay >= 24) && (lunarCycleDay <= 27))
  {
    moonType = 7;
    myGLCD.print(F("Old Crescent"), 24, 200);
  }
  if ((lunarCycleDay >= 28) && (lunarCycleDay <= 30))
  {
    moonType = 0;
    myGLCD.print(F("New moon"), 56, 200);
  }
  
  myFiles.load(87, 120, 65, 65, moonImages[moonType]);
  
  myGLCD.setColor(255, 255, 255);
  myGLCD.setFont(Sinclair_S);
  myGLCD.print(F("Adjusts Moonlight setting"), 20, 240);
  myGLCD.print(F("down to 10% based on"), 40, 250);
  myGLCD.print(F("lunar calender"), 64, 260);
}

void Opaq_iaqua::screenColor(byte selectedChan)
{
  dispScreen=22;

  if(selectedChan == 0) //re-draw entire screen
  {
    myGLCD.clrScr();
    updateTimeDate(true);
  
    // draw header
    myFiles.load(2, 2, 30, 30, "22color.raw");
    myGLCD.setFont(arial_bold);
    myGLCD.setColor(255, 77, 0);
    myGLCD.print(F("LED colors"), 36, 12);

    myGLCD.setColor(130, 130, 130);
    myGLCD.drawLine(40, 31, 239, 31); // under header
    myGLCD.drawLine(0, 307, 104, 307); // left footer
    myGLCD.drawLine(136, 307, 239, 307); // right footer

    myFiles.load(107, 294, 26, 26, "footextr.raw");  // footer settings button
    
    myGLCD.setFont(arial_bold);
    myGLCD.setColor(255, 255, 255);
    myGLCD.print(F("Current colors"), 8, 49);
    myGLCD.print(F("Available"), 48, 200);
    myGLCD.print(F("Colors"), 72, 220);
    
    //draw labels above colors
    myGLCD.setFont(Sinclair_S);
    myGLCD.print(F("LED 1"), 28, 71);
    myGLCD.print(F("LED 2"), 99, 71);
    myGLCD.print(F("LED 3"), 170, 71);
    myGLCD.print(F("LED 4"), 28, 131);
    myGLCD.print(F("LED 5"), 99, 131);
    myGLCD.print(F("LED 6"), 170, 131);
    
    //Draw available colors
    myGLCD.setColor(VGAColor[0]);
    myGLCD.fillRect(8, 243, 33, 268);
    myGLCD.setColor(VGAColor[1]);
    myGLCD.fillRect(41, 243, 66, 268);
    myGLCD.setColor(VGAColor[2]);
    myGLCD.fillRect(74, 243, 99, 268);
    myGLCD.setColor(VGAColor[3]);
    myGLCD.fillRect(107, 243, 132, 268);
    myGLCD.setColor(VGAColor[4]);
    myGLCD.fillRect(140, 243, 165, 268);
    myGLCD.setColor(VGAColor[5]);
    myGLCD.fillRect(173, 243, 198, 268);
    myGLCD.setColor(VGAColor[6]);
    myGLCD.fillRect(206, 243, 231, 268);
  }
    
  //Unselected outlines
  myGLCD.setColor(100, 100, 100);
  myGLCD.drawRect(26, 80, 71, 125); //bar 1
  myGLCD.drawRect(25, 79, 72, 126);
  myGLCD.drawRect(97, 80, 142, 125); //bar 2
  myGLCD.drawRect(96, 79, 143, 126);
  myGLCD.drawRect(168, 80, 213, 125); //bar 3
  myGLCD.drawRect(167, 79, 214, 126);
  myGLCD.drawRect(26, 140, 71, 185); //bar 4
  myGLCD.drawRect(25, 139, 72, 186);
  myGLCD.drawRect(97, 140, 142, 185); //bar 5
  myGLCD.drawRect(96, 139, 143, 186);
  myGLCD.drawRect(168, 140, 213, 185); //bar 6
  myGLCD.drawRect(167, 139, 214, 186);
  
  //fill in current colors
  myGLCD.setColor(VGAColor[barColors[0]]);
  myGLCD.fillRect(27, 81, 70, 124);
  myGLCD.setColor(VGAColor[barColors[1]]);
  myGLCD.fillRect(98, 81, 141, 124);
  myGLCD.setColor(VGAColor[barColors[2]]);
  myGLCD.fillRect(169, 81, 212, 124);
  myGLCD.setColor(VGAColor[barColors[3]]);
  myGLCD.fillRect(27, 141, 70, 184);
  myGLCD.setColor(VGAColor[barColors[4]]);
  myGLCD.fillRect(98, 141, 141, 184);
  myGLCD.setColor(VGAColor[barColors[5]]);
  myGLCD.fillRect(169, 141, 212, 184);
  
  // selected outlines (2 pixels thick)
  myGLCD.setColor(VGA_WHITE);
  if(selectedChan == 1){myGLCD.drawRect(25, 79, 72, 126);myGLCD.drawRect(26, 80, 71, 125);}
  if(selectedChan == 2){myGLCD.drawRect(96, 79, 143, 126);myGLCD.drawRect(97, 80, 142, 125);}
  if(selectedChan == 3){myGLCD.drawRect(167, 79, 214, 126);myGLCD.drawRect(168, 80, 213, 125);}
  if(selectedChan == 4){myGLCD.drawRect(25, 139, 72, 186);myGLCD.drawRect(26, 140, 71, 185);}
  if(selectedChan == 5){myGLCD.drawRect(96, 139, 143, 186);myGLCD.drawRect(97, 140, 142, 185);}
  if(selectedChan == 6){myGLCD.drawRect(167, 139, 214, 186);myGLCD.drawRect(168, 140, 213, 185);}
}

void Opaq_iaqua::screenGraphLEDs()
{
  dispScreen = 23;
  myGLCD.clrScr();
  // draw header
  myFiles.load(2, 2, 30, 30, "23graph.raw");
  myGLCD.setFont(arial_bold);
  myGLCD.setColor(255, 77, 0);
  myGLCD.print(F("LED graph"), 36, 12);
  updateTimeDate(true);

  myGLCD.setColor(130, 130, 130);
  myGLCD.drawLine(40, 31, 239, 31); // under header
  myGLCD.drawLine(0, 307, 104, 307); // left footer
  myGLCD.drawLine(136, 307, 239, 307); // right footer
  myFiles.load(107, 294, 26, 26, "footextr.raw");  // footer settings button
  
  //Draw graph
  byte xS = 65;   // x coordinate of 0,0 position of graph
  byte yS = 50;   // y coordinate of 0,0 position of graph
  byte dx = 240;  // number of pixels on x-axis
  byte dy = 144;  // number of pixels on y-axis
  char char6[6];
  
  // draw graph outline
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRect(xS, yS, xS+dy, yS+dx);
  
  /* graph outline for portrait
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRect(xS, yS, xS+dx, yS+dy);
  */
  
  // draw axis labels
  myGLCD.setFont(arial_bold);
  myGLCD.print(F("% Output"),xS+8,yS-17);
  myGLCD.setFont(Sinclair_S);
  myGLCD.print(F("12:00AM"),xS-57,yS-2);
  myGLCD.print(F("6:00AM"),xS-49,yS-4+(dx/4));
  myGLCD.print(F("12:00PM"),xS-57,yS-4+(dx/2));
  myGLCD.print(F("6:00PM"),xS-49,yS-4+((dx*3)/4));
  myGLCD.print(F("12:00AM"),xS-57,yS-4+dx);
  
  // draw graph grid
  myGLCD.setColor(100, 100, 100);
  for( int a = 1 ; a < 24 ; a++) // one line per hour
  {
    myGLCD.drawLine(xS+1,yS+(a*(dx/24)),xS+dy-1,yS+(a*(dx/24)));
  }
  for( int a = 1 ; a < 4 ; a++) // one line at 25/50/75% PWM
  {
    myGLCD.drawLine(xS+a*(dy/4),yS+1,xS+a*(dy/4),yS+dx-1);
  }
  
  /* grids for portrait
  myGLCD.setColor(100, 100, 100);
  for( int a = 1 ; a < 24 ; a++) // one line per hour
  {
    myGLCD.drawLine(xS+(a*(dx/24)),yS+1,xS+(a*(dx/24)),yS+dy-1);
  }
  for( int a = 1 ; a < 4 ; a++) // one line at 25/50/75% PWM
  {
    myGLCD.drawLine(xS+1,yS+a*(dy/4),xS+dx-1,yS+a*(dy/4));
  }
  */
  
  //===========  Graphing code starts here  =========================

  //draw channel 1 line here
  graphChannel(lightLowSun.chan1,lightMidSun.chan1,lightHighSun.chan1,lightMoon.chan1,1,xS,yS,dx,dy);
  //draw channel 2 line here
  graphChannel(lightLowSun.chan2,lightMidSun.chan2,lightHighSun.chan2,lightMoon.chan2,2,xS,yS,dx,dy);
  //draw channel 3 line here
  graphChannel(lightLowSun.chan3,lightMidSun.chan3,lightHighSun.chan3,lightMoon.chan3,3,xS,yS,dx,dy);
  //draw channel 4 line here
  graphChannel(lightLowSun.chan4,lightMidSun.chan4,lightHighSun.chan4,lightMoon.chan4,4,xS,yS,dx,dy);
  if(lightCSP == false)
  {
    //draw channel 5 line here
    graphChannel(lightLowSun.chan5,lightMidSun.chan5,lightHighSun.chan5,lightMoon.chan5,5,xS,yS,dx,dy);
    //draw channel 6 line here
    graphChannel(lightLowSun.chan6,lightMidSun.chan6,lightHighSun.chan6,lightMoon.chan6,6,xS,yS,dx,dy);
  }
  // refresh outline to cover any lines that print on it
  myGLCD.setColor(VGA_WHITE);
  myGLCD.drawRect(xS, yS, xS+dy, yS+dx);
}

void Opaq_iaqua::screenLightsIR() // draw the screen for configuring the lights
{ 
  dispScreen=24;
  //touchWaitTime = LONG_WAIT;
  
  char char3[3]; // used for converting numbers to char
  char char3t[3];

  myGLCD.clrScr();
  updateTimeDate(true);

  // draw header
  myFiles.load(2, 2, 30, 30, "5lights.raw",2);
  myGLCD.setFont(arial_bold);
  myGLCD.setColor(185, 55, 255);
  myGLCD.print(F("LIGHT MODES"), 36, 12);

  myGLCD.setColor(130, 130, 130);
  myGLCD.drawLine(40, 31, 239, 31); // under header
  myGLCD.drawLine(0, 307, 104, 307); // left footer
  myGLCD.drawLine(136, 307, 239, 307); // right footer

  myGLCD.drawLine(0, 94, 239, 94); // vertical center line
  myGLCD.drawLine(0, 168, 239, 168); // vertical center line

  myFiles.load(107, 294, 26, 26, "footextr.raw",2); // footer button

  // draw buttons based on current mode (either selected or not selected)
  if (currentLightMode==0)// high sun
  {
    Serial.print(F("Light mode = 0\n"));
    myFiles.load(10, 39, 48, 48, lightModeS[0],2);
    myFiles.load(67, 39, 48, 48, lightMode[1],2);
    myFiles.load(124, 39, 48, 48, lightMode[2],2);
    myFiles.load(181, 39, 48, 48, lightMode[3],2);
    myFiles.load(10, 101, 48, 63, lightEdit[1],2);
    myFiles.load(181, 101, 48, 63, lightResync[1],2);    

    // get RGBW for high sun
    currentColor.chan1=EEPROM.read(400);
    currentColor.chan2=EEPROM.read(401);
    currentColor.chan3=EEPROM.read(402);
    currentColor.chan4=EEPROM.read(403);
    currentColor.chan5=EEPROM.read(404);
    currentColor.chan6=EEPROM.read(405);
  }  
  else if (currentLightMode==1) // mid sun
  {
    Serial.print(F("Light mode = 1\n"));
    myFiles.load(10, 39, 48, 48, lightMode[0],2);
    myFiles.load(67, 39, 48, 48, lightModeS[1],2);
    myFiles.load(124, 39, 48, 48, lightMode[2],2);
    myFiles.load(181, 39, 48, 48, lightMode[3],2);
    myFiles.load(10, 101, 48, 63, lightEdit[1],2);
    myFiles.load(181, 101, 48, 63, lightResync[1],2);    

    // get RGBW for mid sun
    currentColor.chan1=EEPROM.read(410);
    currentColor.chan2=EEPROM.read(411);
    currentColor.chan3=EEPROM.read(412);
    currentColor.chan4=EEPROM.read(413);
    currentColor.chan5=EEPROM.read(414);
    currentColor.chan6=EEPROM.read(415);
  }
  else if (currentLightMode==2) // low sun
  {
    Serial.print(F("Light mode = 2\n"));
    myFiles.load(10, 39, 48, 48, lightMode[0],2);
    myFiles.load(67, 39, 48, 48, lightMode[1],2);
    myFiles.load(124, 39, 48, 48, lightModeS[2],2);
    myFiles.load(181, 39, 48, 48, lightMode[3],2);
    myFiles.load(10, 101, 48, 63, lightEdit[1],2);
    myFiles.load(181, 101, 48, 63, lightResync[1],2);    

    // get RGBW for low sun
    currentColor.chan1=EEPROM.read(420);
    currentColor.chan2=EEPROM.read(421);
    currentColor.chan3=EEPROM.read(422);
    currentColor.chan4=EEPROM.read(423);
    currentColor.chan5=EEPROM.read(424);
    currentColor.chan6=EEPROM.read(425);
  }
  else if (currentLightMode==3) // moon
  {
    Serial.print(F("Light mode = 3\n"));
    myFiles.load(10, 39, 48, 48, lightMode[0],2);
    myFiles.load(67, 39, 48, 48, lightMode[1],2);
    myFiles.load(124, 39, 48, 48, lightMode[2],2);
    myFiles.load(181, 39, 48, 48, lightModeS[3],2);
    myFiles.load(10, 101, 48, 63, lightEdit[1],2);
    myFiles.load(181, 101, 48, 63, lightResync[1],2);

    // get RGBW for moon
    currentColor.chan1=EEPROM.read(430);
    currentColor.chan2=EEPROM.read(431);
    currentColor.chan3=EEPROM.read(432);
    currentColor.chan4=EEPROM.read(433);
    currentColor.chan5=EEPROM.read(434);
    currentColor.chan6=EEPROM.read(435);
  }
  else if ((currentLightMode==4)||(currentLightMode==5)) // lights in transition or unknown
  {
    myFiles.load(10, 39, 48, 48, lightMode[0],2);
    myFiles.load(67, 39, 48, 48, lightMode[1],2);
    myFiles.load(124, 39, 48, 48, lightMode[2],2);
    myFiles.load(181, 39, 48, 48, lightMode[3],2);
    myFiles.load(10, 101, 48, 63, lightEdit[0],2);
    myFiles.load(181, 101, 48, 63, lightResync[1],2);
  }
  Serial.print(F("About to draw up/down arrows\n"));

  //Draw greyed out up/down arrows
  myFiles.load(10, 175, 48, 48, lightGray[0]);
  myFiles.load(67, 175, 48, 48, lightGray[0]);
  myFiles.load(124, 175, 48, 48, lightGray[0]);
  myFiles.load(181, 175, 48, 48, lightGray[0]);

  myFiles.load(10, 241, 48, 48, lightGray[1]);
  myFiles.load(67, 241, 48, 48, lightGray[1]);
  myFiles.load(124, 241, 48, 48, lightGray[1]);
  myFiles.load(181, 241, 48, 48, lightGray[1]);
  
  // draw the RGBW values to the screen
  myGLCD.setFont(Sinclair_S);
  myGLCD.setColor(255, 255, 255);
  
  byte dx = 25;
  if(currentColor.chan1 < 9)  dx += 4;
  if(currentColor.chan1 < 99) dx += 4;
  itoa(currentColor.chan1, char3, 10);
  myGLCD.print(char3, dx, 228);

  dx = 80;
  if(currentColor.chan2 < 9)  dx += 4;
  if(currentColor.chan2 < 99) dx += 4;
  itoa(currentColor.chan2, char3, 10);
  myGLCD.print(char3, dx, 228);

  dx = 138;
  if(currentColor.chan3 < 9)  dx += 4;
  if(currentColor.chan3 < 99) dx += 4;
  itoa(currentColor.chan3, char3, 10);
  myGLCD.print(char3, dx, 228);

  dx = 195;
  if(currentColor.chan4 < 9)  dx += 4;
  if(currentColor.chan4 < 99) dx += 4;
  itoa(currentColor.chan4, char3, 10);
  myGLCD.print(char3, dx, 228);


  // draw the rest of the buttons disabled until the edit button is pressed
  myFiles.load(67, 101, 48, 63, lightSave[0],2);
  myFiles.load(124, 101, 48, 63, lightCancel[0],2);
  
  myGLCD.setFont(Sinclair_S);
  myGLCD.setColor(255, 255, 255);
  
  Serial.print(F("End of screenLightsIR()\n"));
}



// returns day in lunar cycle 0-29
byte Opaq_iaqua::getLunarCycleDay()
{
  tmElements_t fixedDate = {0,35,20,0,7,1,0};
  long lp = 2551443;
  time_t newMoonCycle = makeTime(fixedDate);
  long phase = (now() - newMoonCycle) % lp;
  long returnValue = ((phase / 86400) + 1);
  return returnValue;
}

void Opaq_iaqua::graphChannel(int lowSunVal,int midSunVal,int highSunVal,int moonVal,byte channel,int x,int y,int dx,int dy)
{
  unsigned int divisor = 1440 / dx; // Minutes in a day divided by number of x-axis pixels
  //14 x,y points on graph, 12 points at ramp start/ends and 1 each for start/end of graph
  unsigned int xPos[14]; 
  unsigned int yPos[14];
  //if ramp starts before midnight but ends after this variable is used to calculate the cross-over value
  long transitionValue; 
  long rampTime;
  
  //calculate start/end time of each ramp
  unsigned long startRamp1 = (ramp1.onHour*60) + ramp1.onMinute;
  unsigned long endRamp1 = startRamp1 + ((ramp1.offHour*60) + ramp1.offMinute);
  unsigned long startRamp2 = (ramp2.onHour*60) + ramp2.onMinute;
  unsigned long endRamp2 = startRamp2 + ((ramp2.offHour*60) + ramp2.offMinute);
  unsigned long startRamp3 = (ramp3.onHour*60) + ramp3.onMinute;
  unsigned long endRamp3 = startRamp3 + ((ramp3.offHour*60) + ramp3.offMinute);
  unsigned long startRamp4 = (ramp4.onHour*60) + ramp4.onMinute;
  unsigned long endRamp4 = startRamp4 + ((ramp4.offHour*60) + ramp4.offMinute);
  unsigned long startRamp5 = (ramp5.onHour*60) + ramp5.onMinute;
  unsigned long endRamp5 = startRamp5 + ((ramp5.offHour*60) + ramp5.offMinute);
  unsigned long startRamp6 = (ramp6.onHour*60) + ramp6.onMinute;
  unsigned long endRamp6 = startRamp6 + ((ramp6.offHour*60) + ramp6.offMinute);
  
  // divide by divisor to bring scale to 0-dx
  startRamp1 /= divisor;
  endRamp1 /= divisor;
  startRamp2 /= divisor;
  endRamp2 /= divisor;
  startRamp3 /= divisor;
  endRamp3 /= divisor;
  startRamp4 /= divisor;
  endRamp4 /= divisor;
  startRamp5 /= divisor;
  endRamp5 /= divisor;
  startRamp6 /= divisor;
  endRamp6 /= divisor;
  
  //roll-over any end points after midnight
  if(endRamp1 > dx) endRamp1 -= dx;
  if(endRamp2 > dx) endRamp2 -= dx;
  if(endRamp3 > dx) endRamp3 -= dx;
  if(endRamp4 > dx) endRamp4 -= dx;
  if(endRamp5 > dx) endRamp5 -= dx;
  if(endRamp6 > dx) endRamp6 -= dx;
  
  //y-axis points (% of PWM output)
  moonVal = map(moonVal,0,100,1,dy-1); // results in value of 0-dy excluding graph outline
  lowSunVal = map(lowSunVal,0,100,1,dy-1); 
  midSunVal = map(midSunVal,0,100,1,dy-1);
  highSunVal = map(highSunVal,0,100,1,dy-1);
  
  if(startRamp1 < endRamp6) // case 1: ramp 1 starts after midnight
  {
    xPos[0] = 0; //start at 0:00
    yPos[0] = moonVal;
    xPos[1] = startRamp1;
    yPos[1] = moonVal;
    xPos[2] = endRamp1;
    yPos[2] = lowSunVal;
    xPos[3] = startRamp2;
    yPos[3] = lowSunVal;
    xPos[4] = endRamp2;
    yPos[4] = midSunVal;
    xPos[5] = startRamp3;
    yPos[5] = midSunVal;
    xPos[6] = endRamp3;
    yPos[6] = highSunVal;
    xPos[7] = startRamp4;
    yPos[7] = highSunVal;
    xPos[8] = endRamp4;
    yPos[8] = midSunVal;
    xPos[9] = startRamp5;
    yPos[9] = midSunVal;
    xPos[10] = endRamp5;
    yPos[10] = lowSunVal;
    xPos[11] = startRamp6;
    yPos[11] = lowSunVal;
    xPos[12] = endRamp6;
    yPos[12] = moonVal;
    xPos[13] = dx-1; //end at 24:00
    yPos[13] = moonVal;
  }else if(endRamp6 < startRamp6) //case 2: ramp 6 starts before but ends after midnight
  {
    //calculate value at midnight:
    rampTime = ((ramp1.offHour*60) + ramp1.offMinute) / divisor;
    transitionValue = (endRamp6 * (lowSunVal - moonVal));
    transitionValue = transitionValue / rampTime;
    transitionValue += moonVal;
    xPos[0] = 0; //start at 0:00
    yPos[0] = transitionValue;
    xPos[1] = endRamp6;
    yPos[1] = moonVal;
    xPos[2] = startRamp1;
    yPos[2] = moonVal;
    xPos[3] = endRamp1;
    yPos[3] = lowSunVal;
    xPos[4] = startRamp2;
    yPos[4] = lowSunVal;
    xPos[5] = endRamp2;
    yPos[5] = midSunVal;
    xPos[6] = startRamp3;
    yPos[6] = midSunVal;
    xPos[7] = endRamp3;
    yPos[7] = highSunVal;
    xPos[8] = startRamp4;
    yPos[8] = highSunVal;
    xPos[9] = endRamp4;
    yPos[9] = midSunVal;
    xPos[10] = startRamp5;
    yPos[10] = midSunVal;
    xPos[11] = endRamp5;
    yPos[11] = lowSunVal;
    xPos[12] = startRamp6;
    yPos[12] = lowSunVal;
    xPos[13] = dx-1; //end at 24:00
    yPos[13] = transitionValue;
  }else if(startRamp6 < endRamp5) //case 3: ramp 6 starts after midnight
  {
    xPos[0] = 0; //start at 0:00
    yPos[0] = lowSunVal;
    xPos[1] = startRamp6;
    yPos[1] = lowSunVal;
    xPos[2] = endRamp6;
    yPos[2] = moonVal;
    xPos[3] = startRamp1;
    yPos[3] = moonVal;
    xPos[4] = endRamp1;
    yPos[4] = lowSunVal;
    xPos[5] = startRamp2;
    yPos[5] = lowSunVal;
    xPos[6] = endRamp2;
    yPos[6] = midSunVal;
    xPos[7] = startRamp3;
    yPos[7] = midSunVal;
    xPos[8] = endRamp3;
    yPos[8] = highSunVal;
    xPos[9] = startRamp4;
    yPos[9] = highSunVal;
    xPos[10] = endRamp4;
    yPos[10] = midSunVal;
    xPos[11] = startRamp5;
    yPos[11] = midSunVal;
    xPos[12] = endRamp5;
    yPos[12] = lowSunVal;
    xPos[13] = dx-1; //end at 24:00
    yPos[13] = lowSunVal;
  }else if(endRamp5 < startRamp5) //case 4: ramp 5 starts before but ends after midnight
  {
    //calculate value at midnight:
    rampTime = ((ramp5.offHour*60) + ramp5.offMinute) / divisor;
    transitionValue = (endRamp5 * (midSunVal - lowSunVal));
    transitionValue = transitionValue / rampTime;
    transitionValue += lowSunVal;
    xPos[0] = 0; //start at 0:00
    yPos[0] = transitionValue;
    xPos[1] = endRamp5;
    yPos[1] = lowSunVal;
    xPos[2] = startRamp6;
    yPos[2] = lowSunVal;
    xPos[3] = endRamp6;
    yPos[3] = moonVal;
    xPos[4] = startRamp1;
    yPos[4] = moonVal;
    xPos[5] = endRamp1;
    yPos[5] = lowSunVal;
    xPos[6] = startRamp2;
    yPos[6] = lowSunVal;
    xPos[7] = endRamp2;
    yPos[7] = midSunVal;
    xPos[8] = startRamp3;
    yPos[8] = midSunVal;
    xPos[9] = endRamp3;
    yPos[9] = highSunVal;
    xPos[10] = startRamp4;
    yPos[10] = highSunVal;
    xPos[11] = endRamp4;
    yPos[11] = midSunVal;
    xPos[12] = startRamp5;
    yPos[12] = midSunVal;
    xPos[13] = dx-1; //end at 24:00
    yPos[13] = transitionValue;
  }else if(startRamp5 < endRamp4) //case 5: ramp 5 starts after midnight
  {
    xPos[0] = 0; //start at 0:00
    yPos[0] = midSunVal;
    xPos[1] = startRamp5;
    yPos[1] = midSunVal;
    xPos[2] = endRamp5;
    yPos[2] = lowSunVal;
    xPos[3] = startRamp6;
    yPos[3] = lowSunVal;
    xPos[4] = endRamp6;
    yPos[4] = moonVal;
    xPos[5] = startRamp1;
    yPos[5] = moonVal;
    xPos[6] = endRamp1;
    yPos[6] = lowSunVal;
    xPos[7] = startRamp2;
    yPos[7] = lowSunVal;
    xPos[8] = endRamp2;
    yPos[8] = midSunVal;
    xPos[9] = startRamp3;
    yPos[9] = midSunVal;
    xPos[10] = endRamp3;
    yPos[10] = highSunVal;
    xPos[11] = startRamp4;
    yPos[11] = highSunVal;
    xPos[12] = endRamp4;
    yPos[12] = midSunVal;
    xPos[13] = dx-1; //end at 24:00
    yPos[13] = midSunVal;
  }else if(endRamp4 < startRamp4) //case  6: ramp 4 starts before but ends after midnight
  {
    //calculate value at midnight:
    rampTime = ((ramp4.offHour*60) + ramp4.offMinute) / divisor;
    transitionValue = (endRamp4 * (highSunVal - midSunVal));
    transitionValue = transitionValue / rampTime;
    transitionValue += midSunVal;
    xPos[0] = 0; //start at 0:00
    yPos[0] = transitionValue;
    xPos[1] = endRamp4;
    yPos[1] = midSunVal;
    xPos[2] = startRamp5;
    yPos[2] = midSunVal;
    xPos[3] = endRamp5;
    yPos[3] = lowSunVal;
    xPos[4] = startRamp6;
    yPos[4] = lowSunVal;
    xPos[5] = endRamp6;
    yPos[5] = moonVal;
    xPos[6] = startRamp1;
    yPos[6] = moonVal;
    xPos[7] = endRamp1;
    yPos[7] = lowSunVal;
    xPos[8] = startRamp2;
    yPos[8] = lowSunVal;
    xPos[9] = endRamp2;
    yPos[9] = midSunVal;
    xPos[10] = startRamp3;
    yPos[10] = midSunVal;
    xPos[11] = endRamp3;
    yPos[11] = highSunVal;
    xPos[12] = startRamp4;
    yPos[12] = highSunVal;
    xPos[13] = dx-1; //end at 24:00
    yPos[13] = transitionValue;
  }else if(startRamp4 < endRamp3) //case  7: ramp 4 starts after midnight
  {
    xPos[0] = 0; //start at 0:00
    yPos[0] = highSunVal;
    xPos[1] = startRamp4;
    yPos[1] = highSunVal;
    xPos[2] = endRamp4;
    yPos[2] = midSunVal;
    xPos[3] = startRamp5;
    yPos[3] = midSunVal;
    xPos[4] = endRamp5;
    yPos[4] = lowSunVal;
    xPos[5] = startRamp6;
    yPos[5] = lowSunVal;
    xPos[6] = endRamp6;
    yPos[6] = moonVal;
    xPos[7] = startRamp1;
    yPos[7] = moonVal;
    xPos[8] = endRamp1;
    yPos[8] = lowSunVal;
    xPos[9] = startRamp2;
    yPos[9] = lowSunVal;
    xPos[10] = endRamp2;
    yPos[10] = midSunVal;
    xPos[11] = startRamp3;
    yPos[11] = midSunVal;
    xPos[12] = endRamp3;
    yPos[12] = highSunVal;
    xPos[13] = dx-1; //end at 24:00
    yPos[13] = highSunVal;
  }else if(endRamp3 < startRamp3) //case  8: ramp 3 starts before but ends after midnight
  {
    //calculate value at midnight:
    rampTime = ((ramp3.offHour*60) + ramp3.offMinute) / divisor;
    transitionValue = (endRamp3 * (midSunVal - highSunVal));
    transitionValue = transitionValue / rampTime;
    transitionValue += highSunVal;
    xPos[0] = 0; //start at 0:00
    yPos[0] = transitionValue;
    xPos[1] = endRamp3;
    yPos[1] = highSunVal;
    xPos[2] = startRamp4;
    yPos[2] = highSunVal;
    xPos[3] = endRamp4;
    yPos[3] = midSunVal;
    xPos[4] = startRamp5;
    yPos[4] = midSunVal;
    xPos[5] = endRamp5;
    yPos[5] = lowSunVal;
    xPos[6] = startRamp6;
    yPos[6] = lowSunVal;
    xPos[7] = endRamp6;
    yPos[7] = moonVal;
    xPos[8] = startRamp1;
    yPos[8] = moonVal;
    xPos[9] = endRamp1;
    yPos[9] = lowSunVal;
    xPos[10] = startRamp2;
    yPos[10] = lowSunVal;
    xPos[11] = endRamp2;
    yPos[11] = midSunVal;
    xPos[12] = startRamp3;
    yPos[12] = midSunVal;
    xPos[13] = dx-1; //end at 24:00
    yPos[13] = transitionValue;
  }else if(startRamp3 < endRamp2) //case  9: ramp 3 starts after midnight
  {
    xPos[0] = 0; //start at 0:00
    yPos[0] = midSunVal;
    xPos[1] = startRamp3;
    yPos[1] = midSunVal;
    xPos[2] = endRamp3;
    yPos[2] = highSunVal;
    xPos[3] = startRamp4;
    yPos[3] = highSunVal;
    xPos[4] = endRamp4;
    yPos[4] = midSunVal;
    xPos[5] = startRamp5;
    yPos[5] = midSunVal;
    xPos[6] = endRamp5;
    yPos[6] = lowSunVal;
    xPos[7] = startRamp6;
    yPos[7] = lowSunVal;
    xPos[8] = endRamp6;
    yPos[8] = moonVal;
    xPos[9] = startRamp1;
    yPos[9] = moonVal;
    xPos[10] = endRamp1;
    yPos[10] = lowSunVal;
    xPos[11] = startRamp2;
    yPos[11] = lowSunVal;
    xPos[12] = endRamp2;
    yPos[12] = midSunVal;
    xPos[13] = dx-1; //end at 24:00
    yPos[13] = midSunVal;
  }else if(endRamp2 < startRamp2) //case 10: ramp 2 starts before but ends after midnight
  {
    //calculate value at midnight:
    rampTime = ((ramp2.offHour*60) + ramp2.offMinute) / divisor;
    transitionValue = (endRamp2 * (lowSunVal - midSunVal));
    transitionValue = transitionValue / rampTime;
    transitionValue += midSunVal;
    xPos[0] = 0; //start at 0:00
    yPos[0] = transitionValue;
    xPos[1] = endRamp2;
    yPos[1] = midSunVal;
    xPos[2] = startRamp3;
    yPos[2] = midSunVal;
    xPos[3] = endRamp3;
    yPos[3] = highSunVal;
    xPos[4] = startRamp4;
    yPos[4] = highSunVal;
    xPos[5] = endRamp4;
    yPos[5] = midSunVal;
    xPos[6] = startRamp5;
    yPos[6] = midSunVal;
    xPos[7] = endRamp5;
    yPos[7] = lowSunVal;
    xPos[8] = startRamp6;
    yPos[8] = lowSunVal;
    xPos[9] = endRamp6;
    yPos[9] = moonVal;
    xPos[10] = startRamp1;
    yPos[10] = moonVal;
    xPos[11] = endRamp1;
    yPos[11] = lowSunVal;
    xPos[12] = startRamp2;
    yPos[12] = lowSunVal;
    xPos[13] = dx-1; //end at 24:00
    yPos[13] = transitionValue;
  }else if(startRamp2 < endRamp1) //case 11: ramp 2 starts after midnight
  {
    xPos[0] = 0; //start at 0:00
    yPos[0] = lowSunVal;
    xPos[1] = startRamp2;
    yPos[1] = lowSunVal;
    xPos[2] = endRamp2;
    yPos[2] = midSunVal;
    xPos[3] = startRamp3;
    yPos[3] = midSunVal;
    xPos[4] = endRamp3;
    yPos[4] = highSunVal;
    xPos[5] = startRamp4;
    yPos[5] = highSunVal;
    xPos[6] = endRamp4;
    yPos[6] = midSunVal;
    xPos[7] = startRamp5;
    yPos[7] = midSunVal;
    xPos[8] = endRamp5;
    yPos[8] = lowSunVal;
    xPos[9] = startRamp6;
    yPos[9] = lowSunVal;
    xPos[10] = endRamp6;
    yPos[10] = moonVal;
    xPos[11] = startRamp1;
    yPos[11] = moonVal;
    xPos[12] = endRamp1;
    yPos[12] = lowSunVal;
    xPos[13] = dx-1; //end at 24:00
    yPos[13] = lowSunVal;
  }else if(endRamp1 < startRamp1) //case 12: ramp 1 starts before but ends after midnight
  {
    //calculate value at midnight:
    rampTime = ((ramp1.offHour*60) + ramp1.offMinute) / divisor;
    transitionValue = (endRamp1 * (moonVal - lowSunVal));
    transitionValue = transitionValue / rampTime;
    transitionValue += lowSunVal;
    xPos[0] = 0; //start at 0:00
    yPos[0] = transitionValue;
    xPos[1] = endRamp1;
    yPos[1] = lowSunVal;
    xPos[2] = startRamp2;
    yPos[2] = lowSunVal;
    xPos[3] = endRamp2;
    yPos[3] = midSunVal;
    xPos[4] = startRamp3;
    yPos[4] = midSunVal;
    xPos[5] = endRamp3;
    yPos[5] = highSunVal;
    xPos[6] = startRamp4;
    yPos[6] = highSunVal;
    xPos[7] = endRamp4;
    yPos[7] = midSunVal;
    xPos[8] = startRamp5;
    yPos[8] = midSunVal;
    xPos[9] = endRamp5;
    yPos[9] = lowSunVal;
    xPos[10] = startRamp6;
    yPos[10] = lowSunVal;
    xPos[11] = endRamp6;
    yPos[11] = moonVal;
    xPos[12] = startRamp1;
    yPos[12] = moonVal;
    xPos[13] = dx-1; //end at 24:00
    yPos[13] = transitionValue;
  }
  
  //draw channel line here (portrait display)
  myGLCD.setColor(VGAColor[barColors[channel-1]]); //set to proper color for channel
  for(int i = 0; i < 13 ; i++)
  {
    myGLCD.drawLine(x+yPos[i],y+xPos[i],x+yPos[i+1],y+xPos[i+1]);
  }
  
  //draw channel line here (landscape display)
  /*myGLCD.setColor(VGAColor[color]); 
  for(int i = 0; i < 13 ; i++)
  {
    myGLCD.drawLine(x+xPos[i]  ,y+yPos[i]  ,x+xPos[i+1]  ,y+yPos[i+1]);
  }*/
}

void Opaq_iaqua::drawATO()
{ 
  //draw water levels
  if(ATOEnabled)
  {
    myGLCD.setColor(255, 255, 255);
    myGLCD.setFont(arial_bold);
    if(ATOAlarm)
    {
      myFiles.load(150, 52, 60, 55, "1warn.raw",2);
      myGLCD.print(F("Alarm"), 136, 36);
    }else if(ATOPumpState)
    {
      myFiles.load(150, 52, 55, 55, "1atoon.raw",2);
      myGLCD.print(F("Running"), 125, 36);
    }else
    {
      myGLCD.setColor(255, 255, 255);
      myGLCD.setFont(arial_bold);
      myGLCD.print("T", 122, 36); //print letters individually due to space constraints
      myGLCD.print("A", 135, 36);
      myGLCD.print("N", 148, 36);
      myGLCD.print("K", 161, 36);
      myGLCD.print("ATO", 186, 36);
      myFiles.load(126, 54, 50, 50, WaterIcon[WaterLevel],4);
      myFiles.load(184, 54, 50, 50, WaterIcon[ReservoirLevel],4);
    }
  }
  else
  {
    myFiles.load(122, 45, 115, 45, "logo.raw",2); //display a iAqua logo if ATO is disabled
  }
}

int Opaq_iaqua::calcFeeding()
{
  // need to retrieve and calculate last feeding time
  time_t timeSinceLastFeed = now() - lastFeedingTime;
  int elapsedHr = timeSinceLastFeed/60/60;
  return elapsedHr;
}

void Opaq_iaqua::drawFeeding()
{
  myGLCD.setFont(arial_bold);
  myGLCD.setColor(255, 255, 255);

  char char4[4]; // used to convert int to char
  int fx; // x value used to center the hours/mins

  // if feeding isn't active, display the elapsed hours since last feeding
  if (feedingActive==false)
  {
    int feedHours=calcFeeding(); // calculate how many hours since we've fed the fish

    // print the feeding hours to screen
    if(feedHours < 100)
    {
      itoa(feedHours, char4, 10);
      if (feedHours<10) fx=20; // shift x position to center
      else if (feedHours>9) fx=12; // shift x position to center
      myGLCD.print(char4, fx, 130);
      myGLCD.print("HR", 12, 146);
    }else
    {
      myGLCD.print(">99", 4, 130);
      myGLCD.print("HR", 12, 146);
    }
  }

  // if feeding is active, we display the amount of time left in the feeding cycle
  else if (feedingActive==true)
  {
    // calculate how many minutes are left from millis to display on the screen
    unsigned long feedingTotalSeconds = feedingMins * 60;
    int feedingMinsLeft=(feedingTotalSeconds-(now()-startFeedingTime))/60;
    feedingMinsLeft=feedingMinsLeft+1; // make it 1 minute more, then use < sign for more intuitive display

    // convert to chars to write to screen
    itoa(feedingMinsLeft, char4, 10);
    if (feedingMinsLeft<10) fx=28;
    else if (feedingMinsLeft>9) fx=20;

    if (dispScreen==1) // write to home screen
    {
      myGLCD.setColor(0, 0, 0);
      myGLCD.fillRect(0,130,55,145); // clear previous value
      myGLCD.setColor(0, 184, 19);
      myGLCD.print("<", (fx-16), 130);
      myGLCD.print(char4, fx, 130);
      myGLCD.print(F("MIN"), 4, 146);
    }
    else if (dispScreen==2)  // write to feeding screen
    {
      myGLCD.setColor(0, 0, 0);
      myGLCD.fillRect(40,80,87,96); // clear previous value
      myGLCD.setColor(255, 255, 255);
      myGLCD.print("<", 24, 80);
      myGLCD.print(char4, 40, 80);
      myGLCD.print(F("MINUTES"), 88, 80);
    }
  }
}


void Opaq_iaqua::printValueUpdate()
{
  time_t printTime = now();
  Serial.print(F("Updating time: "));
  if(hourFormat12(printTime) < 10)Serial.print(F("0"));
  Serial.print(hourFormat12(printTime));
  Serial.print(F(":"));
  if(minute(printTime) < 10)Serial.print(F("0"));
  Serial.print(minute(printTime));
  Serial.print(F(":"));
  if(second(printTime) < 10)Serial.print(F("0"));
  Serial.print(second(printTime));
  if(fadeInProgress==true)
  {
    Serial.print(F("  Current Colors: "));
    Serial.print(currentColor.chan1);Serial.print(F(","));Serial.print(currentColor.chan2);Serial.print(F(","));
    Serial.print(currentColor.chan3);Serial.print(F(","));Serial.print(currentColor.chan4);Serial.print(F(","));
    Serial.print(currentColor.chan5);Serial.print(F(","));Serial.print(currentColor.chan6);
    Serial.print(F("  Last Colors: "));
    Serial.print(lastColor.chan1);Serial.print(F(","));Serial.print(lastColor.chan2);Serial.print(F(","));
    Serial.print(lastColor.chan3);Serial.print(F(","));Serial.print(lastColor.chan4);Serial.print(F(","));
    Serial.print(lastColor.chan5);Serial.print(F(","));Serial.print(lastColor.chan6);
    Serial.print(F("  Target Colors: "));
    Serial.print(targetColor.chan1);Serial.print(F(","));Serial.print(targetColor.chan2);Serial.print(F(","));
    Serial.print(targetColor.chan3);Serial.print(F(","));Serial.print(targetColor.chan4);Serial.print(F(","));
    Serial.print(targetColor.chan5);Serial.print(F(","));Serial.print(targetColor.chan6);
  }
  Serial.print(F("\n"));
}

void Opaq_iaqua::printTime(int thour, int tminute, byte ampm, int posx, int posy)
{
  char tmpTime[8], charT[3];

  tmpTime[0] = '\0';

  if (thour>=0 && thour<=9) {          //add space
    strcat(tmpTime, " ");
    itoa(thour, charT, 10);
    strcat(tmpTime, charT);
  }
  else 
    itoa(thour, tmpTime, 10);

  strcat(tmpTime, ":");  

  if (tminute>=0 && tminute<=9) {         //add 0
    strcat(tmpTime, "0");
    itoa(tminute, charT, 10);
    strcat(tmpTime, charT);
  }
  else {
    itoa(tminute, charT, 10);
    strcat(tmpTime, charT);
  }
  if (ampm==0) strcat(tmpTime, "am");
  else strcat(tmpTime, "pm");

  myGLCD.print(tmpTime, posx, posy);           // Display time  
}

void Opaq_iaqua::printTime24Hr(int thour, int tminute, int posx, int posy)
{
  char tmpTime[8], charT[3];

  tmpTime[0] = '\0';

  if (thour>=0 && thour<=9) {       //add space
    strcat(tmpTime, " ");
    itoa(thour, charT, 10);
    strcat(tmpTime, charT);
  }
  else 
    itoa(thour, tmpTime, 10);

  strcat(tmpTime, ":");  

  if (tminute>=0 && tminute<=9) {    //add 0
    strcat(tmpTime, "0");
    itoa(tminute, charT, 10);
    strcat(tmpTime, charT);
  }
  else {
    itoa(tminute, charT, 10);
    strcat(tmpTime, charT);
  }

  myGLCD.print(tmpTime, posx, posy); // Display time  
}

void Opaq_iaqua::printDate(int x, int y) 
{
  char  chDate[25], tmpChar[5];

  strcat(chDate, "     ");
  chDate[0] = '\0';
  strcat(chDate, dayShortStr(weekday()));
  strcat(chDate, ", ");
  strcat(chDate, monthShortStr(month()));
  strcat(chDate, " ");
  itoa(day(), tmpChar, 10);
  strcat(chDate, tmpChar);
  // this line is for omitting year
  strcat(chDate, "  ");

  myGLCD.print(chDate, x, y);            //Display date 
}

void Opaq_iaqua::updateTimeDate(boolean updateTime)
{
  // draw date and time
  myGLCD.setColor(240, 240, 255);
  myGLCD.setFont(NULL);

  if ((hour()!=prevRTC.Hour) || (minute()!=prevRTC.Minute) || updateTime)
  {
    prevRTC.Hour = hour();
    prevRTC.Minute = minute();
    if(displayIn12Hr == true)
       printTime(hourFormat12() , minute(), isPM(), 180, 2);
    
    if(displayIn12Hr == false)
       printTime24Hr(hour(),minute(),180,2);
    
    if(updateTime == false)
    {
      printValueUpdate();
    }
  }

  if ((day()!=prevRTC.Day) || (month()!=prevRTC.Month) || updateTime)
  {
    prevRTC.Day = day();
    prevRTC.Month = month();
    printDate(40, 2);             
  }
}


void Opaq_iaqua::drawTemp()
{
  char tempstring[7];  // water temperature as a string
  sensorToDisplay++;
  if(sensorToDisplay > 3)sensorToDisplay = 1;
  
  if((sensorToDisplay==1)&&((displaySensor1==false)||(sensor1Enabled==false)))sensorToDisplay++;
  if((sensorToDisplay==2)&&((displaySensor2==false)||(sensor2Enabled==false)))sensorToDisplay++;
  if((sensorToDisplay==3)&&((displaySensor3==false)||(sensor3Enabled==false)))
  {
    if((displaySensor1==true)&&(sensor1Enabled==true))
    {
      sensorToDisplay = 1;
    }else if((displaySensor2==true)&&(sensor2Enabled==true))
    {
      sensorToDisplay = 2;
    }else
    {
      sensorToDisplay = 0; //could not find a sensor to display...
    }
  }
  
  if(sensorToDisplay==1)
  {
    myGLCD.setFont(arial_bold);
    
    if(heaterWarning)
    {
      myFiles.load(30, 35, 60, 51, "1thermR.raw",4);
    }else
    {
      myFiles.load(30, 35, 60, 51, "1therm.raw",4);
    }
    
    if(temperature > -25)
    {
      // set temp color based on alarms on home screen
      if (heaterWarningCleared==true)
      {
        myGLCD.setColor(VGA_WHITE);  // if no warning is active, or it has been acknowledged
        if(displayInC == true)myFiles.load(90, 94, 14, 12, "c.raw",4);
        if(displayInC == false)myFiles.load(90, 94, 14, 12, "f.raw",4);
      }
      else if (heaterWarningCleared==false)
      {
        myGLCD.setColor(222, 8, 51);  // if warning is active and hasn't been acknowledged
        if(displayInC == true)myFiles.load(90, 94, 14, 12, "c_R.raw",4);
        if(displayInC == false)myFiles.load(90, 94, 14, 12, "f_R.raw",4);
      }
      dtostrf(temperature, 4, 1, tempstring);  //convert to string
      myGLCD.print(tempstring, 20, 92);
    }else
    {
      myGLCD.setColor(VGA_WHITE);
      myGLCD.print(F("  N/C  "), 4, 92);
    }
  }
  else if(sensorToDisplay==2)
  {
    myGLCD.setFont(arial_bold);
    myGLCD.setColor(VGA_WHITE);
    myFiles.load(30, 35, 60, 51, "1temp2.raw",4);
    if(temperature > -25)
    {
      dtostrf(temperature2, 4, 1, tempstring);
      myGLCD.print(tempstring, 20, 92);
      if(displayInC == true)myFiles.load(90, 94, 14, 12, "c.raw",4);
      if(displayInC == false)myFiles.load(90, 94, 14, 12, "f.raw",4);
    }else
    {
      myGLCD.print(F("  N/C  "), 4, 92);
    }
  }
  else if(sensorToDisplay==3)
  {
    myFiles.load(30, 35, 60, 51, "1temp3.raw",4);
    dtostrf(temperature3, 4, 1, tempstring);
    myGLCD.setFont(arial_bold);
    myGLCD.setColor(VGA_WHITE);
    myGLCD.print(tempstring, 20, 92);
    if(displayInC == true)myFiles.load(90, 94, 14, 12, "c.raw",4);
    if(displayInC == false)myFiles.load(90, 94, 14, 12, "f.raw",4);
  }
  else
  {
    //no sensors active, load blank image? 
    myFiles.load(36, 36, 48, 48, "1quest.raw");
  }
}

void Opaq_iaqua::checkDosing() // updates dosing info on the home screen
{
  char char3[4];  // used to convert int to char

  // just to note, that since we need to store a number higher than 255 in EEPROM, we have a high
  // and low byte when retrieved, the high byte is a number that is converted to the 10th multiple
  // then the low byte is added to the high byte to get our stored value
  // this gives us values up to 2559, which would be over 2L of ferts, so we are set

  long vol1_H=EEPROM.read(32); // read the remaining volume for pump 1 (high byte)
  long vol1_L=EEPROM.read(33); // read the remaining volume for pump 1 (low byte)
  long vol2_H=EEPROM.read(34); // read the remaining volume for pump 2 (high byte *10)
  long vol2_L=EEPROM.read(35); // read the remaining volume for pump 2 (low byte)
  long vol3_H=EEPROM.read(36); // read the remaining volume for pump 3 (high byte *10)
  long vol3_L=EEPROM.read(37); // read the remaining volume for pump 3 (low byte)

  long vol1 = (vol1_H*10)+vol1_L;
  long vol2 = (vol2_H*10)+vol2_L;
  long vol3 = (vol3_H*10)+vol3_L;

  long dose1Amt=EEPROM.read(20); // 20 // pump 1 dose in mL
  long dose2Amt=EEPROM.read(21); // 20 // pump 2 dose in mL
  long dose3Amt=EEPROM.read(22); // 20 // pump 3 dose in mL
  //unsigned int doseCap=EEPROM.read(26); // 26 // dosing reseviors capacity in mL*10;
  //doseCap=doseCap*10;  // to power of 10
  
  long dose1Cap=EEPROM.read(26);
  dose1Cap=dose1Cap*10;  // to power of 10
  long dose2Cap=EEPROM.read(261);
  dose2Cap=dose2Cap*10;  // to power of 10
  long dose3Cap=EEPROM.read(263);
  dose3Cap=dose3Cap*10;  // to power of 10

  long doses1=(vol1/dose1Amt);  // cacluate how many Macro doeses are left
  long doses2=(vol2/dose2Amt);  // cacluate how many Micro are left
  long doses3=(vol3/dose3Amt);  // cacluate how many Micro are left

 // here we prepare to draw the fill over the screen icons
  
  // draw empty fert tubes (red if less than 5 doses)
  if(doses1 > 4)myFiles.load(112, 122, 23, 50, "1ferts.raw",4);
  if(doses1 < 5)myFiles.load(112, 122, 23, 50, "1fertse.raw",4);
  if(doses2 > 4)myFiles.load(138, 122, 23, 50, "1ferts.raw",4);
  if(doses2 < 5)myFiles.load(138, 122, 23, 50, "1fertse.raw",4);
  if(doses3 > 4)myFiles.load(112, 183, 23, 50, "1ferts.raw",4);
  if(doses3 < 5)myFiles.load(112, 183, 23, 50, "1fertse.raw",4);
  
  // set initial Y values of an emtpy resevoir
  long y1mac=165;
  long y1mic=165;
  long y1exc=226;

  // calculate percentage left and generate the y1 coordinate for drawing the fill levels
  if (doses1>0) y1mac = 165 - ((vol1*40)/dose1Cap);  // for Macros
  if (doses2>0) y1mic = 165 - ((vol2*40)/dose2Cap);  // for Micros
  if (doses3>0) y1exc = 226 - ((vol3*40)/dose3Cap);  // for Excel

  // if the math above throws the pixels outside the tube, set it back to empty
  if ((y1mac > 165)||(y1mac < 125)) y1mac = 165;
  if ((y1mic > 165)||(y1mic < 125)) y1mic = 165;
  if ((y1exc > 225)||(y1exc < 186)) y1exc = 226;

  // fill top of fert tube with black as necessary
  myGLCD.setColor(0, 0, 0);
  myGLCD.fillRect(117,125,130,y1mac); //x1,y1,x2,y2
  myGLCD.fillRect(143,125,156,y1mic); 
  myGLCD.fillRect(117,186,130,y1exc); 

  //draw fert fill
  myGLCD.setColor(34, 81, 255);  // blue for macros
  myGLCD.fillRect(117,y1mac,130,165);
  myGLCD.setColor(255, 77, 0);  // orange for micros
  myGLCD.fillRect(143,y1mic,156,165);
  myGLCD.setColor(34, 255, 77);  // green for excel
  myGLCD.fillRect(117,y1exc,130,226);
  
  // values to center remaining dose numerical values over tubes on screen
  int xcharMacro=116;
  int xcharMicro=142;
  int xcharExcel=116;
  if (doses1 <= 9) xcharMacro=117;
  if (doses2 <= 9) xcharMicro=145;
  if (doses3 <= 9) xcharExcel=117;
  if (doses1 > 99) xcharMacro=111;
  if (doses2 > 99) xcharMicro=138;
  if (doses3 > 99) xcharExcel=111;
  
  // draw remaining dose numerical values
  myGLCD.setFont(Sinclair_S);
  myGLCD.setColor(255, 255, 255);
  itoa(doses1, char3, 10);
  myGLCD.print(char3, xcharMacro, 112);
  itoa(doses2, char3, 10);
  myGLCD.print(char3, xcharMicro, 112);
  itoa(doses3, char3, 10);
  myGLCD.print(char3, xcharExcel, 174);
}

void Opaq_iaqua::checkLighting()
{
  // display lighting info
  myGLCD.setColor(0, 0, 0);
  myGLCD.fillRect(57,111,104,236);

  if (currentLightMode==4) // active transition
  {
    myFiles.load(67, 118, 28, 28, lightModeSm[fadeFromMode],4);
    myFiles.load(75, 151, 12, 20, "1arrow.raw",4);
    myFiles.load(67, 174, 28, 28, lightModeSm[fadeToMode],4);

    // Get the time in seconds (since 1970)
    unsigned long rightNow = now();

    // calculate how much time is left in the fade in minutes
    unsigned long timeLeft=((fadeStartingSeconds + fadeDurationSeconds)-rightNow)/60;

    // convert to char
    int minsLeft=timeLeft;
    char char4[4];
    itoa(minsLeft, char4, 10);

    // center the minutes left
    int xmin=0;
    if (minsLeft<10) xmin=77;
    if ((minsLeft>9)&&(minsLeft<100)) xmin=73;
    if (minsLeft>99) xmin=69;

    // clear old data
    myGLCD.setColor(0, 0, 0);
    myGLCD.fillRect(69,207,93,215);

    // print to screen
    myGLCD.setFont(Sinclair_S);
    myGLCD.setColor(255, 255, 255);
    myGLCD.print(char4, xmin, 207);
    myGLCD.print("MIN", 69, 219);
  }
  else if (currentLightMode==5) 
  {//if unknown state print a question mark
    myFiles.load(57, 140, 48, 48, "1quest.raw");
  }
  else
  {// if we are not fading, then print the current mode
    myFiles.load(57, 140, 48, 48, lightMode[currentLightMode],4);
  }
}







void Opaq_iaqua::AlarmPwrLight1_On() 
{
  //digitalWrite(pwrLight1Pin, HIGH);
  globalPower.pwrLight1=1;
  if (dispScreen==1) myFiles.load(178, 121, 24, 24, pwrLightIconS[globalPower.pwrLight1]); // update home screen
}

void Opaq_iaqua::AlarmPwrLight2_On()
{
  //digitalWrite(pwrLight2Pin, HIGH);
  globalPower.pwrLight2=1;
  if (dispScreen==1) myFiles.load(206, 121, 24, 24, pwrLightIconS[globalPower.pwrLight2]); // update home screen
}

void Opaq_iaqua::AlarmPwrCO2_On()
{
  //digitalWrite(pwrCO2Pin, HIGH);
  globalPower.pwrCO2=1;
  if (dispScreen==1) myFiles.load(206, 177, 24, 24, pwrCO2IconS[globalPower.pwrCO2]); // update home screen
}

void Opaq_iaqua::AlarmPwrCirc_On()
{
  //digitalWrite(pwrCircPin, HIGH);
  globalPower.pwrCirc=1;
  if (feedingActive==true) preFeedPower.pwrCirc = 1;
  if (dispScreen==1) myFiles.load(206, 149, 24, 24, pwrCircIconS[globalPower.pwrCirc]); // update home screen
}

void Opaq_iaqua::AlarmPwrFilter_On()
{
  //digitalWrite(pwrFilterPin, HIGH);
  globalPower.pwrFilter=1;
  if (feedingActive==true) preFeedPower.pwrFilter = 0;
  if (dispScreen==1) myFiles.load(178, 149, 24, 24, pwrFilterIconS[globalPower.pwrFilter]); // update home screen
}

void Opaq_iaqua::AlarmPwrHeat_On()
{
  //digitalWrite(pwrHeatPin, HIGH);
  globalPower.pwrHeat=1;
  if (dispScreen==1) myFiles.load(178, 177, 24, 24, pwrHeatIconS[globalPower.pwrHeat]); // update home screen
}

void Opaq_iaqua::AlarmPwrAux1_On()
{
  //digitalWrite(pwrAux1Pin, HIGH);
  globalPower.pwrAux1=1;
  if (feedingActive==true) preFeedPower.pwrAux1 = 0;
  if (dispScreen==1) myFiles.load(178, 205, 24, 24, pwrAux1IconS[globalPower.pwrAux1]); // update home screen
}

void Opaq_iaqua::AlarmPwrAux2_On()
{
  //digitalWrite(pwrAux2Pin, HIGH);
  globalPower.pwrAux2=1;
  if (feedingActive==true) preFeedPower.pwrAux2 = 0;
  if (dispScreen==1) myFiles.load(206, 205, 24, 24, pwrAux2IconS[globalPower.pwrAux2]); // update home screen
}

void Opaq_iaqua::AlarmPwrLight1_Off()
{
  //digitalWrite(pwrLight1Pin, LOW);
  globalPower.pwrLight1 = 0;
  if (dispScreen==1) myFiles.load(178, 121, 24, 24, pwrLightIconS[globalPower.pwrLight1]); // update home screen
}

void Opaq_iaqua::AlarmPwrLight2_Off()
{
  //digitalWrite(pwrLight2Pin, LOW);
  globalPower.pwrLight2 = 0;
  if (dispScreen==1) myFiles.load(206, 121, 24, 24, pwrLightIconS[globalPower.pwrLight2]); // update home screen
}

void Opaq_iaqua::AlarmPwrCO2_Off()
{
  //digitalWrite(pwrCO2Pin, LOW);
  globalPower.pwrCO2 = 0;
  if (dispScreen==1) myFiles.load(206, 177, 24, 24, pwrCO2IconS[globalPower.pwrCO2]); // update home screen
}

void Opaq_iaqua::AlarmPwrCirc_Off()
{
  //digitalWrite(pwrCircPin, LOW);
  globalPower.pwrCirc = 0;
  if (feedingActive==true) preFeedPower.pwrCirc = 0;
  if (dispScreen==1) myFiles.load(206, 149, 24, 24, pwrCircIconS[globalPower.pwrCirc]); // update home screen
}

void Opaq_iaqua::AlarmPwrFilter_Off()
{
  //digitalWrite(pwrFilterPin, LOW);
  globalPower.pwrFilter = 0;
  if (feedingActive==true) preFeedPower.pwrFilter = 0;
  if (dispScreen==1) myFiles.load(178, 149, 24, 24, pwrFilterIconS[globalPower.pwrFilter]); // update home screen
}

void Opaq_iaqua::AlarmPwrHeat_Off()
{
  //digitalWrite(pwrHeatPin, LOW);
  globalPower.pwrHeat = 0;
  if (dispScreen==1) myFiles.load(178, 177, 24, 24, pwrHeatIconS[globalPower.pwrHeat]); // update home screen
}

void Opaq_iaqua::AlarmPwrAux1_Off()
{
  //digitalWrite(pwrAux1Pin, LOW);
  globalPower.pwrAux1 = 0;
  if (feedingActive==true) preFeedPower.pwrAux1 = 0;
  if (dispScreen==1) myFiles.load(178, 205, 24, 24, pwrAux1IconS[globalPower.pwrAux1]); // update home screen
}

void Opaq_iaqua::AlarmPwrAux2_Off()
{
  //digitalWrite(pwrAux2Pin, LOW);
  globalPower.pwrAux2 = 0;
  if (feedingActive==true) preFeedPower.pwrAux2 = 0;
  if (dispScreen==1) myFiles.load(206, 205, 24, 24, pwrAux2IconS[globalPower.pwrAux2]); // update home screen
}






// %%%%%%%%%%%%%%%%%%%%%%%%%%%
// FILES HAL
// %%%%%%%%%%%%%%%%%%%%%%%%%%%


void Files_Hal::load(int x, int y, int sx, int sy, char *filename, int bufmult)
{
  // file exists ?


  uint8_t chunck[1024];
  sprintf((char*)chunck, "/iaqua/%s", filename);
  
  // lets load and draw the file in chuncks
  File f = SPIFFS.open((char*)chunck, "r");
  if (!f) {
    Serial.println(F("File open has been failed."));
  }

  tft.area_update_start(x, y, sx, sy);

  for(int i=0; i < f.size(); i+=1024)
  {
    size_t last_bytes = f.readBytes((char*)chunck, 1024);
    
    // draw
    tft.area_update_data(chunck, last_bytes/2);
  }

  tft.area_update_end();

  f.close();
  
}


// %%%%%%%%%%%%%%%%%%%%%%%%%%%
// LCD HAL
// %%%%%%%%%%%%%%%%%%%%%%%%%%%
#include <Fonts/FreeSans9pt7b.h>
void Lcd_Hal::setFont(uint8_t* font)
{
  if(font != NULL)
  {
    tft.setFont(&FreeSans9pt7b);
    h=12;
  }
  else
  {
    tft.setFont(NULL);
    h=0;
  }
}

void Lcd_Hal::setColor(byte r, byte g, byte b)
{
  byte fch, fcl;
  
  fch=((r&248)|g>>5);
  fcl=((g&28)<<3|b>>3);

  color = (fcl&0xff) | (fch<<8);
}

void Lcd_Hal::setColor(const uint_least16_t& cl)
{
  color = cl;
}

void Lcd_Hal::drawLine(int x1, int y1, int x2, int y2)
{
  int lx = x2 - x1;
  int ly = y2 - y1;

  if(lx == 0)
    tft.drawFastVLine(x1, y1, ly, color );
  else
    tft.drawFastHLine(x1, y1, lx, color );
}

void Lcd_Hal::print(const __FlashStringHelper* st, int x, int y, int deg)
{
  char name[256];
  strcpy_P(name, (PGM_P)st);

  print(name, x, y, deg);
}

void Lcd_Hal::print(const char* st, int x, int y, int deg)
{
  
  tft.setCursor(x, y+h);
  tft.setTextColor(color);
  //tft.setTextSize(1);
  tft.println(st);
}

void Lcd_Hal::fillRect(int x1, int y1, int x2, int y2)
{
  tft.fillRect(x1, y1, x2-x1, y2-y1, color);
}

void Lcd_Hal::drawRect(int x1, int y1, int x2, int y2)
{
  
}

void Lcd_Hal::drawRoundRect(int x1, int y1, int x2, int y2)
{
  
}

void Lcd_Hal::clrScr()
{
  tft.fillScreen(ILI9341_BLACK);
}


// %%%%%%%%%%%%%%%%%%%%%%%%%%%
// LCD HAL
// %%%%%%%%%%%%%%%%%%%%%%%%%%%

uint8_t Storage_Hal::read(uint8_t id)
{
  return 0;
}

void Storage_Hal::write(uint8_t id, uint8_t data)
{
  
}

