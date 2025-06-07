#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "global_state.h"
#include "screen.h"
#include "nvs_config.h"
#include "display.h"
#include "connect.h"
#include "esp_timer.h"
#include "esp_ota_ops.h"

typedef enum {
    SCR_SELF_TEST,
    SCR_OVERHEAT,
    SCR_ASIC_STATUS,
    SCR_CONFIGURE,
    SCR_FIRMWARE_UPDATE,
    SCR_CONNECTION,
    SCR_BITAXE_LOGO,
    SCR_OSMU_LOGO,
    SCR_URLS,
    SCR_STATS,
    SCR_WIFI_RSSI,
    MAX_SCREENS,
} screen_t;

#define SCREEN_UPDATE_MS 500

#define SCR_CAROUSEL_START SCR_URLS
#define SCR_CAROUSEL_END SCR_WIFI_RSSI

extern const lv_img_dsc_t bitaxe_logo;
extern const lv_img_dsc_t osmu_logo;

static lv_obj_t * screens[MAX_SCREENS];
static int delays_ms[MAX_SCREENS] = {0, 0, 0, 0, 0, 1000, 2000, 4000, 4000, 10000, 4000};

static screen_t current_screen = -1;
static int current_screen_time_ms;
static int current_screen_delay_ms;

static GlobalState * GLOBAL_STATE;

static lv_obj_t *asic_status_label;

static lv_obj_t *hashrate_label;
static lv_obj_t *efficiency_label;
static lv_obj_t *difficulty_label;
static lv_obj_t *chip_temp_label;

static lv_obj_t *firmware_update_scr_filename_label;
static lv_obj_t *firmware_update_scr_status_label;
static lv_obj_t *ip_addr_scr_overheat_label;
static lv_obj_t *ip_addr_scr_urls_label;
static lv_obj_t *mining_url_scr_urls_label;
static lv_obj_t *wifi_status_label;

static lv_obj_t *self_test_message_label;
static lv_obj_t *self_test_result_label;
static lv_obj_t *self_test_finished_label;

static lv_obj_t *wifi_rssi_value_label;
static lv_obj_t *esp_uptime_label;

static double current_hashrate;
static float current_power;
static uint64_t current_difficulty;
static float current_chip_temp;
static bool found_block;
static bool self_test_finished;

static lv_obj_t * create_scr_self_test() {
    lv_obj_t * scr = lv_obj_create(NULL);

    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *label1 = lv_label_create(scr);
    lv_label_set_text(label1, "BITAXE SELF TEST");

    self_test_message_label = lv_label_create(scr);
    self_test_result_label = lv_label_create(scr);

    self_test_finished_label = lv_label_create(scr);
    lv_obj_set_width(self_test_finished_label, LV_HOR_RES);
    lv_label_set_long_mode(self_test_finished_label, LV_LABEL_LONG_SCROLL_CIRCULAR);

    return scr;
}

static lv_obj_t * create_scr_overheat(SystemModule * module) {
    lv_obj_t * scr = lv_obj_create(NULL);

    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *label1 = lv_label_create(scr);
    lv_label_set_text(label1, "DEVICE OVERHEAT!");

    lv_obj_t *label2 = lv_label_create(scr);
    lv_obj_set_width(label2, LV_HOR_RES);
    lv_label_set_long_mode(label2, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(label2, "Power, frequency and fan configurations have been reset. Go to AxeOS to reconfigure device.");

    lv_obj_t *label3 = lv_label_create(scr);
    lv_label_set_text(label3, "IP Address:");

    ip_addr_scr_overheat_label = lv_label_create(scr);

    return scr;
}

static lv_obj_t * create_scr_asic_status(SystemModule * module) {
    lv_obj_t * scr = lv_obj_create(NULL);

    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *label1 = lv_label_create(scr);
    lv_label_set_text(label1, "ASIC STATUS:");

    asic_status_label = lv_label_create(scr);
    lv_label_set_long_mode(asic_status_label, LV_LABEL_LONG_SCROLL_CIRCULAR);

    return scr;
}

static lv_obj_t * create_scr_configure(SystemModule * module) {
    lv_obj_t * scr = lv_obj_create(NULL);

    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *label1 = lv_label_create(scr);
    lv_obj_set_width(label1, LV_HOR_RES);
    lv_obj_set_style_anim_duration(label1, 15000, LV_PART_MAIN);
    lv_label_set_long_mode(label1, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(label1, "Welcome to your new Bitaxe! Connect to the configuration Wi-Fi and connect the Bitaxe to your network.");

    // skip a line, it looks nicer this way
    lv_label_create(scr);

    lv_obj_t *label2 = lv_label_create(scr);
    lv_label_set_text(label2, "Wi-Fi (for setup):");

    lv_obj_t *label3 = lv_label_create(scr);
    lv_label_set_text(label3, module->ap_ssid);

    return scr;
}

static lv_obj_t * create_scr_ota(SystemModule * module) {
    lv_obj_t * scr = lv_obj_create(NULL);

    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *label1 = lv_label_create(scr);
    lv_obj_set_width(label1, LV_HOR_RES);
    lv_label_set_text(label1, "Firmware update");

    firmware_update_scr_filename_label = lv_label_create(scr);

    firmware_update_scr_status_label = lv_label_create(scr);

    return scr;
}

static lv_obj_t * create_scr_connection(SystemModule * module) {
    lv_obj_t * scr = lv_obj_create(NULL);

    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *label1 = lv_label_create(scr);
    lv_obj_set_width(label1, LV_HOR_RES);
    lv_label_set_long_mode(label1, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text_fmt(label1, "Wi-Fi: %s", module->ssid);

    wifi_status_label = lv_label_create(scr);
    lv_obj_set_width(wifi_status_label, LV_HOR_RES);
    lv_label_set_long_mode(wifi_status_label, LV_LABEL_LONG_SCROLL_CIRCULAR);

    lv_obj_t *label3 = lv_label_create(scr);
    lv_label_set_text(label3, "Wi-Fi (for setup):");

    lv_obj_t *label4 = lv_label_create(scr);
    lv_label_set_text(label4, module->ap_ssid);

    return scr;
}

static lv_obj_t * create_scr_bitaxe_logo(const char * name, const char * board_version) {
    lv_obj_t * scr = lv_obj_create(NULL);

    lv_obj_t *img = lv_img_create(scr);
    lv_img_set_src(img, &bitaxe_logo);
    lv_obj_align(img, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t *label1 = lv_label_create(scr);
    lv_label_set_text(label1, name);
    lv_obj_align(label1, LV_ALIGN_TOP_RIGHT, -6, 0);

    lv_obj_t *label2 = lv_label_create(scr);
    lv_label_set_text(label2, board_version);
    lv_obj_align(label2, LV_ALIGN_TOP_RIGHT, -6, 8);

    return scr;
}

static lv_obj_t * create_scr_osmu_logo() {
    lv_obj_t * scr = lv_obj_create(NULL);

    lv_obj_t *img = lv_img_create(scr);
    lv_img_set_src(img, &osmu_logo);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);

    return scr;
}

static lv_obj_t * create_scr_urls(SystemModule * module) {
    lv_obj_t * scr = lv_obj_create(NULL);

    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *label1 = lv_label_create(scr);
    lv_label_set_text(label1, "Stratum Host:");

    mining_url_scr_urls_label = lv_label_create(scr);
    lv_obj_set_width(mining_url_scr_urls_label, LV_HOR_RES);
    lv_label_set_long_mode(mining_url_scr_urls_label, LV_LABEL_LONG_SCROLL_CIRCULAR);

    lv_obj_t *label3 = lv_label_create(scr);
    lv_label_set_text(label3, "IP Address:");

    ip_addr_scr_urls_label = lv_label_create(scr);

    return scr;
}

static lv_obj_t * create_scr_unpainted(SystemModule * module) {
    lv_obj_t * scr = lv_obj_create(NULL);

    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *label1 = lv_label_create(scr);
    lv_label_set_text(label1, "  unpainted  ");

    lv_obj_t *label2 = lv_label_create(scr);
    lv_label_set_text(label2, esp_app_get_description()->version);

    return scr;
}

static lv_obj_t * create_scr_stats() {
    lv_obj_t * scr = lv_obj_create(NULL);

    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    hashrate_label = lv_label_create(scr);
    lv_label_set_text(hashrate_label, "Gh/s: --");

    efficiency_label = lv_label_create(scr);
    lv_label_set_text(efficiency_label, "J/Th: --");

    difficulty_label = lv_label_create(scr);
    lv_label_set_text(difficulty_label, "Best: --");

    chip_temp_label = lv_label_create(scr);
    lv_label_set_text(chip_temp_label, "Temp: --");

    return scr;
}

static lv_obj_t * create_scr_wifi_rssi() {
    lv_obj_t * scr = lv_obj_create(NULL);

    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *title_label = lv_label_create(scr);
    lv_label_set_text(title_label, "Wi-Fi Signal");

    wifi_rssi_value_label = lv_label_create(scr);
    lv_label_set_text(wifi_rssi_value_label, "RSSI: -- dBm");

    esp_uptime_label = lv_label_create(scr);
    lv_label_set_text(esp_uptime_label, "Uptime: --");

    return scr;
}

static void screen_show(screen_t screen)
{
    if (SCR_CAROUSEL_START > screen) {
        lv_display_trigger_activity(NULL);
    }

    if (current_screen != screen) {
        lv_obj_t * scr = screens[screen];

        if (scr && lvgl_port_lock(0)) {
            lv_screen_load_anim(scr, LV_SCR_LOAD_ANIM_MOVE_LEFT, LV_DEF_REFR_PERIOD * 128 / 8, 0, false);
            lvgl_port_unlock();
        }

        current_screen = screen;
        current_screen_time_ms = 0;
        current_screen_delay_ms = delays_ms[screen];
    }
}

static void screen_update_cb(lv_timer_t * timer)
{
    int32_t display_timeout_config = nvs_config_get_i32(NVS_CONFIG_DISPLAY_TIMEOUT, -1);

    if (0 > display_timeout_config) {
        // display always on
        display_on(true);
    } else if (0 == display_timeout_config) {
        // display off
        display_on(false);
    } else {
        // display timeout
        const uint32_t display_timeout = display_timeout_config * 60 * 1000;

        if ((lv_display_get_inactive_time(NULL) > display_timeout) && (SCR_CAROUSEL_START <= current_screen)) {
            display_on(false);
        } else {
            display_on(true);
        }
    }
    // Update WiFi RSSI periodically
    int8_t rssi_value = -128; // Invalid value by default
    if (GLOBAL_STATE->SYSTEM_MODULE.is_connected) {
        get_wifi_current_rssi(&rssi_value);
    }
    
    

    if (GLOBAL_STATE->SELF_TEST_MODULE.active) {

        screen_show(SCR_SELF_TEST);

        SelfTestModule * self_test = &GLOBAL_STATE->SELF_TEST_MODULE;

        lv_label_set_text(self_test_message_label, self_test->message);

        if (self_test->finished && !self_test_finished) {
            self_test_finished = true;

            if (self_test->result) {
                lv_label_set_text(self_test_result_label, "TESTS PASS!");
                lv_label_set_text(self_test_finished_label, "Press RESET button to start Bitaxe.");
            } else {
                lv_label_set_text(self_test_result_label, "TESTS FAIL!");
                lv_label_set_text(self_test_finished_label, "Hold BOOT button for 2 seconds to cancel self test, or press RESET to run again.");
            }
        }

        return;
    }

    if (GLOBAL_STATE->SYSTEM_MODULE.is_firmware_update) {
        if (strcmp(GLOBAL_STATE->SYSTEM_MODULE.firmware_update_filename, lv_label_get_text(firmware_update_scr_filename_label)) != 0) {
            lv_label_set_text(firmware_update_scr_filename_label, GLOBAL_STATE->SYSTEM_MODULE.firmware_update_filename);
        }
        if (strcmp(GLOBAL_STATE->SYSTEM_MODULE.firmware_update_status, lv_label_get_text(firmware_update_scr_status_label)) != 0) {
            lv_label_set_text(firmware_update_scr_status_label, GLOBAL_STATE->SYSTEM_MODULE.firmware_update_status);
        }
        screen_show(SCR_FIRMWARE_UPDATE);
        return;
    }

    SystemModule * module = &GLOBAL_STATE->SYSTEM_MODULE;

    if (module->asic_status) {
        lv_label_set_text(asic_status_label, module->asic_status);
        screen_show(SCR_ASIC_STATUS);
        return;
    }

    if (module->overheat_mode == 1) {
        if (strcmp(module->ip_addr_str, lv_label_get_text(ip_addr_scr_overheat_label)) != 0) {
            lv_label_set_text(ip_addr_scr_overheat_label, module->ip_addr_str);
        }
        screen_show(SCR_OVERHEAT);
        return;
    }

    if (module->ssid[0] == '\0') {
        screen_show(SCR_CONFIGURE);
        return;
    }

    if (module->ap_enabled) {
        if (strcmp(module->wifi_status, lv_label_get_text(wifi_status_label)) != 0) {
            lv_label_set_text(wifi_status_label, module->wifi_status);
        }
        screen_show(SCR_CONNECTION);
        current_screen_time_ms = 0;
        return;
    }

    // Carousel

    current_screen_time_ms += SCREEN_UPDATE_MS;

    PowerManagementModule * power_management = &GLOBAL_STATE->POWER_MANAGEMENT_MODULE;

    char *pool_url = module->is_using_fallback ? module->fallback_pool_url : module->pool_url;
    if (strcmp(lv_label_get_text(mining_url_scr_urls_label), pool_url) != 0) {
        lv_label_set_text(mining_url_scr_urls_label, pool_url);
    }

    if (strcmp(lv_label_get_text(ip_addr_scr_urls_label), module->ip_addr_str) != 0) {
        lv_label_set_text(ip_addr_scr_urls_label, module->ip_addr_str);
    }

    if (current_hashrate != module->current_hashrate) {
        lv_label_set_text_fmt(hashrate_label, "Gh/s: %.2f", module->current_hashrate);
    }

    if (current_power != power_management->power || current_hashrate != module->current_hashrate) {
        if (power_management->power > 0 && module->current_hashrate > 0) {
            float efficiency = power_management->power / (module->current_hashrate / 1000.0);
            lv_label_set_text_fmt(efficiency_label, "J/Th: %.2f", efficiency);
        }
    }

    if (module->FOUND_BLOCK && !found_block) {
        found_block = true;

        lv_obj_set_width(difficulty_label, LV_HOR_RES);
        lv_label_set_long_mode(difficulty_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_label_set_text_fmt(difficulty_label, "Best: %s   !!! BLOCK FOUND !!!", module->best_session_diff_string);

        screen_show(SCR_STATS);
        lv_display_trigger_activity(NULL);
    } else {
        if (current_difficulty != module->best_session_nonce_diff) {
            lv_label_set_text_fmt(difficulty_label, "Best: %s/%s", module->best_session_diff_string, module->best_diff_string);
        }
    }

    if (current_chip_temp != power_management->chip_temp_avg && power_management->chip_temp_avg > 0) {
        lv_label_set_text_fmt(chip_temp_label, "Temp: %.1f C", power_management->chip_temp_avg);
    }

    
    char rssi_buf[25];
        
    if (rssi_value < 0 && rssi_value >= -127) { // Typical RSSI range
        snprintf(rssi_buf, sizeof(rssi_buf), "RSSI: %d dBm", rssi_value);
    } else {
        snprintf(rssi_buf, sizeof(rssi_buf), "RSSI: -- dBm");
        }
    if (strcmp(lv_label_get_text(wifi_rssi_value_label), rssi_buf) != 0) {
        lv_label_set_text(wifi_rssi_value_label, rssi_buf);
    }
    

    current_hashrate = module->current_hashrate;
    current_power = power_management->power;
    current_difficulty = module->best_session_nonce_diff;
    current_chip_temp = power_management->chip_temp_avg;

    if (current_screen_time_ms <= current_screen_delay_ms || found_block) {
        return;
    }

    screen_next();
}

void screen_next()
{
    screen_t next_scr = current_screen;

    // Loop to find the next screen that should be displayed
    do {
        next_scr++; // Advance to the next screen candidate
        if (next_scr > SCR_CAROUSEL_END) { // If past the end of carousel
            next_scr = SCR_CAROUSEL_START; // Wrap around to the start of carousel
        }
        // If the candidate screen is SCR_WIFI_RSSI AND this is NOT a bigger display,
        // then this screen should be skipped, and the loop will continue to find the next one.
    } while (next_scr == SCR_WIFI_RSSI && GLOBAL_STATE->DISPLAY_CONFIG.v_res / 8 != 8);

    screen_show(next_scr);
}

static void uptime_update_cb(lv_timer_t * timer)
{
    if (esp_uptime_label) {
        char uptime[50];
        uint32_t uptime_seconds = (esp_timer_get_time() - GLOBAL_STATE->SYSTEM_MODULE.start_time) / 1000000;
        
        uint32_t days = uptime_seconds / (24 * 3600);
        uptime_seconds %= (24 * 3600);
        uint32_t hours = uptime_seconds / 3600;
        uptime_seconds %= 3600;
        uint32_t minutes = uptime_seconds / 60;
        uptime_seconds %= 60;
        
        if (days > 0) {
            snprintf(uptime, sizeof(uptime), "Uptime: %ldd %ldh %ldm %lds", days, hours, minutes, uptime_seconds);
        } else if (hours > 0) {
            snprintf(uptime, sizeof(uptime), "Uptime: %ldh %ldm %lds", hours, minutes, uptime_seconds);
        } else if (minutes > 0) {
            snprintf(uptime, sizeof(uptime), "Uptime: %ldm %lds", minutes, uptime_seconds);
        } else {
            snprintf(uptime, sizeof(uptime), "Uptime: %lds", uptime_seconds);
        }
        
        if (strcmp(lv_label_get_text(esp_uptime_label), uptime) != 0) {
            lv_label_set_text(esp_uptime_label, uptime);
        }
    }
}

esp_err_t screen_start(void * pvParameters)
{
    GLOBAL_STATE = (GlobalState *) pvParameters;

    if (GLOBAL_STATE->SYSTEM_MODULE.is_screen_active) {
        SystemModule * module = &GLOBAL_STATE->SYSTEM_MODULE;

        screens[SCR_SELF_TEST] = create_scr_self_test();
        screens[SCR_OVERHEAT] = create_scr_overheat(module);
        screens[SCR_ASIC_STATUS] = create_scr_asic_status(module);
        screens[SCR_CONFIGURE] = create_scr_configure(module);
        screens[SCR_FIRMWARE_UPDATE] = create_scr_ota(module);
        screens[SCR_CONNECTION] = create_scr_connection(module);
        screens[SCR_BITAXE_LOGO] = create_scr_bitaxe_logo(GLOBAL_STATE->DEVICE_CONFIG.family.name, GLOBAL_STATE->DEVICE_CONFIG.board_version);
        screens[SCR_OSMU_LOGO] = create_scr_unpainted(module);
        screens[SCR_URLS] = create_scr_urls(module);
        screens[SCR_STATS] = create_scr_stats();
        screens[SCR_WIFI_RSSI] = create_scr_wifi_rssi();

        lv_timer_create(screen_update_cb, SCREEN_UPDATE_MS, NULL);
        
        // Create uptime update timer (runs every 1 second)
        lv_timer_create(uptime_update_cb, 1000, NULL);
    }

    return ESP_OK;
}