uint32_t get_ticket_mask_from_table(uint32_t ticket_mask) {
    for (int i = 0; i < TICKET_MASK_TABLE_SIZE; i++) {
        if (TICKET_MASK_TABLE[i].difficulty == ticket_mask) {
            return TICKET_MASK_TABLE[i].difficulty;
        }
    }
    return 256; // Default fallback
}

uint32_t get_version_mask_from_table(uint32_t version_mask) {
    for (int i = 0; i < VERSION_MASK_TABLE_SIZE; i++) {
        if (VERSION_MASK_TABLE[i].mask == version_mask) {
            return VERSION_MASK_TABLE[i].roll_count;
        }
    }
    return 65536; // Default for 0x1FFFE000
}

typedef struct
{
	double hr_cal_factor;
	uint8_t hr_interval_seconds;
    uint32_t hr_total_work;
    double hr_smooting_factor;
} Hashrate_t;

Hashrate_t Hashrate_cfg = {
	.hr_cal_factor = 1.0,			// Factor Formula:  Observed 1hr avg miner hashrate  / Observed 1hr avg pool hashrate. 
									// Example: 800GH / 750GH = 1.06666666667 Calibration Factor
	.hr_interval_seconds = 5,		// Update rate in seconds 
	.hr_smooting_factor = 0.1		// 10% weight to new values (adjust between 0.05–0.2)
};

void esp_miner_notify_found_nonce(Esp_Miner_t *esp_miner) {

    uint32_t ticket_mask = get_ticket_mask_from_table(esp_miner->ticket_mask);
    uint32_t version_mask = get_version_mask_from_table(esp_miner->version_mask);
    
    // ACTUAL work represented by this nonce
    // e.g. 128 * 65536 * Double SHA-256 = 16,777,216 hashes
    uint64_t work_units = ticket_mask * version_mask * 2; 
    Hashrate_cfg.hr_total_work += work_units;
    
    // Get time since last calculation
    static uint64_t last_time = 0;
    uint64_t current_time = esp_timer_get_time();
    double elapsed_sec = (current_time - last_time) / 1e6;
        
	if (elapsed_sec >= Hashrate_cfg.hr_interval_seconds) {

		// Runtime 34000 raw calibration constant.
	    double hashrate_gh = (Hashrate_cfg.hr_total_work / elapsed_sec) / 1e9 * (34000 / Hashrate_cfg.hr_cal_factor); 
	    
	    esp_miner->current_hashrate = (esp_miner->current_hashrate * 
	    	(1.0 - Hashrate_cfg.hr_smooting_factor)) + (hashrate_gh * Hashrate_cfg.hr_smooting_factor);

	    Hashrate_cfg.hr_total_work = 0;
	    last_time = current_time;
	    
	    // DEBUG
        printf(
            "TMask=%"PRIu32", VMask=%"PRIu32" | "
            "Work: %.6f TH | " "Elapsed: %.2fs | "
            "GH/s: %.6f\n",
            ticket_mask, version_mask,
            work_units / 1e9, elapsed_sec,
            (esp_miner->current_hashrate)
        );
    }
}
