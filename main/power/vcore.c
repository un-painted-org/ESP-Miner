#include <stdio.h>
#include <math.h>
#include "esp_log.h"
#include <string.h>

#include "vcore.h"
#include "adc.h"
#include "DS4432U.h"
#include "TPS546.h"
#include "INA260.h"
#include "driver/gpio.h"

#define GPIO_ASIC_ENABLE CONFIG_GPIO_ASIC_ENABLE
#define GPIO_ASIC_RESET  CONFIG_GPIO_ASIC_RESET
#define GPIO_PLUG_SENSE  CONFIG_GPIO_PLUG_SENSE

static TPS546_CONFIG TPS546_CONFIG_GAMMATURBO = {
    /* vin voltage */
    .TPS546_INIT_VIN_ON = 11.0,
    .TPS546_INIT_VIN_OFF = 10.5,
    .TPS546_INIT_VIN_UV_WARN_LIMIT = 11.0,
    .TPS546_INIT_VIN_OV_FAULT_LIMIT = 14.0,
    /* vout voltage */
    .TPS546_INIT_SCALE_LOOP = 0.25,
    .TPS546_INIT_VOUT_MIN = 1,
    .TPS546_INIT_VOUT_MAX = 3,
    .TPS546_INIT_VOUT_COMMAND = 1.2,
    /* iout current */
    .TPS546_INIT_IOUT_OC_WARN_LIMIT = 50.00, /* A */
    .TPS546_INIT_IOUT_OC_FAULT_LIMIT = 55.00 /* A */
};

static TPS546_CONFIG TPS546_CONFIG_GAMMA = {
    /* vin voltage */
    .TPS546_INIT_VIN_ON = 4.8,
    .TPS546_INIT_VIN_OFF = 4.5,
    .TPS546_INIT_VIN_UV_WARN_LIMIT = 0, //Set to 0 to ignore. TI Bug in this register
    .TPS546_INIT_VIN_OV_FAULT_LIMIT = 6.5,
    /* vout voltage */
    .TPS546_INIT_SCALE_LOOP = 0.25,
    .TPS546_INIT_VOUT_MIN = 1,
    .TPS546_INIT_VOUT_MAX = 2,
    .TPS546_INIT_VOUT_COMMAND = 1.2,
    /* iout current */
    .TPS546_INIT_IOUT_OC_WARN_LIMIT = 25.00, /* A */
    .TPS546_INIT_IOUT_OC_FAULT_LIMIT = 30.00 /* A */
};

static TPS546_CONFIG TPS546_CONFIG_LV07 = {
    /* vin voltage */
    .TPS546_INIT_VIN_ON = 11.5,
    .TPS546_INIT_VIN_OFF = 11.0,
    .TPS546_INIT_VIN_UV_WARN_LIMIT = 11.0,
    .TPS546_INIT_VIN_OV_FAULT_LIMIT = 14.0,
    /* vout voltage */
    .TPS546_INIT_SCALE_LOOP = 0.25,
    .TPS546_INIT_VOUT_MIN = 1,
    .TPS546_INIT_VOUT_MAX = 3,
    .TPS546_INIT_VOUT_COMMAND = 1.2,
    /* iout current */
    .TPS546_INIT_IOUT_OC_WARN_LIMIT = 25.00, /* A */
    .TPS546_INIT_IOUT_OC_FAULT_LIMIT = 30.00 /* A */
};

static const char *TAG = "vcore.c";

esp_err_t VCORE_init(GlobalState * GLOBAL_STATE) {
    if (GLOBAL_STATE->DEVICE_CONFIG.DS4432U) {
        ESP_RETURN_ON_ERROR(DS4432U_init(), TAG, "DS4432 init failed!");
    }
    if (GLOBAL_STATE->DEVICE_CONFIG.INA260) {
        ESP_RETURN_ON_ERROR(INA260_init(), TAG, "INA260 init failed!");
    }
    if (GLOBAL_STATE->DEVICE_CONFIG.TPS546) {
        switch (GLOBAL_STATE->DEVICE_CONFIG.family.asic_count) {
            case 1:
                ESP_RETURN_ON_ERROR(TPS546_init(TPS546_CONFIG_GAMMA), TAG, "TPS546 init failed!");
                break;
            case 2:
                if (strcmp(GLOBAL_STATE->DEVICE_CONFIG.family.name, "LV07") == 0) {
                    ESP_RETURN_ON_ERROR(TPS546_init(TPS546_CONFIG_LV07), TAG, "TPS546 init failed!");
                } else {
                    ESP_RETURN_ON_ERROR(TPS546_init(TPS546_CONFIG_GAMMATURBO), TAG, "TPS546 init failed!");
                }
                break;
        }
    }

    if (GLOBAL_STATE->DEVICE_CONFIG.plug_sense) {
        gpio_config_t barrel_jack_conf = {
            .pin_bit_mask = (1ULL << GPIO_PLUG_SENSE),
            .mode = GPIO_MODE_INPUT,
        };
        gpio_config(&barrel_jack_conf);
        int barrel_jack_plugged_in = gpio_get_level(GPIO_PLUG_SENSE);

        gpio_set_direction(GPIO_ASIC_ENABLE, GPIO_MODE_OUTPUT);
        if (barrel_jack_plugged_in == 1 || GLOBAL_STATE->DEVICE_CONFIG.asic_enable) {
            gpio_set_level(GPIO_ASIC_ENABLE, 0);
        } else {
            // turn ASIC off
            gpio_set_level(GPIO_ASIC_ENABLE, 1);
        }
    }

    return ESP_OK;
}

esp_err_t VCORE_set_voltage(float core_voltage, GlobalState * GLOBAL_STATE)
{
    ESP_LOGI(TAG, "Set ASIC voltage = %.3fV", core_voltage);
 
    if (GLOBAL_STATE->DEVICE_CONFIG.DS4432U) {
        if (core_voltage != 0.0f) {
            ESP_RETURN_ON_ERROR(DS4432U_set_voltage(core_voltage), TAG, "DS4432U set voltage failed!");
        }
    }
    if (GLOBAL_STATE->DEVICE_CONFIG.TPS546) {
        ESP_RETURN_ON_ERROR(TPS546_set_vout(core_voltage), TAG, "TPS546 set voltage failed!");
    }
    if (core_voltage == 0.0f && GLOBAL_STATE->DEVICE_CONFIG.asic_enable) {
        gpio_set_level(GPIO_ASIC_ENABLE, 1);
    }

    return ESP_OK;
}

int16_t VCORE_get_voltage_mv(GlobalState * GLOBAL_STATE) 
{
    // TODO: What about hex?
    return ADC_get_vcore();
}

esp_err_t VCORE_check_fault(GlobalState * GLOBAL_STATE) 
{
    if (GLOBAL_STATE->DEVICE_CONFIG.TPS546) {
        ESP_RETURN_ON_ERROR(TPS546_check_status(GLOBAL_STATE), TAG, "TPS546 check status failed!");
    }
    return ESP_OK;
}

const char* VCORE_get_fault_string(GlobalState * GLOBAL_STATE) {
    if (GLOBAL_STATE->DEVICE_CONFIG.TPS546) {
        return TPS546_get_error_message();
    }
    return NULL;
}

