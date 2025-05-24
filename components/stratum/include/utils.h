#ifndef INC_UTILS_H_
#define INC_UTILS_H_

#include <stddef.h>
#include <stdint.h>

#define STRATUM_DEFAULT_VERSION_MASK 0x1fffe000

size_t hex2bin(const char *restrict hex, uint8_t *restrict bin, size_t bin_len);
size_t bin2hex(const uint8_t *restrict bin, size_t bin_len, char *restrict hex, size_t hex_len);
uint8_t *double_sha256_bin(const uint8_t *data, const size_t data_len);
void single_sha256_bin(const uint8_t *data, const size_t data_len, uint8_t *dest);
void midstate_sha256_bin(const uint8_t *data, const size_t data_len, uint8_t *dest);
void swap_endian_words(const char *hex_words, uint8_t *output);
void reverse_bytes(uint8_t *data, size_t len);
double le256todouble(const void *restrict target);
void prettyHex(unsigned char *buf, int len);
void print_hex(const uint8_t *b, size_t len, const size_t in_line, const char *prefix);

#endif /* INC_UTILS_H_ */
