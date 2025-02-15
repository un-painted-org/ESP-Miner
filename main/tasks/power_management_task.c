#include <string.h>
#include "INA260.h"
#include "bm1397.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "global_state.h"
#include "math.h"
#include "mining.h"
#include "nvs_config.h"
#include "serial.h"
#include "TMP1075.h"
#include "TPS546.h"
#include "vcore.h"
#include "thermal.h"
#include "power.h"

#define POLL_RATE 2000
#define MAX_TEMP 90.0
#define THROTTLE_TEMP 75.0
#define THROTTLE_TEMP_RANGE (MAX_TEMP - THROTTLE_TEMP)

#define VOLTAGE_START_THROTTLE 4900
#define VOLTAGE_MIN_THROTTLE 3500
#define VOLTAGE_RANGE (VOLTAGE_START_THROTTLE - VOLTAGE_MIN_THROTTLE)

#define TPS546_THROTTLE_TEMP 105.0
#define TPS546_MAX_TEMP 145.0

#define SUPRA_POWER_OFFSET 5
#define GAMMA_POWER_OFFSET 5
#define LV07_POWER_OFFSET 6

static const char * TAG = "power_management";

// static float _fbound(float value, float lower_bound, float upper_bound)
// {
//     if (value < lower_bound)
//         return lower_bound;
//     if (value > upper_bound)
//         return upper_bound;

//     return value;
// }

// Set the fan speed between 20% min and 100% max based on chip temperature as input.
// The fan speed increases from 20% to 100% proportionally to the temperature increase from 50 and THROTTLE_TEMP
static double automatic_fan_speed(float chip_temp, GlobalState * GLOBAL_STATE)
{
    double result = 0.0;
    double min_temp = 45.0;
    double min_fan_speed = 20.0;

    if (chip_temp < min_temp) {
        result = min_fan_speed;
    } else if (chip_temp >= THROTTLE_TEMP) {
        result = 100;
    } else {
        double temp_range = THROTTLE_TEMP - min_temp;
        double fan_range = 100 - min_fan_speed;
        result = ((chip_temp - min_temp) / temp_range) * fan_range + min_fan_speed;
    }

    float perc = (float) result / 100;
    GLOBAL_STATE->POWER_MANAGEMENT_MODULE.fan_perc = perc;

    switch (GLOBAL_STATE->device_model) {
        case DEVICE_MAX:
        case DEVICE_ULTRA:
        case DEVICE_SUPRA:
        case DEVICE_GAMMA:
            EMC2101_set_fan_speed( perc );
            break;
        case DEVICE_LV07:
            EMC2302_set_fan_speed(0,perc);
            EMC2302_set_fan_speed(1,perc);
        default:
    }
    PowerManagementModule * power_management = &GLOBAL_STATE->POWER_MANAGEMENT_MODULE;
    power_management->fan_perc = result;
    Thermal_set_fan_percent(GLOBAL_STATE->device_model, result/100.0);

	return result;
}

void POWER_MANAGEMENT_task(void * pvParameters)
{
    ESP_LOGI(TAG, "Starting");

    GlobalState * GLOBAL_STATE = (GlobalState *) pvParameters;

    PowerManagementModule * power_management = &GLOBAL_STATE->POWER_MANAGEMENT_MODULE;

    power_management->frequency_multiplier = 1;

    //int last_frequency_increase = 0;
    //uint16_t frequency_target = nvs_config_get_u16(NVS_CONFIG_ASIC_FREQ, CONFIG_ASIC_FREQUENCY);

    switch (GLOBAL_STATE->device_model) {
        case DEVICE_MAX:
        case DEVICE_ULTRA:
        case DEVICE_SUPRA:
			if (GLOBAL_STATE->board_version < 402 || GLOBAL_STATE->board_version > 499) {
                // Configure plug sense pin as input(barrel jack) 1 is plugged in
                gpio_config_t barrel_jack_conf = {
                    .pin_bit_mask = (1ULL << GPIO_PLUG_SENSE),
                    .mode = GPIO_MODE_INPUT,
                };
                gpio_config(&barrel_jack_conf);
                int barrel_jack_plugged_in = gpio_get_level(GPIO_PLUG_SENSE);

                gpio_set_direction(GPIO_ASIC_ENABLE, GPIO_MODE_OUTPUT);
                if (barrel_jack_plugged_in == 1 || !power_management->HAS_PLUG_SENSE) {
                    // turn ASIC on
                    gpio_set_level(GPIO_ASIC_ENABLE, 0);
                } else {
                    // turn ASIC off
                    gpio_set_level(GPIO_ASIC_ENABLE, 1);
                }
			}
            break;
        case DEVICE_GAMMA:
            break;
        case DEVICE_LV07:
            TMP1075_init();
            EMC2302_init(true);
            break;
        default:
    }

    vTaskDelay(500 / portTICK_PERIOD_MS);
    uint16_t last_core_voltage = 0.0;
    uint16_t last_asic_frequency = power_management->frequency_value;
    
    while (1) {

        switch (GLOBAL_STATE->device_model) {
            case DEVICE_MAX:
            case DEVICE_ULTRA:
            case DEVICE_SUPRA:
				if (GLOBAL_STATE->board_version >= 402 && GLOBAL_STATE->board_version <= 499) {
                    power_management->voltage = TPS546_get_vin() * 1000;
                    power_management->current = TPS546_get_iout() * 1000;
                    // calculate regulator power (in milliwatts)
                    power_management->power = (TPS546_get_vout() * power_management->current) / 1000;
                    // The power reading from the TPS546 is only it's output power. So the rest of the Bitaxe power is not accounted for.
                    power_management->power += SUPRA_POWER_OFFSET; // Add offset for the rest of the Bitaxe power. TODO: this better.
				} else {
                    if (INA260_installed() == true) {
                        power_management->voltage = INA260_read_voltage();
                        power_management->current = INA260_read_current();
                        power_management->power = INA260_read_power() / 1000;
                    }
				}
                power_management->fan_rpm = EMC2101_get_fan_speed();
                break;
            case DEVICE_GAMMA:
                power_management->voltage = TPS546_get_vin() * 1000;
                power_management->current = TPS546_get_iout() * 1000;
                // calculate regulator power (in milliwatts)
                power_management->power = (TPS546_get_vout() * power_management->current) / 1000;
                // The power reading from the TPS546 is only it's output power. So the rest of the Bitaxe power is not accounted for.
                power_management->power += GAMMA_POWER_OFFSET; // Add offset for the rest of the Bitaxe power. TODO: this better.

                power_management->fan_rpm = EMC2101_get_fan_speed();
                break;
            case DEVICE_LV07:
                //TPS546_print_status();
                power_management->voltage = TPS546_get_vin() * 1000;
                power_management->current = TPS546_get_iout() * 1000;
                power_management->power = (TPS546_get_vout() * power_management->current) / 1000 + LV07_POWER_OFFSET;

                power_management->fan_rpm = EMC2302_get_fan_speed(1);
                break;
            default:
        }

        switch (GLOBAL_STATE->device_model) {
            case DEVICE_MAX:
                power_management->chip_temp_avg = GLOBAL_STATE->ASIC_initalized ? EMC2101_get_external_temp() : -1;

                if ((power_management->chip_temp_avg > THROTTLE_TEMP) &&
                    (power_management->frequency_value > 50 || power_management->voltage > 1000)) {
                    ESP_LOGE(TAG, "OVERHEAT ASIC %fC", power_management->chip_temp_avg );

                    EMC2101_set_fan_speed(1);
                    if (power_management->HAS_POWER_EN) {
                        gpio_set_level(GPIO_ASIC_ENABLE, 1);
                    }
                    nvs_config_set_u16(NVS_CONFIG_ASIC_VOLTAGE, 1000);
                    nvs_config_set_u16(NVS_CONFIG_ASIC_FREQ, 50);
                    nvs_config_set_u16(NVS_CONFIG_FAN_SPEED, 100);
                    nvs_config_set_u16(NVS_CONFIG_AUTO_FAN_SPEED, 0);
                    nvs_config_set_u16(NVS_CONFIG_OVERHEAT_MODE, 1);
                    exit(EXIT_FAILURE);
                }
                break;
            case DEVICE_ULTRA:
            case DEVICE_SUPRA:
                
                if (GLOBAL_STATE->board_version >= 402 && GLOBAL_STATE->board_version <= 499) {
                    power_management->chip_temp_avg = GLOBAL_STATE->ASIC_initalized ? EMC2101_get_external_temp() : -1;
                    power_management->vr_temp = (float)TPS546_get_temperature();
                } else {
                    power_management->chip_temp_avg = EMC2101_get_internal_temp() + 5;
                    power_management->vr_temp = 0.0;
                }

                // EMC2101 will give bad readings if the ASIC is turned off
                if(power_management->voltage < TPS546_INIT_VOUT_MIN){
                    break;
                }
        power_management->voltage = Power_get_input_voltage(GLOBAL_STATE);
        power_management->power = Power_get_power(GLOBAL_STATE);

        power_management->fan_rpm = Thermal_get_fan_speed(GLOBAL_STATE->device_model);
        power_management->chip_temp_avg = Thermal_get_chip_temp(GLOBAL_STATE);

        power_management->vr_temp = Power_get_vreg_temp(GLOBAL_STATE);


        // ASIC Thermal Diode will give bad readings if the ASIC is turned off
        // if(power_management->voltage < tps546_config.TPS546_INIT_VOUT_MIN){
        //     goto looper;
        // }

        //overheat mode if the voltage regulator or ASIC is too hot
        if ((power_management->vr_temp > TPS546_THROTTLE_TEMP || power_management->chip_temp_avg > THROTTLE_TEMP) && (power_management->frequency_value > 50 || power_management->voltage > 1000)) {
            ESP_LOGE(TAG, "OVERHEAT! VR: %fC ASIC %fC", power_management->vr_temp, power_management->chip_temp_avg );
            power_management->fan_perc = 100;
            Thermal_set_fan_percent(GLOBAL_STATE->device_model, 1);

            // Turn off core voltage
            Power_disable(GLOBAL_STATE);

                    nvs_config_set_u16(NVS_CONFIG_ASIC_VOLTAGE, 1000);
                    nvs_config_set_u16(NVS_CONFIG_ASIC_FREQ, 50);
                    nvs_config_set_u16(NVS_CONFIG_FAN_SPEED, 100);
                    nvs_config_set_u16(NVS_CONFIG_AUTO_FAN_SPEED, 0);
                    nvs_config_set_u16(NVS_CONFIG_OVERHEAT_MODE, 1);
                    exit(EXIT_FAILURE);
                }
                break;
            case DEVICE_LV07:
                    //uint8_t temps[6] = {0,0,0,0,0,0};
                    //max6689_read_temp(temps);
                    //ESP_LOGI(TAG,"Diode Temp: [%d,%d,%d,%d,%d,%d]",temps[0],temps[1],temps[2],temps[3],temps[4],temps[5]);

                    power_management->chip_temp_avg = (TMP1075_read_temperature(0)+TMP1075_read_temperature(1))/2+5;
					power_management->vr_temp = (float)TPS546_get_temperature();

                    // EMC2302 will give bad readings if the ASIC is turned off
                    if(power_management->voltage < TPS546_INIT_VOUT_MIN){
                        break;
                    }
                    // Need to fix for SUPRAHEX which read the actual ASIC temp.
                    if ((power_management->vr_temp > TPS546_MAX_TEMP || power_management->chip_temp_avg > MAX_TEMP) &&
                        (power_management->frequency_value > 50 || power_management->voltage > 1000)) {
                        ESP_LOGE(TAG, "OVERHEAT  VR: %fC ASIC %fC", power_management->vr_temp, power_management->chip_temp_avg );

                        EMC2302_set_fan_speed(0,1);
                        EMC2302_set_fan_speed(1,1);
                        VCORE_set_voltage(0.0, GLOBAL_STATE);
                        nvs_config_set_u16(NVS_CONFIG_ASIC_VOLTAGE, 1000);
                        nvs_config_set_u16(NVS_CONFIG_ASIC_FREQ, 50);
                        nvs_config_set_u16(NVS_CONFIG_FAN_SPEED, 100);
                        nvs_config_set_u16(NVS_CONFIG_AUTO_FAN_SPEED, 0);
                        nvs_config_set_u16(NVS_CONFIG_OVERHEAT_MODE, 1);
                        exit(EXIT_FAILURE);
					}
                    break;
            default:
        }

            nvs_config_set_u16(NVS_CONFIG_ASIC_VOLTAGE, 1000);
            nvs_config_set_u16(NVS_CONFIG_ASIC_FREQ, 50);
            nvs_config_set_u16(NVS_CONFIG_FAN_SPEED, 100);
            nvs_config_set_u16(NVS_CONFIG_AUTO_FAN_SPEED, 0);
            nvs_config_set_u16(NVS_CONFIG_OVERHEAT_MODE, 1);
            exit(EXIT_FAILURE);
        }

        if (nvs_config_get_u16(NVS_CONFIG_AUTO_FAN_SPEED, 1) == 1) {

            power_management->fan_perc = (float)automatic_fan_speed(power_management->chip_temp_avg, GLOBAL_STATE);

        } else {
            switch (GLOBAL_STATE->device_model) {
                case DEVICE_MAX:
                case DEVICE_ULTRA:
                case DEVICE_SUPRA:
                case DEVICE_GAMMA:

                    float fs = (float) nvs_config_get_u16(NVS_CONFIG_FAN_SPEED, 100);
                    power_management->fan_perc = fs;
                    EMC2101_set_fan_speed((float) fs / 100);

                    break;
                case DEVICE_LV07:
                    fs = (float) nvs_config_get_u16(NVS_CONFIG_FAN_SPEED, 100);
                    //ESP_LOGI(TAG, "Manual Fan = %.02f", fs);
                    power_management->fan_perc = fs;
                    EMC2302_set_fan_speed(0,(float) fs / 100);
                    EMC2302_set_fan_speed(1,(float) fs / 100);
                    break;
                default:
            }
            float fs = (float) nvs_config_get_u16(NVS_CONFIG_FAN_SPEED, 100);
            power_management->fan_perc = fs;
            Thermal_set_fan_percent(GLOBAL_STATE->device_model, (float) fs / 100.0);
        }

        // Read the state of plug sense pin
        // if (power_management->HAS_PLUG_SENSE) {
        //     int gpio_plug_sense_state = gpio_get_level(GPIO_PLUG_SENSE);
        //     if (gpio_plug_sense_state == 0) {
        //         // turn ASIC off
        //         gpio_set_level(GPIO_ASIC_ENABLE, 1);
        //     }
        // }

        // New voltage and frequency adjustment code
        uint16_t core_voltage = nvs_config_get_u16(NVS_CONFIG_ASIC_VOLTAGE, CONFIG_ASIC_VOLTAGE);
        uint16_t asic_frequency = nvs_config_get_u16(NVS_CONFIG_ASIC_FREQ, CONFIG_ASIC_FREQUENCY);

        if (core_voltage != last_core_voltage) {
            ESP_LOGI(TAG, "setting new vcore voltage to %umV", core_voltage);
            VCORE_set_voltage((double) core_voltage / 1000.0, GLOBAL_STATE);
            last_core_voltage = core_voltage;
        }

        if (asic_frequency != last_asic_frequency) {
            ESP_LOGI(TAG, "New ASIC frequency requested: %uMHz (current: %uMHz)", asic_frequency, last_asic_frequency);
            if (do_frequency_transition((float)asic_frequency)) {
                power_management->frequency_value = (float)asic_frequency;
                ESP_LOGI(TAG, "Successfully transitioned to new ASIC frequency: %uMHz", asic_frequency);
            } else {
                ESP_LOGE(TAG, "Failed to transition to new ASIC frequency: %uMHz", asic_frequency);
            }
            last_asic_frequency = asic_frequency;
        }

        // Check for changing of overheat mode
        SystemModule * module = &GLOBAL_STATE->SYSTEM_MODULE;
        uint16_t new_overheat_mode = nvs_config_get_u16(NVS_CONFIG_OVERHEAT_MODE, 0);
        
        if (new_overheat_mode != module->overheat_mode) {
            module->overheat_mode = new_overheat_mode;
            ESP_LOGI(TAG, "Overheat mode updated to: %d", module->overheat_mode);
        }

        // looper:
        vTaskDelay(POLL_RATE / portTICK_PERIOD_MS);
    }
}
