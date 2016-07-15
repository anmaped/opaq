#ifndef ADS7846_h
#define ADS7846_h

#include <inttypes.h>

typedef struct
{
  uint_least32_t x;
  uint_least32_t y;
} CAL_POINT; //calibration points for touchpanel


#define MIN_ENDPOINT 0
#define MAX_ENDPOINT 1

typedef struct
{
  uint_least32_t a;
  uint_least32_t b;
  uint_least32_t c;
  uint_least32_t d;
  uint_least32_t e;
  uint_least32_t f;
  uint_least32_t div;
  CAL_POINT endpoints[2];
} CAL_MATRIX; //calibration matrix for touchpanel

#define RGB(r,g,b) ((r & 0x1f) << 11) | ((g & 0x3f) << 5) | ((b & 0x1f) << 0)

class LCD_HAL
{
public:
    virtual void fillScreen(int) const = 0;
    virtual void drawText( int, int, const char*, int, int, int ) const = 0;
    virtual int  getWidth() const = 0;
    virtual int  getHeight() const = 0;
    virtual void drawCircle( int, int, int, int ) const = 0;
    virtual void fillCircle( int, int, int, int ) const = 0;
};

class ADS7846
{
  public:

    bool invert_x, invert_y, swap_xy;

    uint_least16_t lcd_orientation;      //lcd_orientation
    uint_least16_t lcd_x, lcd_y;         //calibrated pos (screen)
    uint_least16_t tp_x, tp_y;           //raw pos (touch panel)
    uint_least16_t tp_last_x, tp_last_y; //last raw pos (touch panel)

    CAL_MATRIX tp_matrix;                //calibrate matrix
    uint_least8_t pressure;              //touch panel pressure
    bool is_calibrated;

    ADS7846();

    void begin(void);
    void setOrientation(uint_least16_t o);
    void setRotation(uint_least16_t r);
    uint_least8_t setCalibration(CAL_POINT *lcd, CAL_POINT *tp);
    uint_least8_t setCalibration(uint_least32_t xmin, uint_least32_t ymin, uint_least32_t xmax, uint_least32_t ymax);
    uint_least8_t getCalibrationMatrix(CAL_MATRIX& matrix);
    uint_least8_t setCalibration(CAL_MATRIX matrix);
    void doCalEndPoint(LCD_HAL *lcd);
    uint_least8_t doCalibration(LCD_HAL *lcd); //example touch panel calibration routine
    void calibrate(void);
    uint_least16_t getX(void);
    uint_least16_t getY(void);
    uint_least16_t getXraw(void);
    uint_least16_t getYraw(void);
    uint_least8_t getPressure(void);
    void service(void);
    bool isTouchCalibrated();

  private:
    void rd_data(void);
    uint_least8_t rd_spi(void);
    void wr_spi(uint_least8_t data);
};


#endif //ADS7846_h
