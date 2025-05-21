
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include "mbedtls/sha256.h"


#if defined(__GNUC__) || defined(__clang__)
#define swab16(x) __builtin_bswap16(x)
#define swab32(x) __builtin_bswap32(x)
#define swab64(x) __builtin_bswap64(x)
#elif defined(_MSC_VER)
#define swab16(x) _byteswap_ushort(x)
#define swab32(x) _byteswap_ulong(x)
#define swab64(x) _byteswap_uint64(x)
#else

static inline uint16_t swab16(uint16_t x) {
    return (x << 8) | (x >> 8);
}

static inline uint32_t swab32(uint32_t x) {
    return ((x & 0xff000000) >> 24) | ((x & 0x00ff0000) >>  8) |
           ((x & 0x0000ff00) <<  8) | ((x & 0x000000ff) << 24);
}

static inline uint64_t swab64(uint64_t x) {
    return ((x & 0xff00000000000000ull) >> 56) |
           ((x & 0x00ff000000000000ull) >> 40) |
           ((x & 0x0000ff0000000000ull) >> 24) |
           ((x & 0x000000ff00000000ull) >>  8) |
           ((x & 0x00000000ff000000ull) <<  8) |
           ((x & 0x0000000000ff0000ull) << 24) |
           ((x & 0x000000000000ff00ull) << 40) |
           ((x & 0x00000000000000ffull) << 56);
}
#endif

__attribute__((hot, always_inline)) 
inline void flip32bytes(void *restrict dest_p, const void *restrict src_p) {
	
    uint32_t temp[8];
    memcpy(temp, src_p, 32); // Ensure 32-bit alignment

    temp[0] = swab32(temp[0]);
    temp[1] = swab32(temp[1]);
    temp[2] = swab32(temp[2]);
    temp[3] = swab32(temp[3]);
    temp[4] = swab32(temp[4]);
    temp[5] = swab32(temp[5]);
    temp[6] = swab32(temp[6]);
    temp[7] = swab32(temp[7]);

    memcpy(dest_p, temp, 32);
}

static const char hex_digits[16] = "0123456789abcdef";
static const uint8_t hex_vals[256] = {
    ['0'] = 0,  ['1'] = 1,  ['2'] = 2,  ['3'] = 3,
    ['4'] = 4,  ['5'] = 5,  ['6'] = 6,  ['7'] = 7,
    ['8'] = 8,  ['9'] = 9,  ['a'] = 10, ['b'] = 11,
    ['c'] = 12, ['d'] = 13, ['e'] = 14, ['f'] = 15,
    ['A'] = 10, ['B'] = 11, ['C'] = 12, ['D'] = 13,
    ['E'] = 14, ['F'] = 15
};

__attribute__((hot))
size_t hex2bin(const char *restrict hex, uint8_t *restrict bin, size_t bin_len) {
	
    size_t count = 0;
    
    while (*hex && count < bin_len) {
        uint8_t msn = hex_vals[(uint8_t)*hex++];
        if (!*hex) break;
        
        uint8_t lsn = hex_vals[(uint8_t)*hex++];
        bin[count++] = (msn << 4) | lsn;
    }
    
    return count;
}

__attribute__((hot))
size_t bin2hex(const uint8_t *restrict bin, size_t bin_len, char *restrict hex, size_t hex_len) {
	
    if (hex_len < (bin_len * 2 + 1)) return 0;
    
    for (size_t i = 0; i < bin_len; i++) {
        hex[i*2]   = hex_digits[bin[i] >> 4];
        hex[i*2+1] = hex_digits[bin[i] & 0x0F];
    }
    hex[bin_len*2] = '\0';
    
    return bin_len * 2;
}

uint8_t *double_sha256_bin(const uint8_t *data, const size_t data_len) {
	
    uint8_t first_hash_output[32];
	uint8_t *second_hash_output = malloc(32);

    mbedtls_sha256(data, data_len, first_hash_output, 0);
    mbedtls_sha256(first_hash_output, 32, second_hash_output, 0);

    return second_hash_output;
}

void single_sha256_bin(const uint8_t *data, const size_t data_len, uint8_t *dest) {
	
    // Initialize SHA256 context
    mbedtls_sha256_context sha256_ctx;
    mbedtls_sha256_init(&sha256_ctx);
    mbedtls_sha256_starts(&sha256_ctx, 0);

    // Compute first SHA256 hash of header
    mbedtls_sha256_update(&sha256_ctx, data, 64);
    
    // Compute midstate from hash
    unsigned char hash[32];
    mbedtls_sha256_finish(&sha256_ctx, hash);
    memcpy(dest, hash, 32);
}

void midstate_sha256_bin(const uint8_t *data, const size_t data_len, uint8_t *dest) {
	
    mbedtls_sha256_context midstate;

    // Calculate midstate
    mbedtls_sha256_init(&midstate);
    mbedtls_sha256_starts(&midstate, 0);
    mbedtls_sha256_update(&midstate, data, 64);

    flip32bytes(dest, midstate.state);
}

void swap_endian_words(const char *hex_words, uint8_t *output) {
	
    size_t hex_length = strlen(hex_words);
    if (hex_length % 8 != 0)
    {
        fprintf(stderr, "Must be 4-byte word aligned\n");
        exit(EXIT_FAILURE);
    }
	for(size_t i=0; i<hex_length; i+=8) {
	    uint32_t word;
	    hex2bin(hex_words+i, (uint8_t*)&word, 4);
	    *(uint32_t*)(output+i/2) = swab32(word);
	}
}

__attribute__((hot)) 
void reverse_bytes(uint8_t *data, size_t len) {
	
    uint8_t *start = data;
    uint8_t *end = data + len - 1;
    
    while (start < end) {
        uint8_t temp = *start;
        *start++ = *end;
        *end-- = temp;
    }
}

// Constants as power-of-two for exact representation
static const double pow2_192 = 6277101735386680763835789423207666416102355444464034512896.0; // 2^192
static const double pow2_128 = 340282366920938463463374607431768211456.0; // 2^128
static const double pow2_64  = 18446744073709551616.0; // 2^64

__attribute__((hot))
double le256todouble(const void *restrict target) {
	
    const uint8_t *t = target;
    uint64_t chunk;
    
    memcpy(&chunk, t + 24, 8);
    double result = (double)chunk * pow2_192;
    
    memcpy(&chunk, t + 16, 8);
    result += (double)chunk * pow2_128;
    
    memcpy(&chunk, t + 8, 8);
    result += (double)chunk * pow2_64;
    
    memcpy(&chunk, t, 8);
    result += (double)chunk;
    
    return result;
}

void prettyHex(unsigned char *buf, int len) {
	
    int i;
    printf("[");
    for (i = 0; i < len - 1; i++)
    {
        printf("%02X ", buf[i]);
    }
    printf("%02X]", buf[len - 1]);
}

void print_hex(const uint8_t *b, size_t len, const size_t in_line, const char *prefix) {
	
    size_t i = 0;
    const uint8_t *end = b + len;

    if (prefix == NULL)
    {
        prefix = "";
    }

    printf("%s", prefix);
    while (b < end)
    {
        if (++i > in_line)
        {
            printf("\n%s", prefix);
            i = 1;
        }
        printf("%02X ", (uint8_t)*b++);
    }
    printf("\n");
    fflush(stdout);
}
