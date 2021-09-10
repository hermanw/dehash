#pragma once

#define HASH_LEN 32
#define BLOCK_LEN 64 // In bytes
#define STATE_LEN 2  // In dwords

typedef struct
{
    uint64_t value[STATE_LEN];
} Hash;

typedef struct
{
    int index;
    int index_dup;
    Hash hash;
} SortedHash;

namespace HASH_UTIL
{
    int is_valid_digit(const char c);
    char hexToNibble(char n);
    void hex_to_bytes(uint8_t *to, const char *from, int len);
    int compare_hash_binary(const uint64_t *a, const uint64_t *b);
    bool compare_hash(SortedHash &a, SortedHash &b);
    std::vector<std::string> split(const std::string& s, char delimiter);
}