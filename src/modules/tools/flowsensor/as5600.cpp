#include "as5600.h"
#include "I2C.h"

// The Mbed i2c library is blocking, which is a problem for Smoothie.
// Possible non-blocking alternatives:
// * https://os.mbed.com/users/Sissors/code/MODI2C/
// * https://www.nxp.com/docs/en/application-note/AN4803.pdf


AS5600::AS5600()
{
    // Pins require external pull up resistors
    this->i2c = new mbed::I2C(P0_27, P0_28);    // Verified with multimeter
    //this->i2c->frequency(100000);               // Standard mode
    this->i2c->frequency(400000);               // Fast mode
}

AS5600::~AS5600()
{
    delete this->i2c;
}

/**
 * Sets the sensor configuration
 * 
 * @return 0 if successful. I2C status code if failed.
*/
int AS5600::setup(){
    // Set the device configuration
    #define FTH 0b001                 // Fast Filter Threshold
    #define SF  0b00                  // Slow Filter
    char data[2];
    data[0] = 0x07;                     // CONF Register
    data[1] = (FTH<<2) | (SF<<0);       // Value
    return this->i2c->write( addr, data, 2 );
}

/**
 * Reads the angle from the sensor
 * 
 * @param[out]  angle Angle is written to this variable. Values from 0 to 4095.
 * @return      0 if successful. I2C status code if failed.
 */
int AS5600::get_angle( int* angle )
{
    // Read two bytes from the angle registers
    //char reg = 0x0E;   // Processed angle
    char reg = 0x0C;     // Raw angle
    char data[2];
    int result = 0;
    result = this->i2c->write( addr, &reg, 1, true);
    if (result)
        return result;
    result = this->i2c->read( addr, data, 2);
    if (result)
        return result;
    // Convert to an int
    *angle = ((data[0] & 0x0F) << 8) | data[1];
    return result;
}

/**
 * Reads the status of the magnet.
 * 
 * @param[out]  status Status code is written to this variable.
 * @return      0 if successful. I2C status code if failed.
 */
int AS5600::get_status( int* status )
{
    // Read one byte from the status register
    char reg = 0x0B;
    char data = 0;
    int result = 0;
    result = this->i2c->write( addr, &reg, 1, true );
    if (result)
        return result;
    result = this->i2c->read( addr, &data, 1 );
    if (result)
        return result;
    bool MD = (data & 0b100000) >> 5;
    bool ML = (data & 0b010000) >> 4;
    bool MH = (data & 0b001000) >> 3;
    if (!MD) {
        *status = MAGNET_MISSING;
    } else if (ML) {
        *status = MAGNET_WEAK;
    } else if (MH) {
        *status = MAGNET_STRONG;
    } else {
        *status = MAGNET_OK;
    }
    return result;
}