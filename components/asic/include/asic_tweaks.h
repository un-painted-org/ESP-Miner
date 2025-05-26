/*
 * asic_tweaks.h
 *
 *  Created on: May 24, 2025
 *      Author: mecanix
 */

#ifndef INC_ASIC_ASIC_TWEAKS_H_
#define INC_ASIC_ASIC_TWEAKS_H_

#include <stdint.h>

typedef struct {
    uint32_t mask; 
    uint32_t difficulty;
    uint32_t roll_count;
    uint8_t asic_bytes[2];
} AsicMaskEntry_t;
extern AsicMaskEntry_t AsicMaskEntry;


static const AsicMaskEntry_t TICKET_MASK_TABLE[] = {
    {0x80000000,	1,		0,	{0x00, 0x80}},
    {0xC0000000,	4,		0,	{0x00, 0xC0}},
    {0xE0000000,	8,		0,	{0x00, 0xE0}},
    {0xF0000000,	16,		0,	{0x00, 0xF0}},
    {0xF8000000,	32,		0,	{0x00, 0xF8}},
    {0xFC000000,	64,		0,	{0x00, 0xFC}},
    {0xFE000000,	128,	0,	{0x00, 0xFE}},
    {0xFF000000,	256,	0,	{0x00, 0xFF}}, // default ESP Miner
    {0xFF800000,	512,	0,	{0x80, 0xFF}},
    {0xFFC00000,	1024,	0,	{0xC0, 0xFF}},
    {0xFFE00000,	2048,	0,	{0xE0, 0xFF}},
    {0xFFF00000,	4096,	0,	{0xF0, 0xFF}},
    {0xFFF80000,	8192,	0,	{0xF8, 0xFF}},
    {0xFFFC0000,	16384,	0,	{0xFC, 0xFF}},
    {0xFFFE0000,	32768,	0,	{0xFE, 0xFF}},
    {0xFFFF0000,	65536,	0,	{0xFF, 0xFF}}
};
#define TICKET_MASK_TABLE_SIZE (sizeof(TICKET_MASK_TABLE) / sizeof(TICKET_MASK_TABLE[0]))


static const AsicMaskEntry_t VERSION_MASK_TABLE[] = {
    {0x00002000, 0, 1,      {0x00, 0x01}},
    {0x00006000, 0, 3,      {0x00, 0x03}},
    {0x0000E000, 0, 7,      {0x00, 0x07}},
    {0x0001E000, 0, 15,     {0x00, 0x0F}},
    {0x0003E000, 0, 31,     {0x00, 0x1F}},
    {0x0007E000, 0, 63,     {0x00, 0x3F}},
    {0x000FE000, 0, 127,    {0x00, 0x7F}},
    {0x001FE000, 0, 255,    {0x00, 0xFF}},
    {0x003FE000, 0, 511,    {0x01, 0xFF}},
    {0x007FE000, 0, 1023,   {0x03, 0xFF}},
    {0x00FFE000, 0, 2047,   {0x07, 0xFF}},
    {0x01FFE000, 0, 4095,   {0x0F, 0xFF}},
    {0x03FFE000, 0, 8191,   {0x1F, 0xFF}},
    {0x07FFE000, 0, 16383,  {0x3F, 0xFF}},
    {0x0FFFE000, 0, 32767,  {0x7F, 0xFF}},
    {0x1FFFE000, 0, 65535,  {0xFF, 0xFF}}  // Most's Default
};
#define VERSION_MASK_TABLE_SIZE (sizeof(VERSION_MASK_TABLE) / sizeof(VERSION_MASK_TABLE[0]))


//void ASIC_set_ticket_mask(uint32_t difficulty);
//void ASIC_set_version_mask(uint32_t value, bool is_direct_mask);


#endif /* INC_ASIC_ASIC_TWEAKS_H_ */
