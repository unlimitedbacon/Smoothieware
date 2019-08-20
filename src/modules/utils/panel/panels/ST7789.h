#ifndef ST7789_H_
#define ST7789_H_

#include "LcdBase.h"
#include "mbed.h"
#include "libs/Pin.h"

// SPI Commands
#define ST_CMD_DELAY      0x80    // special signifier for command lists

#define ST77XX_NOP        0x00
#define ST77XX_SWRESET    0x01
#define ST77XX_RDDID      0x04
#define ST77XX_RDDST      0x09
#define ST77XX_RDDPM      0x0A
#define ST77XX_RDDMADCTL  0x0B
#define ST77XX_RDDCOLMOD  0x0C
#define ST77XX_RDDIM      0x0D

#define ST77XX_SLPIN      0x10
#define ST77XX_SLPOUT     0x11
#define ST77XX_PTLON      0x12
#define ST77XX_NORON      0x13

#define ST77XX_INVOFF     0x20
#define ST77XX_INVON      0x21
#define ST77XX_DISPOFF    0x28
#define ST77XX_DISPON     0x29
#define ST77XX_CASET      0x2A
#define ST77XX_RASET      0x2B
#define ST77XX_RAMWR      0x2C
#define ST77XX_RAMRD      0x2E

#define ST77XX_PTLAR      0x30
#define ST77XX_COLMOD     0x3A
#define ST77XX_MADCTL     0x36

#define ST77XX_MADCTL_MY  0x80
#define ST77XX_MADCTL_MX  0x40
#define ST77XX_MADCTL_MV  0x20
#define ST77XX_MADCTL_ML  0x10
#define ST77XX_MADCTL_RGB 0x00

#define ST77XX_RDID1      0xDA
#define ST77XX_RDID2      0xDB
#define ST77XX_RDID3      0xDC
#define ST77XX_RDID4      0xDD

class ST7789 : public LcdBase {
    public:
        ST7789();
        ~ST7789();

        // Required functions
        void init();
        void display();

        // Graphics functions
        void clear();

        // Text functions
        void home();
        void setCursor(uint8_t col, uint8_t row);
        void write(const char* line, int len);

        // Physical Controls
        uint8_t readButtons();
        int readEncoderDelta();
        int getEncoderResolution() { return 4; };

    private:
        // Driver functions
        void startWrite();
        void endWrite();
        int  writeCommand(uint8_t cmd);
        void setRotation(uint8_t m);
        void setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
        void getID();
        void getStatus();

        mbed::SPI* spi;

        // Pins
        Pin cs;
        Pin rst;
        Pin a0;

        // Text drawing state
        int tx, ty;

        uint32_t _xstart = 0;
        uint32_t _ystart = 0;
        uint8_t _colstart = 0;
        uint8_t _rowstart = 0;
};

#endif /* ST7789_H_ */