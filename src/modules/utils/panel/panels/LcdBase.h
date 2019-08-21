#ifndef LCDBASE_H
#define LCDBASE_H

#include "stdint.h"

// Standard directional button bits
#define BUTTON_SELECT 0x01
#define BUTTON_RIGHT  0x02
#define BUTTON_DOWN   0x04
#define BUTTON_UP     0x08
#define BUTTON_LEFT   0x10
#define BUTTON_PAUSE  0x20
#define BUTTON_AUX1   0x40
#define BUTTON_AUX2   0x80

// specific LED assignments
#define LED_FAN_ON    1
#define LED_HOTEND_ON 2
#define LED_BED_ON    3
#define LED_HOT       4

class Panel;

class LcdBase {
    public:
        LcdBase();
        virtual ~LcdBase();
        virtual void init()= 0;
        // only use this to display lines
        int printf(const char* format, ...);

        void setPanel(Panel* p) { panel= p; }

        // Required LCD functions
        virtual void home()= 0;
        virtual void clear()= 0;
        virtual void display()= 0;
        virtual void setCursor(uint8_t col, uint8_t row)= 0;

        // Returns button states including the encoder select button
        virtual uint8_t readButtons()= 0;

        // returns the current encoder position
        virtual int readEncoderDelta()= 0;

        // the number of encoder clicks per detent. this is divided into
        // accumulated clicks for control values so one detent is one
        // increment, this varies depending on encoder type usually 1,2 or 4
        virtual int getEncoderResolution()= 0;

        // optional
        virtual void setLed(int led, bool onoff){};
        virtual void setLedBrightness(int led, int val){};
        virtual void buzz(long,uint16_t){};
        virtual bool hasGraphics() { return false; }
        virtual bool hasFullGraphics() { return false; }     // Temporary. Remove this once RRDGLCD has more graphics functions implemented.
        virtual bool encoderReturnsDelta() { return false; } // set to true if the panel handles encoder clicks and returns a delta
        virtual uint8_t getContrast() { return 0; }
        virtual void setContrast(uint8_t c) { }

        // Graphics Functions
        // on graphics panels, the input bitmap is in X windows XBM format but
        // with the bits in a byte reversed so bit7 is left most and bit0 is
        // right most. x_offset must by byte aligned if used
        virtual void bltGlyph(int x, int y, int w, int h, const uint8_t *glyph, int span= 0, int x_offset=0, int y_offset=0){}

        /**
         * Sets the exact position of the text cursor on graphical displays. Takes the coordinates of the top left
         * of the next character to be printed. Not used for text-only displays.
         * 
         * @param x X coordinate
         * @param y Y coordinate
         */
        virtual void setCursorPX(int x, int y) {};

        /**
         * Sets the mode used for drawing the foreground when printing text. 
         * 
         * @param m 0: Turn pixels off (AND logic)
         *          1: Turn pixels on (OR logic) (default)
         *          2: Invert pixels (XOR logic)
         */
        virtual void setDrawMode(int m) {};

        /**
        * Turns on/off drawing the background behind text. If on, the background color will be the inverse of the text color.
        * If off the background will be transparent. Defaults to on.
        * 
        * @param bg True: Background on (default), False: Background off (transparent)
        */
        virtual void setBackground(bool bg) {};

        /**
         * Sets the color used for all drawing operations.
         * Only works on color (RGB) screens. Has no effect on monochrome screens.
         * Takes a 16 bit RGB 5-6-5 color value. 5 Red bits, 6 Green bits, 5 Blue bits.
         * Red:   0xf800
         * Green: 0x07e0
         * Blue:  0x001f
         * 
         * @param c 16 bit RGB 5-6-5 color value.
         */
        virtual void setColor(uint16_t c) {};

        /**
         * Sets the background color used for all drawing operations with background pixels.
         * Only works on color (RGB) screens. Has no effect on monochrome screens.
         * 
         * @param c Takes a 16 bit RGB 5-6-5 color value. 5 Red bits, 6 Green bits, 5 Blue bits.
         */
        virtual void setBackgroundColor(uint16_t c) {};

        /**
         * Turns on/off a specific pixel on the display. The screen origin (0,0) is at the top left of the display.
         * 
         * @param x X coordinate
         * @param y Y coordinate
         * @param mode Mode to use for drawing the pixel (see setDrawMode() for options)
         */
        virtual void pixel(int x, int y, int mode = 1) {};

        /**
         * Draws a horizontal line.
         * 
         * @param x X coordinate of start of line
         * @param y Y coordinate of start of line
         * @param w Width of the line
         * @param mode Mode to use for drawing (see setDrawMode() for options)
         */
        virtual void drawHLine(int x, int y, int w, int mode = 1) {};

        /**
         * Draws a vertical line.
         * 
         * @param x X coordinate of start of line
         * @param y Y coordinate of start of line
         * @param h Height of the line
         * @param mode Mode to use for drawing (see setDrawMode() for options)
         */
        virtual void drawVLine(int x, int y, int h, int mode = 1) {};

        /**
         * Draws a filled rectangle.
         * 
         * @param x X coordinate of left side
         * @param y Y coordinate of top
         * @param w Width of rectangle
         * @param h Height of rectangle
         * @param mode Mode to use for drawing (see setDrawMode() for options)
         */
        virtual void drawBox(int x, int y, int w, int h, int mode = 1) {};

        // only used on certain panels
        virtual void on_refresh(bool now= false){};
        virtual void on_main_loop(){};
        // override this if the panel can handle more or less screen lines
        virtual uint16_t get_screen_lines() { return 4; }
        // used to set a variant for a panel (like viki2 vs st7565)
        virtual void set_variant(int n) {};

    protected:
        Panel* panel;
        virtual void write(const char* line, int len)= 0;

};

#endif // LCDBASE_H

