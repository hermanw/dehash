#ifndef __DECODER_H
#define __DECODER_H

#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include "device.h"
#include "cfg.h"
#include "hash_util.h"
#include "thread_queue.hpp"

class Decoder
{
public:
    Decoder(Cfg &cfg);
    ~Decoder();
    void set_hash_string(const char *s);
    int get_hash_len() const { return m_hash_len; }
    int get_dedup_len() const { return m_dedup_len; }
    std::string get_result();
    std::string decode(const std::string &devices);
    void benchmark();

private:
    void update_hash(const char *hash_string, int index);
    void parse_hash_string(const char *s);
    void dedup_sorted_hash();
    bool process_inputs(int section);
    void compute_thread_f(int thread_id, Device *device);
    void search_thread_f();
    void search_hash();
    void update_result();
    void print_progress();

private:
    Cfg &m_cfg;
    int m_hash_len;
    int m_dedup_len;
    std::vector<std::string> m_hash_string;
    std::vector<SortedHash> m_hash;
    std::vector<std::string> m_data;
    // for progress
    time_t m_start;
    int m_iterations;
    int m_iterations_len;
    // for benchmark
    bool m_benchmark;
    long m_kernel_score;
    // for gpu threads
    ThreadQueue *m_thread_q;
    bool m_done;
    std::vector<char*> m_results;
    // for cpu threads
    std::vector<std::thread*> m_cpu_threads;
    // buffers
    uint8_t *m_p_input;
    int m_input_size;
    Hash *m_p_output;
    int m_output_size;
    char *m_p_number;
    int m_number_size;
    char *m_p_helper;
    int m_helper_size;
};

#endif