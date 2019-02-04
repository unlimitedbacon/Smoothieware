#include "About.h"
#include "AboutPublicAccess.h"
#include "checksumm.h"
#include "Config.h"
#include "ConfigValue.h"
#include "PublicDataRequest.h"
#include "Kernel.h"

#define about_checksum           CHECKSUM("about")
#define machine_name_checksum   CHECKSUM("machine_name")
#define make_checksum           CHECKSUM("make")
#define model_checksum          CHECKSUM("model")

About::About()
{

}

About::~About()
{

}

void About::on_module_loaded()
{
    this->on_config_reload(this);
    this->register_for_event(ON_GET_PUBLIC_DATA);
}

void About::on_config_reload(void *argument)
{
    make = THEKERNEL->config->value(about_checksum, make_checksum)->as_string();
    model = THEKERNEL->config->value(about_checksum, model_checksum)->as_string();
    machine_name = THEKERNEL->config->value(about_checksum, machine_name_checksum)->as_string();
}

void About::on_get_public_data(void *argument)
{
    PublicDataRequest *pdr = static_cast<PublicDataRequest *>(argument);

    if (!pdr->starts_with(about_checksum)) return;

    // ok this is targeted at us, so send back the requested data
    // caller has provided the location to write the state to
    struct pad_about *pad = static_cast<struct pad_about *>(pdr->get_data_ptr());
    pad->machine_name = machine_name;
    pad->make = make;
    pad->model = model;
    pdr->set_taken();
}