#include "ST7789.h"

#include "checksumm.h"
#include "Config.h"
#include "ConfigValue.h"
#include "glcdfont.h"
#include "Kernel.h"
#include "StreamOutputPool.h"

#define LCD_WIDTH   320
#define LCD_HEIGHT  240
#define FONT_WIDTH  6
#define FONT_HEIGHT 8

#define panel_checksum             CHECKSUM("panel")
#define spi_channel_checksum       CHECKSUM("spi_channel")
#define spi_cs_pin_checksum        CHECKSUM("spi_cs_pin")
#define spi_frequency_checksum     CHECKSUM("spi_frequency")
#define rst_pin_checksum           CHECKSUM("rst_pin")
#define a0_pin_checksum            CHECKSUM("a0_pin")
#define click_button_pin_checksum  CHECKSUM("click_button_pin")
#define back_button_pin_checksum   CHECKSUM("back_button_pin")
#define pause_button_pin_checksum  CHECKSUM("pause_button_pin")
#define encoder_a_pin_checksum     CHECKSUM("encoder_a_pin")
#define encoder_b_pin_checksum     CHECKSUM("encoder_b_pin")
#define buzz_pin_checksum          CHECKSUM("buzz_pin")
#define led_pin_checksum           CHECKSUM("led_pin")

#define SPI_DEFAULT_FREQ        16000000 ///< Default SPI data clock frequency
#define ST7789_240x240_XSTART   0
#define ST7789_240x240_YSTART   0

ST7789::ST7789()
{
    // SPI Comm
    // Select which SPI channel to use
    int spi_channel = THEKERNEL->config->value(panel_checksum, spi_channel_checksum)->by_default(0)->as_int();
    PinName mosi, miso, sclk;
    if(spi_channel == 0) {
        mosi = P0_18; miso = P0_17; sclk = P0_15;
    } else if(spi_channel == 1) {
        mosi = P0_9; miso = P0_8; sclk = P0_7;
    } else {
        mosi = P0_18; miso = P0_17; sclk = P0_15;
    }

    this->spi = new mbed::SPI(mosi, miso, sclk);
    // Should see if we can run this any faster. Find out maximum speed for lpc1769
    this->spi->frequency(THEKERNEL->config->value(panel_checksum, spi_frequency_checksum)->by_default(SPI_DEFAULT_FREQ)->as_int()); //4Mhz freq, can try go a little lower
    //this->spi->frequency(32000000);
    this->spi->format(8, 0);

    // Chip Select Pin
    this->cs.from_string(THEKERNEL->config->value( panel_checksum, spi_cs_pin_checksum)->by_default("0.16")->as_string())->as_output();
    cs.set(1);

    // LCD Reset Pin
    this->rst.from_string(THEKERNEL->config->value( panel_checksum, rst_pin_checksum)->by_default("nc")->as_string())->as_output();
    if(this->rst.connected()) rst.set(1);

    // Data/Command Select Pin
    this->a0.from_string(THEKERNEL->config->value( panel_checksum, a0_pin_checksum)->by_default("nc")->as_string())->as_output();
    if(a0.connected()) a0.set(1);

    // Setup Controls
    this->click_pin.from_string(THEKERNEL->config->value( panel_checksum, click_button_pin_checksum )->by_default("nc")->as_string())->as_input();
    this->back_pin.from_string(THEKERNEL->config->value( panel_checksum, back_button_pin_checksum )->by_default("nc")->as_string())->as_input();
    this->pause_pin.from_string(THEKERNEL->config->value( panel_checksum, pause_button_pin_checksum )->by_default("nc")->as_string())->as_input();
    this->encoder_a_pin.from_string(THEKERNEL->config->value( panel_checksum, encoder_a_pin_checksum)->by_default("nc")->as_string())->as_input();
    this->encoder_b_pin.from_string(THEKERNEL->config->value( panel_checksum, encoder_b_pin_checksum)->by_default("nc")->as_string())->as_input();

    // Lights and Buzzer
    this->buzz_pin.from_string(THEKERNEL->config->value( panel_checksum, buzz_pin_checksum)->by_default("nc")->as_string())->as_output();
    this->led_pin.from_string(THEKERNEL->config->value( panel_checksum, led_pin_checksum)->by_default("nc")->as_string())->as_output();
}

ST7789::~ST7789()
{
    delete this->spi;
}

/**************************************************************************/
/*!
    @brief  Call before issuing command(s) or data to display. Performs
            chip-select (if required) and starts an SPI transaction (if
            using hardware SPI and transactions are supported). Required
            for all display types; not an SPI-specific function.
*/
/**************************************************************************/
void ST7789::startWrite(void) {
    cs.set(0);
}

/**************************************************************************/
/*!
    @brief  Call after issuing command(s) or data to display. Performs
            chip-deselect (if required) and ends an SPI transaction (if
            using hardware SPI and transactions are supported). Required
            for all display types; not an SPI-specific function.
*/
/**************************************************************************/
void ST7789::endWrite(void) {
    cs.set(1);
}

/**************************************************************************/
/*!
    @brief  Write a single command byte to the display. Chip-select and
            transaction must have been previously set -- this ONLY sets
            the device to COMMAND mode, issues the byte and then restores
            DATA mode. There is no corresponding explicit writeData()
            function -- just use spiWrite().
    @param  cmd  8-bit command to write.
*/
/**************************************************************************/
int ST7789::writeCommand(uint8_t cmd) {
    int r;
    if(a0.connected()) a0.set(0);
    r = spi->write(cmd);
    if(a0.connected()) a0.set(1);
    return r;
}

/**************************************************************************/
/*!
    @brief  Set origin of (0,0) and orientation of TFT display
    @param  m  The index for rotation, from 0-3 inclusive
*/
/**************************************************************************/
void ST7789::setRotation(uint8_t m) {
    uint8_t madctl = 0;

    uint8_t rotation = m & 3; // can't be higher than 3

    switch (rotation) {
        case 0:
            madctl  = ST77XX_MADCTL_MX | ST77XX_MADCTL_MY | ST77XX_MADCTL_RGB;
            _xstart = _colstart;
            _ystart = _rowstart;
            break;
        case 1:
            madctl  = ST77XX_MADCTL_MY | ST77XX_MADCTL_MV | ST77XX_MADCTL_RGB;
            _xstart = _rowstart;
            _ystart = _colstart;
            break;
        case 2:
            madctl  = ST77XX_MADCTL_RGB;
            _xstart = 0;
            _ystart = 0;
            break;
        case 3:
            madctl  = ST77XX_MADCTL_MX | ST77XX_MADCTL_MV | ST77XX_MADCTL_RGB;
            _xstart = 0;
            _ystart = 0;
            break;
    }
    startWrite();
    writeCommand(ST77XX_MADCTL);
    spi->write(madctl);
    endWrite();
}

/**************************************************************************/
/*!
  @brief  SPI displays set an address window rectangle for blitting pixels
  @param  x  Top left corner x coordinate
  @param  y  Top left corner x coordinate
  @param  w  Width of window
  @param  h  Height of window
*/
/**************************************************************************/
void ST7789::setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    //x += _xstart;
    //y += _ystart;
    //uint32_t xa = ((uint32_t)x << 16) | (x+w-1);
    //uint32_t ya = ((uint32_t)y << 16) | (y+h-1); 

    uint16_t x1 = x + _xstart;
    uint16_t y1 = y + _ystart;
    uint16_t x2 = x1 + w - 1;
    uint16_t y2 = y1 + h - 1;

    writeCommand(ST77XX_CASET); // Column addr set
    spi->write((uint8_t)(x1 >> 8));
    spi->write((uint8_t)(x1 & 0xff));
    spi->write((uint8_t)(x2 >> 8));
    spi->write((uint8_t)(x2 & 0xff));
    //spi->write(xa);
    //SPI_WRITE32(xa);

    writeCommand(ST77XX_RASET); // Row addr set
    spi->write((uint8_t)(y1 >> 8));
    spi->write((uint8_t)(y1 & 0xff));
    spi->write((uint8_t)(y2 >> 8));
    spi->write((uint8_t)(y2 & 0xff));
    //spi->write(ya);
    //SPI_WRITE32(ya);

    writeCommand(ST77XX_RAMWR); // write to RAM
}

void ST7789::getID()
{
    // Get screen id
    int id[8];
    startWrite();
    writeCommand(ST77XX_RDDID);
    for (int i=0; i<8; i++) {
        id[i] = spi->write(0);
    }
    endWrite();

    THEKERNEL->streams->printf("ID:");
    for (int i=0; i<8; i++) {
        THEKERNEL->streams->printf(" %x", id[i]);
    }
    THEKERNEL->streams->printf("\n");

    startWrite();
    id[0] = writeCommand(ST77XX_RDID1);
    id[1] = spi->write(0);
    endWrite();

    startWrite();
    id[2] = writeCommand(ST77XX_RDID2);
    id[3] = spi->write(1);
    endWrite();

    startWrite();
    id[4] = writeCommand(ST77XX_RDID3);
    id[5] = spi->write(2);
    endWrite();

    startWrite();
    id[6] = writeCommand(ST77XX_RDID4);
    id[7] = spi->write(3);
    endWrite();

    THEKERNEL->streams->printf("ID:");
    for (int i=0; i<8; i++) {
        THEKERNEL->streams->printf(" %x", id[i]);
    }
    THEKERNEL->streams->printf("\n");
}

void ST7789::getStatus()
{
    int stat[10];
    startWrite();
    writeCommand(ST77XX_RDDST);
    for (int i=0; i<10; i++) {
        stat[i] = spi->write(0);
    }
    endWrite();

    THEKERNEL->streams->printf("Status:");
    for (int i=0; i<10; i++) {
        THEKERNEL->streams->printf(" %x", stat[i]);
    }
    THEKERNEL->streams->printf("\n");

    startWrite();
    stat[0] = writeCommand(ST77XX_RDDPM);
    stat[1] = spi->write(0);
    endWrite();

    startWrite();
    stat[2] = writeCommand(ST77XX_RDDCOLMOD);
    stat[3] = spi->write(0);
    endWrite();

    startWrite();
    stat[4] = writeCommand(ST77XX_RDDIM);
    stat[5] = spi->write(0);
    endWrite();

    THEKERNEL->streams->printf("Status:");
    for (int i=0; i<10; i++) {
        THEKERNEL->streams->printf(" %x", stat[i]);
    }
    THEKERNEL->streams->printf("\n");
}

void ST7789::init()
{
    THEKERNEL->streams->printf("Initializing ST7789 LCD\n");

    _colstart = 0;
    _rowstart = 0;

    // Pulse the reset pin if it exists
    if (this->rst.connected()) {
        rst.set(0);
        wait_us(20);
        rst.set(1);
    }

    // Init commands for 7789 screens
    const unsigned char init_seq[] = {
        9,                                  //  9 commands in list:
        ST77XX_SWRESET,   ST_CMD_DELAY,     //  1: Software reset, no args, w/delay
          150,                              //    150 ms delay
        ST77XX_SLPOUT ,   ST_CMD_DELAY,     //  2: Out of sleep mode, no args, w/delay
          255,                              //     255 = 500 ms delay
        ST77XX_COLMOD , 1+ST_CMD_DELAY,     //  3: Set color mode, 1 arg + delay:
          0x55,                             //     16-bit color
          10,                               //     10 ms delay
        ST77XX_MADCTL , 1,                  //  4: Mem access ctrl (directions), 1 arg:
          0x08,                             //     Row/col addr, bottom-top refresh
        ST77XX_CASET  , 4,                  //  5: Column addr set, 4 args, no delay:
          0x00,
          ST7789_240x240_XSTART,            //     XSTART = 0
          (240+ST7789_240x240_XSTART)>>8,
          (240+ST7789_240x240_XSTART)&0xFF, //     XEND = 240
        ST77XX_RASET  , 4,                  //  6: Row addr set, 4 args, no delay:
          0x00,
          ST7789_240x240_YSTART,            //     YSTART = 0
          (320+ST7789_240x240_YSTART)>>8,
          (320+ST7789_240x240_YSTART)&0xFF, //     YEND = 240
        ST77XX_INVOFF  ,   ST_CMD_DELAY,    //  7: hack
          10,
        ST77XX_NORON  ,   ST_CMD_DELAY,     //  8: Normal display on, no args, w/delay
          10,                               //     10 ms delay
        ST77XX_DISPON ,   ST_CMD_DELAY,     //  9: Main screen turn on, no args, delay
          255                               //     255 = max (500 ms) delay
    };

    uint8_t  cmd, numCommands, numArgs;
    uint16_t ms;

    uint8_t index = 0;
    numCommands = init_seq[index++];        // Number of commands to follow
    THEKERNEL->streams->printf("%i\n",numCommands);
    while (numCommands--) {                 // For each command...

        startWrite();

        cmd = init_seq[index++];
        writeCommand(cmd);                  // Read, issue command

        numArgs  = init_seq[index++];       // Number of args to follow
        ms       = numArgs & ST_CMD_DELAY;  // If hibit set, delay follows args
        numArgs &= ~ST_CMD_DELAY;           // Mask out delay bit
        while (numArgs--) {                 // For each argument...
            spi->write(init_seq[index++]);  // Read, issue argument
        }
        
        endWrite();                         // ST7789 needs chip deselect after each

        THEKERNEL->streams->printf("Cmd: %x %i\n", cmd, numArgs);

        if (ms) {
            ms = init_seq[index++];         // Read post-command delay time (ms)
            if(ms == 255) ms = 500;         // If 255, delay for 500 ms
            wait_ms(ms);
        }
    }

    setRotation(3);

    getID();
    getStatus();

    // Invert
    //startWrite();
    //writeCommand(ST77XX_INVON);
    //endWrite();

    //getStatus();

    //// Test shutoff
    //cs.set(0);
    //if (a0.connected()) a0.set(0);
    //spi->write(ST77XX_DISPOFF);
    //if (a0.connected()) a0.set(1);
    //cs.set(1);

    //getStatus();

    //// Test shutoff
    //cs.set(0);
    //if (a0.connected()) a0.set(0);
    //spi->write(ST77XX_NORON);
    //if (a0.connected()) a0.set(1);
    //cs.set(1);

    //getStatus();

    clear();
}

void ST7789::display()
{
    // Nothing
}

/*!
    @brief  Draw a filled rectangle to the display. Self-contained and
            provides its own transaction as needed (see writeFillRect() or
            writeFillRectPreclipped() for lower-level variants). Edge
            clipping and rejection is performed here.
    @param  x      Horizontal position of first corner.
    @param  y      Vertical position of first corner.
    @param  w      Rectangle width in pixels (positive = right of first
                   corner, negative = left of first corner).
    @param  h      Rectangle height in pixels (positive = below first
                   corner, negative = above first corner).
    @param  color  16-bit fill color in '565' RGB format.
*/
void ST7789::fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    uint8_t c1 = color >> 8;
    uint8_t c2 = color & 0xff;

    startWrite();
    setAddrWindow(x, y, w, h);
    for (int i=0; i < (w * h); i++) {
        spi->write(c1);
        spi->write(c2);
    }
    endWrite();
}

void ST7789::clear()
{
    fillRect(0, 0, LCD_WIDTH, LCD_HEIGHT, 0x0000);
    this->tx = 0;
    this->ty = 0;
    this->fgColor = 0xffff;
    this->bgColor = 0x0000;
}

void ST7789::pixel(int x, int y, int mode) {
    uint8_t c1, c2;
    if (mode) {
        c1 = this->fgColor >> 8;
        c2 = this->fgColor & 0xff;
    } else {
        c1 = this->bgColor >> 8;
        c2 = this->bgColor & 0xff;
    }

    startWrite();
    setAddrWindow(x, y, 1, 1);
    spi->write(c1);
    spi->write(c2);
    endWrite();
}

void ST7789::drawHLine(int x, int y, int w, int mode) {
    fillRect(x, y, w, 1, mode ? this->fgColor : this->bgColor);
}

void ST7789::drawVLine(int x, int y, int h, int mode) {
    fillRect(x, y, 1, h, mode ? this->fgColor : this->bgColor);
}

void ST7789::drawBox(int x, int y, int w, int h, int mode) {
    fillRect(x, y, w, h, mode ? this->fgColor : this->bgColor);
}

void ST7789::bltGlyph(int x, int y, int w, int h, const uint8_t *glyph, int span, int x_offset, int y_offset) {
    uint8_t c1 = this->fgColor >> 8;
    uint8_t c2 = this->fgColor & 0xff;
    uint8_t b1 = this->bgColor >> 8;
    uint8_t b2 = this->bgColor & 0xff;

    int rowBytes;
    if (span) {
        rowBytes = span;
    } else {
        rowBytes = w / 8;
        if (w % 8) rowBytes++;
    }

    startWrite();
    setAddrWindow(x, y, w, h);
    for (int gy = y_offset; gy < (h + y_offset); gy++) {
        for (int gx = x_offset; gx < (w + x_offset); gx++) {
            uint8_t glyphByte = glyph[gy * rowBytes + (gx / 8)] << (gx % 8);
            if (glyphByte & 0x80) {
                spi->write(c1);
                spi->write(c2);
            } else {
                spi->write(b1);
                spi->write(b2);
            }
        }
    }
    endWrite();
}

void ST7789::setColor(uint16_t c) {
    this->fgColor = c;
}

void ST7789::setBackgroundColor(uint16_t c) {
    this->bgColor = c;
}

// Draw a character
/**************************************************************************/
/*!
   @brief   Draw a single character
    @param    x   Bottom left corner x coordinate
    @param    y   Bottom left corner y coordinate
    @param    c   The 8-bit font-indexed character (likely ascii)
    @param    color 16-bit 5-6-5 Color to draw character with
    @param    bg 16-bit 5-6-5 Color to fill background with (if same as color, no background)
*/
/**************************************************************************/ 
void ST7789::drawChar(int x, int y, unsigned char c, uint16_t color, uint16_t bg) {
    if (c == '\n') {
        this->ty += 8;
    } else if (c == '\r') {
    } else {
        uint8_t c1, c2, b1, b2;

        // This is not the right thing to do but it's better than nothing
        if (!this->text_draw_mode) {
            c1 = bg >> 8;
            c2 = bg & 0xff;
            b1 = color >> 8;
            b2 = color & 0xff;
        } else {
            c1 = color >> 8;
            c2 = color & 0xff;
            b1 = bg >> 8;
            b2 = bg & 0xff;
        }

        startWrite();
        setAddrWindow(x, y, 6, 8);
        for (uint8_t fx = 0; fx < 8; fx++) {
            for (uint8_t fy = 0; fy < 5; fy++) {
                uint8_t fontByte = glcd_font[(c * 5) + fy] >> fx;
                if (fontByte & 1) {
                    spi->write(c1);
                    spi->write(c2);
                } else {
                    spi->write(b1);
                    spi->write(b2);
                }
            }
            spi->write(b1);
            spi->write(b2);
        }

        endWrite();
        this->tx += 6;
    }
}

void ST7789::home()
{
    this->tx = 0;
    this->ty = 0;
}

void ST7789::setCursor(uint8_t col, uint8_t row)
{
    this->tx = col * FONT_WIDTH;
    this->ty = row * FONT_HEIGHT;
}

void ST7789::setCursorPX(int x, int y)
{
    this->tx = x;
    this->ty = y;
}

void ST7789::setDrawMode(int c)
{
    this->text_draw_mode = c;
}

void ST7789::write(const char* line, int len)
{
    for (int i = 0; i < len; ++i) {
        drawChar(this->tx, this->ty, line[i], this->fgColor, this->bgColor);
    }
}

uint8_t ST7789::readButtons()
{
    uint8_t state = 0;
    state |= (this->click_pin.get() ? BUTTON_SELECT : 0);
    state |= (this->back_pin.get() ? BUTTON_LEFT : 0);
    state |= (this->pause_pin.get() ? BUTTON_PAUSE : 0);
    return state;
}

int ST7789::readEncoderDelta()
{
    static int8_t enc_states[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
    static uint8_t old_AB = 0;
    if (this->encoder_a_pin.connected()) {
        // mviki
        old_AB <<= 2;                   //remember previous state
        old_AB |= ( this->encoder_a_pin.get() + ( this->encoder_b_pin.get() * 2 ) );  //add current state
        return  enc_states[(old_AB & 0x0f)];

    } else {
        return 0;
    }
}

// cycle the buzzer pin at a certain frequency (hz) for a certain duration (ms)
void ST7789::buzz(long duration, uint16_t freq)
{
    if(!this->buzz_pin.connected()) return;

    duration *= 1000; //convert from ms to us
    long period = 1000000 / freq; // period in us
    long elapsed_time = 0;
    while (elapsed_time < duration) {
        this->buzz_pin.set(1);
        wait_us(period / 2);
        this->buzz_pin.set(0);
        wait_us(period / 2);
        elapsed_time += (period);
    }
}

void ST7789::setLed(int led, bool onoff)
{
    if(led == LED_HOT) {
        led_pin.set(onoff);
    }
}