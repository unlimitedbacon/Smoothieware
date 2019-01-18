#pragma once

#include "Module.h"

#include <stdint.h>

class AS5600;

class FlowSensor : public Module
{
    public:
        FlowSensor();
        ~FlowSensor();
        void on_module_loaded();
        void on_config_reload(void *argument);
        void on_gcode_received(void *argument);
        uint32_t update(uint32_t dummy);

    private:
        void reset(float e = 0.0);
        void print_io_error(int code);
        void print_magnet_error();

        AS5600* sensor;
        float circumference;
        int direction;
        int origin;
        int rotation_count;
        int last_angle;
        bool error;
};