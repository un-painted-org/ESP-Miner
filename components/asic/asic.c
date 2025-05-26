#include <string.h>

#include <esp_log.h>

#include "bm1397.h"
#include "bm1366.h"
#include "bm1368.h"
#include "bm1370.h"

#include "asic.h"

static const double NONCE_SPACE = 4294967296.0; //  2^32

static const char *TAG = "asic";

// .init_fn = BM1366_init,
uint8_t ASIC_init(GlobalState * GLOBAL_STATE) {
    switch (GLOBAL_STATE->device_model) {
        case DEVICE_LV07:
            return BM1366_init(GLOBAL_STATE->POWER_MANAGEMENT_MODULE.frequency_value, LV07_ASIC_COUNT);
        case DEVICE_MAX:
            return BM1397_init(GLOBAL_STATE->POWER_MANAGEMENT_MODULE.frequency_value, BITAXE_MAX_ASIC_COUNT);
        case DEVICE_ULTRA:
            return BM1366_init(GLOBAL_STATE->POWER_MANAGEMENT_MODULE.frequency_value, BITAXE_ULTRA_ASIC_COUNT);
        case DEVICE_SUPRA:
            return BM1368_init(GLOBAL_STATE->POWER_MANAGEMENT_MODULE.frequency_value, BITAXE_SUPRA_ASIC_COUNT);
        case DEVICE_GAMMA:
            return BM1370_init(GLOBAL_STATE->POWER_MANAGEMENT_MODULE.frequency_value, BITAXE_GAMMA_ASIC_COUNT);
        case DEVICE_GAMMATURBO:
            return BM1370_init(GLOBAL_STATE->POWER_MANAGEMENT_MODULE.frequency_value, BITAXE_GAMMATURBO_ASIC_COUNT);
        default:
    }
    return ESP_OK;
}

uint8_t ASIC_get_asic_count(GlobalState * GLOBAL_STATE) {
    switch (GLOBAL_STATE->device_model) {
        case DEVICE_LV07:
            return LV07_ASIC_COUNT;
        case DEVICE_MAX:
            return BITAXE_MAX_ASIC_COUNT;
        case DEVICE_ULTRA:
            return BITAXE_ULTRA_ASIC_COUNT;
        case DEVICE_SUPRA:
            return BITAXE_SUPRA_ASIC_COUNT;
        case DEVICE_GAMMA:
            return BITAXE_GAMMA_ASIC_COUNT;
        case DEVICE_GAMMATURBO:
            return BITAXE_GAMMATURBO_ASIC_COUNT;
        default:
    }
    return 0;
}

uint16_t ASIC_get_small_core_count(GlobalState * GLOBAL_STATE) {
    switch (GLOBAL_STATE->device_model) {
        case DEVICE_MAX:
            return BM1397_SMALL_CORE_COUNT;
        case DEVICE_ULTRA:
        case DEVICE_LV07:
            return BM1366_SMALL_CORE_COUNT;
        case DEVICE_SUPRA:
            return BM1368_SMALL_CORE_COUNT;
        case DEVICE_GAMMA:
            return BM1370_SMALL_CORE_COUNT;
        case DEVICE_GAMMATURBO:
            return BM1370_SMALL_CORE_COUNT;
        default:
    }
    return 0;
}

// .receive_result_fn = BM1366_process_work,
task_result * ASIC_process_work(GlobalState * GLOBAL_STATE) {
    switch (GLOBAL_STATE->device_model) {
        case DEVICE_MAX:
            return BM1397_process_work(GLOBAL_STATE);
        case DEVICE_ULTRA:
        case DEVICE_LV07:
            return BM1366_process_work(GLOBAL_STATE);
        case DEVICE_SUPRA:
            return BM1368_process_work(GLOBAL_STATE);
        case DEVICE_GAMMA:
        case DEVICE_GAMMATURBO:
            return BM1370_process_work(GLOBAL_STATE);
        default:
    }
    return NULL;
}

// .set_max_baud_fn = BM1366_set_max_baud,
int ASIC_set_max_baud(GlobalState * GLOBAL_STATE) {
    switch (GLOBAL_STATE->device_model) {
        case DEVICE_MAX:
            return BM1397_set_max_baud();
        case DEVICE_ULTRA:
        case DEVICE_LV07:
            return BM1366_set_max_baud();
        case DEVICE_SUPRA:
            return BM1368_set_max_baud();
        case DEVICE_GAMMA:
        case DEVICE_GAMMATURBO:
            return BM1370_set_max_baud();
        default:
    return 0;
    }
}

// .set_difficulty_mask_fn = BM1366_set_job_difficulty_mask,
void ASIC_set_job_difficulty_mask(GlobalState * GLOBAL_STATE, uint8_t mask) {
    switch (GLOBAL_STATE->device_model) {
        case DEVICE_MAX:
            BM1397_set_job_difficulty_mask(mask);
            break;
        case DEVICE_ULTRA:
        case DEVICE_LV07:
            BM1366_set_job_difficulty_mask(mask);
            break;
        case DEVICE_SUPRA:
            BM1368_set_job_difficulty_mask(mask);
            break;
        case DEVICE_GAMMA:
        case DEVICE_GAMMATURBO:
            BM1370_set_job_difficulty_mask(mask);
            break;
        default:
    }
}

// .send_work_fn = BM1366_send_work,
void ASIC_send_work(GlobalState * GLOBAL_STATE, void * next_job) {
    switch (GLOBAL_STATE->device_model) {
        case DEVICE_MAX:
            BM1397_send_work(GLOBAL_STATE, next_job);
            break;
        case DEVICE_ULTRA:
        case DEVICE_LV07:
            BM1366_send_work(GLOBAL_STATE, next_job);
            break;
        case DEVICE_SUPRA:
            BM1368_send_work(GLOBAL_STATE, next_job);
            break;
        case DEVICE_GAMMA:
        case DEVICE_GAMMATURBO:
            BM1370_send_work(GLOBAL_STATE, next_job);
            break;
        default:
    return;
    }
}

// .set_version_mask = BM1366_set_version_mask
void ASIC_set_version_mask(GlobalState * GLOBAL_STATE, uint32_t mask) {
    
    ASIC_set_version_mask(mask, true);

    /*
    switch (GLOBAL_STATE->device_model) {
        case DEVICE_MAX:
            BM1397_set_version_mask(mask);
            break;
        case DEVICE_ULTRA:
        case DEVICE_LV07:
            BM1366_set_version_mask(mask);
            break;
        case DEVICE_SUPRA:
            BM1368_set_version_mask(mask);
            break;
        case DEVICE_GAMMA:
        case DEVICE_GAMMATURBO:
            BM1370_set_version_mask(mask);
            break;
        default:
    return;
    }
    */
}

//by Mecanix
void ASIC_set_ticket_mask(uint32_t difficulty) {
	
    const AsicMaskEntry_t *entry = NULL;
    for (int i = 0; i < TICKET_MASK_TABLE_SIZE; i++) {
        if (TICKET_MASK_TABLE[i].difficulty >= difficulty) {
            entry = &TICKET_MASK_TABLE[i];
            break;
        }
    }
    if (!entry) entry = &TICKET_MASK_TABLE[TICKET_MASK_TABLE_SIZE - 1]; //Fallback

    uint8_t packet[6] = {0x00, 0x14, 0x00, 0x00, entry->asic_bytes[0], entry->asic_bytes[1]};
    _send(TYPE_CMD | GROUP_ALL | CMD_WRITE, packet, 6, SERIALTX_DEBUG);
    
    // DEBUG
    ESP_LOGI(TAG, "New ticket mask: %02X %02X %02X %02X %02X %02X (Difficulty: 0x%"PRIX32")", packet[0], packet[1], packet[2], packet[3], packet[4], packet[5], difficulty);
}

//by Mecanix
void ASIC_set_version_mask(uint32_t value, bool is_direct_mask) {
	
    const AsicMaskEntry_t *entry = NULL;
    uint32_t roll_count = is_direct_mask ? (value >> 13) : value;

    // Find closest match (>= roll_count)
    for (int i = 0; i < VERSION_MASK_TABLE_SIZE; i++) {
        if (VERSION_MASK_TABLE[i].roll_count >= roll_count) {
            entry = &VERSION_MASK_TABLE[i];
            break;
        }
    }
    if (!entry) entry = &VERSION_MASK_TABLE[VERSION_MASK_TABLE_SIZE - 1]; //Fallback

    uint8_t packet[6] = {0x00, 0xA4, 0x90, 0x00, entry->asic_bytes[0], entry->asic_bytes[1]};
    _send(TYPE_CMD | GROUP_ALL | CMD_WRITE, packet, 6, SERIALTX_DEBUG);

    // DEBUG
    ESP_LOGI(TAG, "New version mask: %02X %02X %02X %02X %02X %02X ", packet[0], packet[1], packet[2], packet[3], packet[4], packet[5]);
}

bool ASIC_set_frequency(GlobalState * GLOBAL_STATE, float target_frequency) {
    ESP_LOGI(TAG, "Setting ASIC frequency to %.2f MHz", target_frequency);
    bool success = false;
    
    switch (GLOBAL_STATE->asic_model) {
        case ASIC_BM1366:
            success = BM1366_set_frequency(target_frequency);
            break;
        case ASIC_BM1368:
            success = BM1368_set_frequency(target_frequency);
            break;
        case ASIC_BM1370:
            success = BM1370_set_frequency(target_frequency);
            break;
        case ASIC_BM1397:
            // BM1397 doesn't have a set_frequency function yet
            ESP_LOGE(TAG, "Frequency transition not implemented for BM1397");
            success = false;
            break;
        default:
            ESP_LOGE(TAG, "Unknown ASIC model, cannot set frequency");
            success = false;
            break;
    }
    
    if (success) {
        ESP_LOGI(TAG, "Successfully transitioned to new ASIC frequency: %.2f MHz", target_frequency);
    } else {
        ESP_LOGE(TAG, "Failed to transition to new ASIC frequency: %.2f MHz", target_frequency);
    }
    
    return success;
}

esp_err_t ASIC_set_device_model(GlobalState * GLOBAL_STATE) {

    if (GLOBAL_STATE->device_model_str == NULL) {
        ESP_LOGE(TAG, "No device model string found");
        return ESP_FAIL;
    }

    if (strcmp(GLOBAL_STATE->device_model_str, "max") == 0) {
        GLOBAL_STATE->asic_model = ASIC_BM1397;
        GLOBAL_STATE->asic_job_frequency_ms = (NONCE_SPACE / (double) (GLOBAL_STATE->POWER_MANAGEMENT_MODULE.frequency_value * BM1397_SMALL_CORE_COUNT * 1000)) / (double) ASIC_get_asic_count(GLOBAL_STATE); // no version-rolling so same Nonce Space is splitted between Small Cores
        GLOBAL_STATE->ASIC_difficulty = BM1397_ASIC_DIFFICULTY;
        ESP_LOGI(TAG, "DEVICE: bitaxeMax");
        ESP_LOGI(TAG, "ASIC: %dx BM1397 (%" PRIu64 " cores)", BITAXE_MAX_ASIC_COUNT, BM1397_CORE_COUNT);
        GLOBAL_STATE->device_model = DEVICE_MAX;

    } else if (strcmp(GLOBAL_STATE->device_model_str, "ultra") == 0) {
        GLOBAL_STATE->asic_model = ASIC_BM1366;
        //GLOBAL_STATE.asic_job_frequency_ms = (NONCE_SPACE / (double) (GLOBAL_STATE.POWER_MANAGEMENT_MODULE.frequency_value * BM1366_CORE_COUNT * 1000)) / (double) BITAXE_ULTRA_ASIC_COUNT; // version-rolling so Small Cores have different Nonce Space
        GLOBAL_STATE->asic_job_frequency_ms = 2000; //ms
        GLOBAL_STATE->ASIC_difficulty = BM1366_ASIC_DIFFICULTY;
        ESP_LOGI(TAG, "DEVICE: bitaxeUltra");
        ESP_LOGI(TAG, "ASIC: %dx BM1366 (%" PRIu64 " cores)", BITAXE_ULTRA_ASIC_COUNT, BM1366_CORE_COUNT);
        GLOBAL_STATE->device_model = DEVICE_ULTRA;

    } else if (strcmp(GLOBAL_STATE->device_model_str, "lv07") == 0) {
        GLOBAL_STATE->asic_model = ASIC_BM1366;
        //GLOBAL_STATE.asic_job_frequency_ms = (NONCE_SPACE / (double) (GLOBAL_STATE.POWER_MANAGEMENT_MODULE.frequency_value * BM1366_CORE_COUNT * 1000)) / (double) BITAXE_ULTRA_ASIC_COUNT; // version-rolling so Small Cores have different Nonce Space
        GLOBAL_STATE->asic_job_frequency_ms = 2000; //ms
        GLOBAL_STATE->ASIC_difficulty = BM1366_ASIC_DIFFICULTY;
        ESP_LOGI(TAG, "DEVICE: Lucky Miner LV07");
        ESP_LOGI(TAG, "ASIC: %dx BM1366 (%" PRIu64 " cores)", LV07_ASIC_COUNT, BM1366_CORE_COUNT);
        GLOBAL_STATE->device_model = DEVICE_LV07;

    } else if (strcmp(GLOBAL_STATE->device_model_str, "supra") == 0) {
        GLOBAL_STATE->asic_model = ASIC_BM1368;
        //GLOBAL_STATE.asic_job_frequency_ms = (NONCE_SPACE / (double) (GLOBAL_STATE.POWER_MANAGEMENT_MODULE.frequency_value * BM1368_CORE_COUNT * 1000)) / (double) BITAXE_SUPRA_ASIC_COUNT; // version-rolling so Small Cores have different Nonce Space
        GLOBAL_STATE->asic_job_frequency_ms = 500; //ms
        GLOBAL_STATE->ASIC_difficulty = BM1368_ASIC_DIFFICULTY;
        ESP_LOGI(TAG, "DEVICE: bitaxeSupra");
        ESP_LOGI(TAG, "ASIC: %dx BM1368 (%" PRIu64 " cores)", BITAXE_SUPRA_ASIC_COUNT, BM1368_CORE_COUNT);
        GLOBAL_STATE->device_model = DEVICE_SUPRA;

    } else if (strcmp(GLOBAL_STATE->device_model_str, "gamma") == 0) {
        GLOBAL_STATE->asic_model = ASIC_BM1370;
        //GLOBAL_STATE.asic_job_frequency_ms = (NONCE_SPACE / (double) (GLOBAL_STATE.POWER_MANAGEMENT_MODULE.frequency_value * BM1370_CORE_COUNT * 1000)) / (double) BITAXE_GAMMA_ASIC_COUNT; // version-rolling so Small Cores have different Nonce Space
        GLOBAL_STATE->asic_job_frequency_ms = 500; //ms
        GLOBAL_STATE->ASIC_difficulty = BM1370_ASIC_DIFFICULTY;
        ESP_LOGI(TAG, "DEVICE: bitaxeGamma");
        ESP_LOGI(TAG, "ASIC: %dx BM1370 (%" PRIu64 " cores)", BITAXE_GAMMA_ASIC_COUNT, BM1370_CORE_COUNT);
        GLOBAL_STATE->device_model = DEVICE_GAMMA;

    } else if (strcmp(GLOBAL_STATE->device_model_str, "gammaturbo") == 0) {
        GLOBAL_STATE->asic_model = ASIC_BM1370;
        //GLOBAL_STATE.asic_job_frequency_ms = (NONCE_SPACE / (double) (GLOBAL_STATE.POWER_MANAGEMENT_MODULE.frequency_value * BM1370_CORE_COUNT * 1000)) / (double) BITAXE_GAMMATURBO_ASIC_COUNT; // version-rolling so Small Cores have different Nonce Space
        GLOBAL_STATE->asic_job_frequency_ms = 500; //ms
        GLOBAL_STATE->ASIC_difficulty = BM1370_ASIC_DIFFICULTY;
        ESP_LOGI(TAG, "DEVICE: bitaxeGammaTurbo");
        ESP_LOGI(TAG, "ASIC: %dx BM1370 (%" PRIu64 " cores)", BITAXE_GAMMATURBO_ASIC_COUNT, BM1370_CORE_COUNT);
        GLOBAL_STATE->device_model = DEVICE_GAMMATURBO;

    } else {
        ESP_LOGE(TAG, "Invalid DEVICE model");
        GLOBAL_STATE->device_model = DEVICE_UNKNOWN;
        return ESP_FAIL;
    }
    return ESP_OK;
}
