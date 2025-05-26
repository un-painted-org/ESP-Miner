
#include "asic_tweaks.h"
#include "asic.h"

AsicMaskEntry_t AsicMaskEntry;

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
    //printf("TICKET MASK: 00 14 00 00 %02X %02X (Difficulty: 0x%"PRIX32") \r\n", 
    //	entry->asic_bytes[0], entry->asic_bytes[1], difficulty);
}


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
    //printf("VERSION MASK: 00 A4 90 00 %02X %02X (Mask: 0x%"PRIX32") \r\n", 
    //	entry->asic_bytes[0], entry->asic_bytes[1], entry->mask);
}
