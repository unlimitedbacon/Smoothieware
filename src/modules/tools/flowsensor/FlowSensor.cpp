#include "FlowSensor.h"
#include "as5600.h"
#include "checksumm.h"
#include "Config.h"
#include "ConfigValue.h"
#include "Gcode.h"
#include "Kernel.h"
#include "SlowTicker.h"
#include "StreamOutput.h"
#include "StreamOutputPool.h"

#define PI 3.14159265358979323846F

#define flow_sensor_checksum    CHECKSUM("flow_sensor")
#define enable_checksum         CHECKSUM("enable")
#define diameter_checksum       CHECKSUM("gear_diameter")
#define direction_checksum      CHECKSUM("direction")

FlowSensor::FlowSensor()
{
    circumference = 0;
    direction = 1;
    sensor = NULL;
    origin = 0;
    rotation_count = 0;
    last_angle = 0;
    error = false;
}

FlowSensor::~FlowSensor()
{
    delete sensor;
}

void FlowSensor::on_module_loaded()
{
    // If the module is disabled, do nothing
    if (!THEKERNEL->config->value(flow_sensor_checksum, enable_checksum)->by_default(false)->as_bool()) {
        delete this;
        return;
    }
    // Read configuration
    this->on_config_reload(this);
    // Initialize sensor
    delete sensor;
    sensor = new AS5600();
    int result = this->sensor->setup();
    if (!result) {
        reset();    // Assumes initial E position is 0
    } else {
        error = true;
        print_io_error(result);
    }
    // Register hooks
    this->register_for_event(ON_GCODE_RECEIVED);
    // If filament is flowing 100 mm/s with a 10.7 mm dia gear,
    // we need to poll at least 12 hz (4x per rotation) to ensure
    // we do not skip a rotation.
    // So at 100 hz we can miss at most 8 checks.
    THEKERNEL->slow_ticker->attach( 100, this, &FlowSensor::update);
}

void FlowSensor::on_config_reload(void *argument)
{
    circumference = THEKERNEL->config->value(flow_sensor_checksum, diameter_checksum)->by_default(10.699F)->as_number() * PI;
    direction = THEKERNEL->config->value(flow_sensor_checksum, direction_checksum)->by_default(true)->as_bool() ? 1 : -1;
}

void FlowSensor::on_gcode_received(void *argument)
{
    Gcode* gcode = static_cast<Gcode *>(argument);
    if (gcode->has_m) {
        // Print measured extrusion
        if (gcode->m == 114) {
            char buf[32] = "E_meas:Err";
            int n = 10;
            if (!error) {
                float distance = direction * (rotation_count + (last_angle / 4096.0)) * circumference;
                n = snprintf(buf, sizeof(buf), "E_meas:%1.4f", distance);
            }
            gcode->txt_after_ok.append(buf, n);
        }

        // Print sensor status information
        else if (gcode->m == 407) {
            int angle = 0;
            int status = 0;
            int result;
            result = this->sensor->get_angle(&angle);
            result = this->sensor->get_status(&status);
            gcode->stream->printf("Flow Sensor - Comm: %s", (result ? "Err" : "Ok"));
            if (!result) {
                gcode->stream->printf(", Magnet: ");
                switch (status)
                {
                    case MAGNET_OK:
                        gcode->stream->printf("Good");
                        break;
                    case MAGNET_WEAK:
                        gcode->stream->printf("Weak");
                        break;
                    case MAGNET_STRONG:
                        gcode->stream->printf("Strong");
                        break;
                    default:
                        gcode->stream->printf("Missing");
                        break;
                }
                gcode->stream->printf(", Angle: %i\n", angle);
            } else {
                gcode->stream->printf("\n");
            }
            gcode->stream->printf("Origin: %i, Rotations: %i, Last Angle: %i\n", origin, rotation_count, last_angle);
        }

    } else if (gcode->has_g) {
        // Reset tracked position
        if (gcode->g == 92 && gcode->subcode == 0 && (gcode->has_letter('E') || gcode->get_num_args() == 0)) {
            float e = gcode->has_letter('E') ? gcode->get_value('E') : 0;
            reset(e);
        }
    }
}

uint32_t FlowSensor::update(uint32_t dummy) {
    if (!error) {
        int status;
        int angle;
        int result;
        result = this->sensor->get_status(&status);
        if (result) {
            //error = true;
            //print_io_error(result);
            return 0;
        }
        if (status == MAGNET_MISSING) {
            error = true;
            print_magnet_error();
            return 0;
        }
        result = this->sensor->get_angle(&angle);
        if (result) {
            //error = true;
            //print_io_error(result);
            return 0;
        }
        // Get angle relative to the origin
        angle = (angle - origin + 4095) % 4095;
        // Check if we have moved past the origin since last time
        if (angle < 1024 && last_angle > 3072) {
            rotation_count++;
        } else if (angle > 3072 && last_angle < 1024) {
            rotation_count--;
        }
        last_angle = angle;
    }

    return 0;
}

void FlowSensor::reset(float e) {
    // TODO: Handle E values other than 0
    rotation_count = 0;
    last_angle = 0;
    error = false;
    int status;
    int result;
    result = this->sensor->get_status(&status);
    if (result) {
        error = true;
        print_io_error(result);
        return;
    }
    if (status == MAGNET_MISSING) {
        error = true;
        print_magnet_error();
        return;
    }
    result = this->sensor->get_angle(&origin);
    if (result) {
        error = true;
        print_io_error(result);
        return;
    }
}

void FlowSensor::print_io_error(int code) {
    THEKERNEL->streams->printf("ERROR: Cannot communicate with flow sensor. Code %X\n", code);
}

void FlowSensor::print_magnet_error() {
    THEKERNEL->streams->printf("ERROR: Flow sensor magnet not detected");
}