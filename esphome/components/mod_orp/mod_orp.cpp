#include "mod_orp.h"
#include "esphome/core/log.h"

namespace esphome
{
    namespace mod_orp
    {

        static const char *const TAG = "mod_orp.sensor";

        void Mod_ORPSensor::setup()
        {
            ESP_LOGCONFIG(TAG, "Setting up Mod-ORP...");

            uint8_t hwVersion;
            uint8_t swVersion;
            if (!this->read_byte(HW_VERSION_REGISTER, &hwVersion) && hwVersion != 0xFF)
            {
                ESP_LOGE(TAG, "Unable to read hardware version");
                this->mark_failed();
                return;
            }
            if (!this->read_byte(SW_VERSION_REGISTER, &swVersion))
            {
                ESP_LOGE(TAG, "Unable to read software version");
                this->mark_failed();
                return;
            }
            ESP_LOGI(TAG, "Found Mod-ORP HW-Version:%d SW-Version:%d", hwVersion, swVersion);
        }

        void Mod_ORPSensor::dump_config()
        {
            LOG_I2C_DEVICE(this);
            if (this->is_failed()) {
                ESP_LOGE(TAG, "Communication with Mod-ORP failed!");
            } else {
                ESP_LOGI(TAG, "Single point %f", _read_4_bytes(CALIBRATE_SINGLE_OFFSET_REGISTER));
            }
            LOG_UPDATE_INTERVAL(this);
            LOG_SENSOR("  ", "ORP", this);
        }

        void Mod_ORPSensor::update()
        {
            int wait = 750;
            
            _send_command(MEASURE_ORP_TASK);
            this->set_timeout("data", wait, [this]()
                              { this->update_internal_(); });
        }

        void Mod_ORPSensor::update_internal_()
        {
            uint8_t status;
            if(!this->read_byte(STATUS_REGISTER, &status))
            {
                ESP_LOGE(TAG, "STATUS_REGISTER read failed");
            }
            if (status == STATUS_NO_ERROR)
            {
                float ms = _read_4_bytes(MV_REGISTER);
                ESP_LOGD(TAG, "'%s': Got ORP=%.2f mV", this->get_name().c_str(), ms);
                this->publish_state(ms);
            }
            else
            {
                switch (status)
                {
                case STATUS_SYSTEM_ERROR:
                    ESP_LOGE(TAG, "Error: module not functioning properly");
                    break;
                }
            }
        }

        void Mod_ORPSensor::calibrateSingle(float calibration_mv)
        {
            ESP_LOGD(TAG, "Calibrate Single Pressed. %f", calibration_mv);
            _write_4_bytes(MV_REGISTER, calibration_mv);

            _send_command(CALIBRATE_SINGLE_TASK);
        }

        void Mod_ORPSensor::calibrateReset()
        {
            ESP_LOGD(TAG, "Calibrate Reset Pressed.");
            _write_4_bytes(CALIBRATE_SINGLE_OFFSET_REGISTER, NAN);
        }

        float Mod_ORPSensor::get_setup_priority() const { return setup_priority::DATA; }

        void Mod_ORPSensor::_write_4_bytes(uint8_t reg, float f)
        {
            uint8_t b[4];
            float f_val = f;

            b[0] = *((uint8_t *)&f_val);
            b[1] = *((uint8_t *)&f_val + 1);
            b[2] = *((uint8_t *)&f_val + 2);
            b[3] = *((uint8_t *)&f_val + 3);
            this->write_register(reg, b, 4);
        }

        void Mod_ORPSensor::_send_command(uint8_t command)
        {
            this->write_byte(TASK_REGISTER, command);
        }

        float Mod_ORPSensor::_read_4_bytes(uint8_t reg)
        {
            float f;
            uint8_t temp[4];

            this->write(&reg, 1);
            this->read(temp, 4);
            memcpy(&f, temp, sizeof(f));

            return f;
        }
    } // namespace mod_orp
} // namespace esphome
