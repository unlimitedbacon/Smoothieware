#pragma once

#include "Module.h"

#include <string>

class About : public Module
{
    public:
        About();
        ~About();
        void on_module_loaded();
        void on_config_reload(void *argument);
        void on_get_public_data(void *argument);
    private:
        std::string machine_name;
        std::string make;
        std::string model;
};