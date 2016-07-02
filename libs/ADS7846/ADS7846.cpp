/*
  ADS7846/TSC2046 Touch-Controller Lib for Arduino
  by Watterott electronic (www.watterott.com)
 */

#include <inttypes.h>
#include <limits.h>

#if (defined(__AVR__) || defined(ARDUINO_ARCH_AVR))
# include <avr/io.h>
# include <avr/eeprom.h>
#endif
#if ARDUINO >= 100
# include "Arduino.h"
#else
# include "WProgram.h"
#endif

#include "SPI.h"
#include "ADS7846.h"



//# define BUSY_PIN       5
//# define IRQ_PIN        3
# define CS_PIN         (6) //SPI_HW_SS_PIN
# define MOSI_PIN       SPI_HW_MOSI_PIN
# define MISO_PIN       SPI_HW_MISO_PIN
# define SCK_PIN        SPI_HW_SCK_PIN


#ifdef ESP8266
#include "C:\Users\DeMatos\Dropbox\openlight_docs\openaq_controller\opaq\avr\coprocessor\protocol.h"
#endif


#ifndef ESP8266
  #define CS(mode)  digitalWrite(CS_PIN, mode)
#else
  #define CS(mode)   \
  digitalWrite(5, LOW); \
  delayMicroseconds(20); \
  SPI.transfer (ID_TSC2046_CSN); \
  delayMicroseconds(25); \
  SPI.transfer (mode); \
\
  delayMicroseconds(20); \
  digitalWrite(5, HIGH);
#endif

#define CS_DISABLE() CS(HIGH)
#define CS_ENABLE()  CS(LOW)

#define MIN_PRESSURE    10 //minimum pressure 1...254

#ifndef LCD_WIDTH
# define LCD_WIDTH      240
# define LCD_HEIGHT     320
#endif

#ifndef CAL_POINT_X1
# define CAL_POINT_X1   20
# define CAL_POINT_Y1   20
# define CAL_POINT1     {CAL_POINT_X1,CAL_POINT_Y1}
# define CAL_POINT_X2   LCD_WIDTH-20 //300
# define CAL_POINT_Y2   LCD_HEIGHT/2 //120
# define CAL_POINT2     {CAL_POINT_X2,CAL_POINT_Y2}
# define CAL_POINT_X3   LCD_WIDTH/2   //160
# define CAL_POINT_Y3   LCD_HEIGHT-20 //220
# define CAL_POINT3     {CAL_POINT_X3,CAL_POINT_Y3}
#endif

#define CMD_START       0x80
#define CMD_12BIT       0x00
#define CMD_8BIT        0x08
#define CMD_DIFF        0x00
#define CMD_SINGLE      0x04
#define CMD_X_POS       0x10
#define CMD_Z1_POS      0x30
#define CMD_Z2_POS      0x40
#define CMD_Y_POS       0x50
#define CMD_PWD         0x00
#define CMD_ALWAYSON    0x03


//-------------------- Constructor --------------------


ADS7846::ADS7846(void)
{
  return;
}


//-------------------- Public --------------------


void ADS7846::begin(void)
{
#ifndef ESP8266
  //init pins
  pinMode(CS_PIN, OUTPUT);
#endif

  CS_DISABLE();

#ifndef ESP8266
  pinMode(SCK_PIN, OUTPUT);
  pinMode(MOSI_PIN, OUTPUT);
  pinMode(MISO_PIN, INPUT);
#endif

#ifdef IRQ_PIN
  pinMode(IRQ_PIN, INPUT);
  digitalWrite(IRQ_PIN, HIGH); //pull-up
#endif
#ifdef BUSY_PIN
  pinMode(BUSY_PIN, INPUT);
  digitalWrite(BUSY_PIN, HIGH); //pull-up
#endif

  //set vars
  tp_matrix.div  = 0;
  tp_x           = 0;
  tp_y           = 0;
  tp_last_x      = 0;
  tp_last_y      = 0;
  lcd_x          = 0;
  lcd_y          = 0;
  pressure       = 0;
  is_calibrated  = false;
  setOrientation(0);

  tp_matrix.endpoints[MIN_ENDPOINT].x = USHRT_MAX;
  tp_matrix.endpoints[MAX_ENDPOINT].x = 0;
  tp_matrix.endpoints[MIN_ENDPOINT].y = USHRT_MAX;
  tp_matrix.endpoints[MAX_ENDPOINT].y = 0;
  invert_x = true;
  invert_y = true;
  swap_xy = true;

  return;
}


void ADS7846::setOrientation(uint_least16_t o)
{
  switch(o)
  {
    default:
    case   0:
      lcd_orientation =   0;
      break;
    case   9:
    case  90:
      lcd_orientation =  90;
      break;
    case  18:
    case 180:
      lcd_orientation = 180;
      break;
    case  27:
    case  14: //270&0xFF
    case 270:
      lcd_orientation = 270;
      break;
  }

  return;
}


void ADS7846::setRotation(uint_least16_t r)
{
  return setOrientation(r);
}


uint_least8_t ADS7846::setCalibration(CAL_POINT *lcd, CAL_POINT *tp)
{
  tp_matrix.div = ((tp[0].x - tp[2].x) * (tp[1].y - tp[2].y)) -
                  ((tp[1].x - tp[2].x) * (tp[0].y - tp[2].y));

  if(tp_matrix.div == 0)
  {
    return 0;
  }

  tp_matrix.a = ((lcd[0].x - lcd[2].x) * (tp[1].y - tp[2].y)) -
                ((lcd[1].x - lcd[2].x) * (tp[0].y - tp[2].y));

  tp_matrix.b = ((tp[0].x - tp[2].x) * (lcd[1].x - lcd[2].x)) -
                ((lcd[0].x - lcd[2].x) * (tp[1].x - tp[2].x));

  tp_matrix.c = (tp[2].x * lcd[1].x - tp[1].x * lcd[2].x) * tp[0].y +
                (tp[0].x * lcd[2].x - tp[2].x * lcd[0].x) * tp[1].y +
                (tp[1].x * lcd[0].x - tp[0].x * lcd[1].x) * tp[2].y;

  tp_matrix.d = ((lcd[0].y - lcd[2].y) * (tp[1].y - tp[2].y)) -
                ((lcd[1].y - lcd[2].y) * (tp[0].y - tp[2].y));

  tp_matrix.e = ((tp[0].x - tp[2].x) * (lcd[1].y - lcd[2].y)) -
                ((lcd[0].y - lcd[2].y) * (tp[1].x - tp[2].x));

  tp_matrix.f = (tp[2].x * lcd[1].y - tp[1].x * lcd[2].y) * tp[0].y +
                (tp[0].x * lcd[2].y - tp[2].x * lcd[0].y) * tp[1].y +
                (tp[1].x * lcd[0].y - tp[0].x * lcd[1].y) * tp[2].y;

  is_calibrated = true;

  return 1;
}

uint_least8_t ADS7846::setCalibration(uint_least32_t xmin, uint_least32_t ymin, uint_least32_t xmax, uint_least32_t ymax)
{
    tp_matrix.endpoints[MIN_ENDPOINT].x = xmin;
    tp_matrix.endpoints[MIN_ENDPOINT].y = ymin;
    tp_matrix.endpoints[MAX_ENDPOINT].x = xmax;
    tp_matrix.endpoints[MAX_ENDPOINT].y = ymax;
}


uint_least8_t ADS7846::getCalibrationMatrix(CAL_MATRIX& matrix)
{
  if(tp_matrix.div != 0)
  {
    matrix = tp_matrix;
    return 1;
  }

  return 0;
}

uint_least8_t ADS7846::setCalibration(CAL_MATRIX matrix)
{
  tp_matrix = matrix;
  is_calibrated = true;

  return 1;
}

void ADS7846::doCalEndPoint(LCD_HAL *lcd)
{
    unsigned short int minx=USHRT_MAX, miny=USHRT_MAX, maxx=0, maxy=0;

    lcd->fillScreen(RGB(255,255,255));
    lcd->drawText((lcd->getWidth()/2)-50, (lcd->getHeight()/2)-10, "X Y End Points", RGB(0,0,0), RGB(255,255,255), 1);

    while(getPressure() > MIN_PRESSURE){ service(); delay(100); };

    // get max and min X/Y
    for(int i=0; i < 100; )
    {
        service();
        delay(100);

        if(getPressure() > MIN_PRESSURE)
        {
            short int value = getXraw();
            short int value2 = getYraw();

            lcd->fillScreen(RGB(255,255,255));
            lcd->drawText((lcd->getWidth()/2)-50, (lcd->getHeight()/2)-10, "ENDPOINT X/Y", RGB(0,0,0), RGB(255,255,255), 1);

            lcd->drawText((lcd->getWidth()/2)+25, (lcd->getHeight()/2)+15, String(minx).c_str(), RGB(0,0,0), RGB(255,255,255), 1);
            lcd->drawText((lcd->getWidth()/2)-25, (lcd->getHeight()/2)+15, String(maxx).c_str(), RGB(0,0,0), RGB(255,255,255), 1);

            lcd->drawText((lcd->getWidth()/2)+25, (lcd->getHeight()/2)+30, String(miny).c_str(), RGB(0,0,0), RGB(255,255,255), 1);
            lcd->drawText((lcd->getWidth()/2)-25, (lcd->getHeight()/2)+30, String(maxy).c_str(), RGB(0,0,0), RGB(255,255,255), 1);

            lcd->drawText((lcd->getWidth()/2), (lcd->getHeight()/2)+40, String(i).c_str(), RGB(0,0,0), RGB(255,255,255), 1);


            if (maxx < value)
            {
                maxx = value;
            }

            if (minx > value)
            {
                minx = value;
            }

            if (maxy < value2)
            {
                maxy = value2;
            }

            if (miny > value2)
            {
                miny = value2;
            }

            i++;

        }
    }

    setCalibration(minx,miny,maxx,maxy);
}

uint_least8_t ADS7846::doCalibration(LCD_HAL *lcd) //example touch panel calibration routine
{
  uint_least8_t i;
  CAL_POINT lcd_points[3] = {CAL_POINT1, CAL_POINT2, CAL_POINT3}; //calibration points
  CAL_POINT tp_points[3];

  // let's first calibrate the end points
  doCalEndPoint(lcd);

  //clear screen and wait for touch release
  lcd->fillScreen(RGB(255,255,255));
  lcd->drawText((lcd->getWidth()/2)-50, (lcd->getHeight()/2)-10, "Calibration", RGB(0,0,0), RGB(255,255,255), 1);

  while(getPressure() > MIN_PRESSURE){ service(); delay(100); };

  //show calibration points
  for(i=0; i<3; )
  {
    //draw points
    lcd->drawCircle(lcd_points[i].x, lcd_points[i].y,  2, RGB(  0,  0,  0));
    lcd->drawCircle(lcd_points[i].x, lcd_points[i].y,  5, RGB(  0,  0,  0));
    lcd->drawCircle(lcd_points[i].x, lcd_points[i].y, 10, RGB(255,  0,  0));

    //run service routine
    service();

    delay(100);


    //press dectected? -> save point
    if(getPressure() > MIN_PRESSURE)
    {
      lcd->fillCircle(lcd_points[i].x, lcd_points[i].y, 2, RGB(255,0,0));
      tp_points[i].x = getXraw();
      tp_points[i].y = getYraw();
      i++;

      //wait and redraw screen
      delay(100);
      lcd->fillScreen(RGB(255,255,255));

      lcd->drawText((lcd->getWidth()/2)-50, (lcd->getHeight()/2)-10, "Calibration", RGB(0,0,0), RGB(255,255,255), 1);
      lcd->drawText((lcd->getWidth()/2)+25, (lcd->getHeight()/2)+50, String(getXraw()).c_str(), RGB(0,0,0), RGB(255,255,255), 1);
      lcd->drawText((lcd->getWidth()/2)-25, (lcd->getHeight()/2)+50, String(getYraw()).c_str(), RGB(0,0,0), RGB(255,255,255), 1);
    }
  }

  //calulate calibration matrix
  setCalibration(lcd_points, tp_points);

  //wait for touch release
  while(getPressure() > MIN_PRESSURE){ service(); };

  return 1;
}


/*uint_least8_t ADS7846::doCalibration(MI0283QT9 *lcd, uint16_t eeprom_addr, uint_least8_t check_eeprom) //example touch panel calibration routine
{
  uint_least8_t i;
  CAL_POINT lcd_points[3] = {CAL_POINT1, CAL_POINT2, CAL_POINT3}; //calibration point postions
  CAL_POINT tp_points[3];

  //calibration data in EEPROM?
  if(readCalibration(eeprom_addr) && check_eeprom)
  {
    return 0;
  }

  //clear screen and wait for touch release
  lcd->fillScreen(RGB(255,255,255));
#if (defined(__AVR__) || defined(ARDUINO_ARCH_AVR))
  lcd->drawTextPGM((lcd->getWidth()/2)-50, (lcd->getHeight()/2)-10, PSTR("Calibration"), RGB(0,0,0), RGB(255,255,255), 1);
#else
  lcd->drawText((lcd->getWidth()/2)-50, (lcd->getHeight()/2)-10, "Calibration", RGB(0,0,0), RGB(255,255,255), 1);
#endif
  while(getPressure() > MIN_PRESSURE){ service(); };

  //show calibration points
  for(i=0; i<3; )
  {
    //draw points
    lcd->drawCircle(lcd_points[i].x, lcd_points[i].y,  2, RGB(  0,  0,  0));
    lcd->drawCircle(lcd_points[i].x, lcd_points[i].y,  5, RGB(  0,  0,  0));
    lcd->drawCircle(lcd_points[i].x, lcd_points[i].y, 10, RGB(255,  0,  0));

    //run service routine
    service();

    //press dectected? -> save point
    if(getPressure() > MIN_PRESSURE)
    {
      lcd->fillCircle(lcd_points[i].x, lcd_points[i].y, 2, RGB(255,0,0));
      tp_points[i].x = getXraw();
      tp_points[i].y = getYraw();
      i++;

      //wait and redraw screen
      delay(100);
      lcd->fillScreen(RGB(255,255,255));
#if (defined(__AVR__) || defined(ARDUINO_ARCH_AVR))
      lcd->drawTextPGM((lcd->getWidth()/2)-50, (lcd->getHeight()/2)-10, PSTR("Calibration"), RGB(0,0,0), RGB(255,255,255), 1);
#else
      lcd->drawText((lcd->getWidth()/2)-50, (lcd->getHeight()/2)-10, "Calibration", RGB(0,0,0), RGB(255,255,255), 1);
#endif
    }
  }

  //calulate calibration matrix
  setCalibration(lcd_points, tp_points);

  //save calibration matrix
  writeCalibration(eeprom_addr);

  //wait for touch release
  while(getPressure() > MIN_PRESSURE){ service(); };

  return 1;
}*/


void ADS7846::calibrate(void)
{
  uint_least32_t x, y;

  //calc x pos
  if(tp_x != tp_last_x)
  {
    tp_last_x = tp_x;
    x = tp_x;
    y = tp_y;
    x = ((tp_matrix.a * x) + (tp_matrix.b * y) + tp_matrix.c) / tp_matrix.div;
         if(x < 0)          { x = 0; }
    else if(x >= LCD_WIDTH) { x = LCD_WIDTH-1; }
    lcd_x = x;
  }

  //calc y pos
  if(tp_y != tp_last_y)
  {
    tp_last_y = tp_y;
    x = tp_x;
    y = tp_y;
    y = ((tp_matrix.d * x) + (tp_matrix.e * y) + tp_matrix.f) / tp_matrix.div;
         if(y < 0)           { y = 0; }
    else if(y >= LCD_HEIGHT) { y = LCD_HEIGHT-1; }
    lcd_y = y;
  }

  return;
}


uint_least16_t ADS7846::getX(void)
{
  calibrate();

  switch(lcd_orientation)
  {
    case   0: return lcd_x;
    case  90: return lcd_y;
    case 180: return LCD_WIDTH-lcd_x;
    case 270: return LCD_HEIGHT-lcd_y;
  }

  return 0;
}


uint_least16_t ADS7846::getY(void)
{
  calibrate();

  switch(lcd_orientation)
  {
    case   0: return lcd_y;
    case  90: return LCD_WIDTH-lcd_x;
    case 180: return LCD_HEIGHT-lcd_y;
    case 270: return lcd_x;
  }

  return 0;
}


uint_least16_t ADS7846::getXraw(void)
{
  return tp_x;
}


uint_least16_t ADS7846::getYraw(void)
{
  return tp_y;
}


uint_least8_t ADS7846::getPressure(void)
{
  return pressure;
}


void ADS7846::service(void)
{
  rd_data();
  
  return;
}


//-------------------- Private --------------------


void ADS7846::rd_data(void)
{
  uint_least8_t p, a_x, a_y, b_x, b_y, a1, b1;
  uint_least16_t x, y;

  SPISettings spiSettings =  SPISettings(1000000, MSBFIRST, SPI_MODE0);

  SPI.beginTransaction(spiSettings);

  //get pressure
  CS_ENABLE();
  wr_spi(CMD_START | CMD_8BIT | CMD_DIFF | CMD_Z1_POS);
  a1 = rd_spi()&0x7F;
  wr_spi(CMD_START | CMD_8BIT | CMD_DIFF | CMD_Z2_POS);
  b1 = (255-rd_spi())&0x7F;
  CS_DISABLE();
  p = a1 + b1;

  if(p > MIN_PRESSURE)
  {
    CS_ENABLE();
    //get X data
    wr_spi(CMD_START | CMD_12BIT | CMD_DIFF | CMD_X_POS);
    a_x = rd_spi();
    b_x = rd_spi();

    //get Y data
    wr_spi(CMD_START | CMD_12BIT | CMD_DIFF | CMD_Y_POS);
    a_y = rd_spi();
    b_y = rd_spi();

    if(swap_xy)
    {
        y = ((a_x<<4)|(b_x>>4)); //12bit: ((a<<4)|(b>>4)) //10bit: ((a<<2)|(b>>6))
        x = ((a_y<<4)|(b_y>>4)); //12bit: ((a<<4)|(b>>4)) //10bit: ((a<<2)|(b>>6))
    }
    else
    {
        x = ((a_x<<4)|(b_x>>4)); //12bit: ((a<<4)|(b>>4)) //10bit: ((a<<2)|(b>>6))
        y = ((a_y<<4)|(b_y>>4)); //12bit: ((a<<4)|(b>>4)) //10bit: ((a<<2)|(b>>6))
    }

    if(tp_matrix.endpoints[MIN_ENDPOINT].x != USHRT_MAX && tp_matrix.endpoints[MIN_ENDPOINT].y != USHRT_MAX)
    {
        if (tp_matrix.endpoints[MIN_ENDPOINT].x <= x && x <= tp_matrix.endpoints[MAX_ENDPOINT].x)
        {
            if (invert_x)
                tp_x = tp_matrix.endpoints[MAX_ENDPOINT].x - x;
            else
                tp_x = x;
        }

        if (tp_matrix.endpoints[MIN_ENDPOINT].y <= y && y <= tp_matrix.endpoints[MAX_ENDPOINT].y)
        {
            if (invert_y)
                tp_y = tp_matrix.endpoints[MAX_ENDPOINT].y - y;
            else
                tp_y = y;
        }
    }
    else
    {
        tp_x = x;
        tp_y = y;
    }

    pressure = p;

    CS_DISABLE();
  }
  else
  {
    pressure = 0;
  }

  SPI.endTransaction();

  return;
}


uint_least8_t ADS7846::rd_spi(void)
{
  return SPI.transfer(0x00);
}


void ADS7846::wr_spi(uint_least8_t data)
{
  SPI.transfer(data);

  return;
}

bool ADS7846::isTouchCalibrated()
{
    return is_calibrated;
}
