#include <stdio.h>
#include <math.h>
#include "esp_log.h"

#include "vcore.h"
#include "adc.h"
#include "DS4432U.h"
#include "TPS546.h"

#define TPS40305_VFB 0.6

// DS4432U Transfer function constants for Bitaxe board
// #define BITAXE_RFS 80000.0     // R16
// #define BITAXE_IFS ((DS4432_VRFS * 127.0) / (BITAXE_RFS * 16))
#define BITAXE_IFS 0.000098921 // (Vrfs / Rfs) x (127/16)  -> Vrfs = 0.997, Rfs = 80000
#define BITAXE_RA 4750.0       // R14
#define BITAXE_RB 3320.0       // R15
#define BITAXE_VNOM 1.451   // this is with the current DAC set to 0. Should be pretty close to (VFB*(RA+RB))/RB
#define BITAXE_VMAX 2.39
#define BITAXE_VMIN 0.046

static const char *TAG = "vcore.c";

esp_err_t VCORE_init(GlobalState * global_state) {
    switch (global_state->device_model) {
        case DEVICE_MAX:
        case DEVICE_ULTRA:
        case DEVICE_SUPRA:
            if (global_state->board_version >= 402 && global_state->board_version <= 499) {
                if (TPS546_init() != ESP_OK) {
                    ESP_LOGE(TAG, "TPS546 init failed!");
                    return ESP_FAIL;
                }
            } else {
                ESP_RETURN_ON_ERROR(DS4432U_init(), TAG, "DS4432 init failed!");
            }
            break;
        case DEVICE_GAMMA:
        case DEVICE_LV07:
            if (TPS546_init() != ESP_OK) {
                ESP_LOGE(TAG, "TPS546 init failed!");
                return ESP_FAIL;
            }
            break;
        // case DEVICE_HEX:
        default:
    }
    return ESP_OK;
}

/**
 * @brief ds4432_tps40305_bitaxe_voltage_to_reg takes a voltage and returns a register setting for the DS4432U to get that voltage on the TPS40305
 * careful with this one!!
 */
static uint8_t ds4432_tps40305_bitaxe_voltage_to_reg(float vout)
{
    float change;
    uint8_t reg;

    // make sure the requested voltage is in within range of BITAXE_VMIN and BITAXE_VMAX
    if (vout > BITAXE_VMAX || vout < BITAXE_VMIN)
    {
        return 0;
    }

    // this is the transfer function. comes from the DS4432U+ datasheet
    change = fabs((((TPS40305_VFB / BITAXE_RB) - ((vout - TPS40305_VFB) / BITAXE_RA)) / BITAXE_IFS) * 127);
    reg = (uint8_t)ceil(change);

    // Set the MSB high if the requested voltage is BELOW nominal
    if (vout < BITAXE_VNOM)
    {
        reg |= 0x80;
    }

    return reg;
}

esp_err_t VCORE_set_voltage(float core_voltage, GlobalState * global_state)
{
    switch (global_state->device_model) {
        case DEVICE_MAX:
        case DEVICE_ULTRA:
        case DEVICE_SUPRA:
            if (global_state->board_version >= 402 && global_state->board_version <= 499) {
                ESP_LOGI(TAG, "Set ASIC voltage = %.3fV", core_voltage);
                TPS546_set_vout(core_voltage * (float)global_state->voltage_domain);
            } else {
                uint8_t reg_setting = ds4432_tps40305_bitaxe_voltage_to_reg(core_voltage * (float)global_state->voltage_domain);
                ESP_LOGI(TAG, "Set ASIC voltage = %.3fV [0x%02X]", core_voltage, reg_setting);
                ESP_RETURN_ON_ERROR(DS4432U_set_current_code(0, reg_setting), TAG, "DS4432U set current code failed!");
            }
            break;
        case DEVICE_GAMMA:
        case DEVICE_LV07:
                ESP_LOGI(TAG, "Set ASIC voltage = %.3fV", core_voltage);
                TPS546_set_vout(core_voltage * (float)global_state->voltage_domain);
            break;
        // case DEVICE_HEX:
        default:
    }

    return ESP_OK;
}

uint16_t VCORE_get_voltage_mv(GlobalState * global_state) {
        
    switch (global_state->device_model) {
        case DEVICE_LV07:
            return (TPS546_get_vout() * 1000) / global_state->voltage_domain;
            break;
        default:
    }

    return ADC_get_vcore() / global_state->voltage_domain;
}
