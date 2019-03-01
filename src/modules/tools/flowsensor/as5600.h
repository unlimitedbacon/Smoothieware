#ifndef AS5600_H
#define AS5600_H

#include "I2C.h"
#include "Pin.h"

#define MAGNET_WEAK     3
#define MAGNET_STRONG   2
#define MAGNET_MISSING  1
#define MAGNET_OK       0

class AS5600 {
    public:
        AS5600();
        ~AS5600();
        int setup();
        int get_angle(int* angle);
        int get_status(int* status);

    private:
        mbed::I2C* i2c;
        int addr = 0x36 << 1;
};

#endif