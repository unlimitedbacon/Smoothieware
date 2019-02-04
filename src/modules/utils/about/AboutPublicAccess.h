#pragma once

#include <string>

#define about_checksum   CHECKSUM("about")

using pad_about_t = struct pad_about {
    std::string machine_name;
    std::string make;
    std::string model;
};