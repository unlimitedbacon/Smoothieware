#include "ST7789.h"

#include "checksumm.h"
#include "Config.h"
#include "ConfigValue.h"
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

#define SPI_DEFAULT_FREQ        16000000 ///< Default SPI data clock frequency
#define ST7789_240x240_XSTART   0
#define ST7789_240x240_YSTART   0

ST7789::ST7789()
{
    // SPI Comm
    // Select which SPI channel to use
    int spi_channel = THEKERNEL->config->value(panel_checksum, spi_channel_checksum)->by_default(0)->as_number();
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
    this->spi->frequency(THEKERNEL->config->value(panel_checksum, spi_frequency_checksum)->by_default(16000000)->as_number()); //4Mhz freq, can try go a little lower
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
    uint16_t x2 = x1 + w;
    uint16_t y2 = y1 + h;

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

void ST7789::clear()
{
    startWrite();
    setAddrWindow(0, 0, LCD_WIDTH, LCD_HEIGHT);
    for (int i=0; i < (LCD_WIDTH * LCD_HEIGHT); i++) {
        spi->write(0x00);
        spi->write(0x00);
    }
    endWrite();
}

void ST7789::home()
{
    this->tx = 0;
    this->ty = 0;
}

void ST7789::setCursor(uint8_t col, uint8_t row)
{
    this->tx = col * FONT_WIDTH;
    this->ty = col * FONT_HEIGHT;
}

void ST7789::write(const char* line, int len)
{
}

uint8_t ST7789::readButtons()
{
    return 0;
}

int ST7789::readEncoderDelta()
{
    return 0;
}