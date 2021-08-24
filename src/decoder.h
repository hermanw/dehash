#ifndef __DECODER_H
#define __DECODER_H

#include <vector>
#include <string>
#include "device.h"
#include "cfg.h"

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

class Decoder
{
public:
    Decoder(Cfg &cfg);
    ~Decoder();
    void set_hash_string(const char *s);
    int get_hash_len() const { return m_hash_len; }
    int get_dedup_len() const { return m_dedup_len; }
    void update_result(char *p_data, int data_length);
    void get_result(std::string &result);
    void decode(const std::string &devices);
    void benchmark(int &platform_index, int &device_index);

private:
    int is_valid_digit(const char c);
    char hexToNibble(char n);
    void hex_to_bytes(uint8_t *to, const char *from, int len);
    void update_hash(const char *hash_string, int index);
    void parse_hash_string(const char *s);
    static int compare_hash_binary(const uint64_t *a, const uint64_t *b);
    static bool compare_hash(SortedHash &a, SortedHash &b);
    void dedup_sorted_hash();
    bool run_in_host(int index);
    bool run_in_kernel();
    void thread_function(Device *device);

private:
    Cfg &m_cfg;
    int m_hash_len;
    int m_dedup_len;
    std::vector<std::string> m_hash_string;
    std::vector<SortedHash> m_hash;
    std::vector<std::string> m_data;
    time_t m_start;
    int m_iterations;
    int m_iterations_len;
    bool m_benchmark;
    long m_kernel_score;
    // for threads
    std::mutex m_mtx;
    uint8_t *m_input;
    bool m_input_ready;
    bool m_done;
    // buffers
    Hash *m_p_hash;
    char *m_p_number;
    char *m_p_helper;
    int hash_length;
    int data_length;
    int helper_length;
};

#endif