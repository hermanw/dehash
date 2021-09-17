#include <iostream>
#include <algorithm>
#include <cmath>
#include <chrono>
#include "decoder.h"
#include "device_pool.h"
#include "thread_profile.hpp"

Decoder::Decoder(Cfg &cfg)
:m_cfg(cfg)
,m_benchmark(false)
{
    m_thread_q = new ThreadQueue(1);
    m_p_input = 0;
    m_p_output = 0;
    m_p_number = 0;
    m_p_helper = 0;
}

Decoder::~Decoder()
{
    delete m_thread_q;
    if(m_p_input) delete[] m_p_input;
    if(m_p_output) delete[] m_p_output;
    if(m_p_number) delete[] m_p_number;
    if(m_p_helper) delete[] m_p_helper;
    for(auto &result : m_results)
    {
        delete result;
    }
    m_results.clear();
}


void Decoder::update_hash(const char *a_hash_string, int index)
{
    m_hash_string.push_back(std::string(a_hash_string));
    SortedHash sh;
    sh.index = index;
    HASH_UTIL::hex_to_bytes((uint8_t *)(sh.hash.value), a_hash_string, HASH_LEN);
    m_hash.push_back(sh);
}

void Decoder::parse_hash_string(const char *s)
{
    int valid_digit_num = 0;
    int count = 0;
    char a_hash_string[HASH_LEN + 1] = {0};
    while (*s)
    {
        if (*s == ',')
        {
            if (valid_digit_num == HASH_LEN)
            {
                update_hash(a_hash_string, count);
                count++;
            }
            valid_digit_num = 0;
        }
        else if (HASH_UTIL::is_valid_digit(*s))
        {
            a_hash_string[valid_digit_num] = *s;
            valid_digit_num++;
        }
        s++;
    }
    if (valid_digit_num == HASH_LEN)
    {
        update_hash(a_hash_string, count);
        count++;
    }
    m_hash_len = count;
}

void Decoder::dedup_sorted_hash()
{
    m_dedup_len = m_hash_len;
    for (int i = 1; i < m_dedup_len; i++)
    {
        if (HASH_UTIL::compare_hash_binary(m_hash[i].hash.value, m_hash[i - 1].hash.value) == 0)
        {
            SortedHash temp = m_hash[i];
            temp.index_dup = m_hash[i - 1].index;
            for (int j = i; j < m_hash_len - 1; j++)
            {
                m_hash[j] = m_hash[j + 1];
            }
            m_hash[m_hash_len - 1] = temp;
            m_dedup_len--;
            i--;
        }
    }
}

void Decoder::update_result()
{
    m_data.resize(m_hash_len);
    for (int i = 0; i < m_hash_len; i++)
    {
        int index = m_hash[i].index;
        if (i < m_dedup_len)
        {
            for(auto result : m_results)
            {
                if (result[i*m_input_size] != 0)
                {
                    for (int j = 0; j < m_input_size; j++)
                    {
                        m_data[index] += result[i*m_input_size + j];
                    }
                }
            }
        }
        else
        {
            int index_dup = m_hash[i].index_dup;
            m_data[index] = m_data[index_dup];
        }
    }
}

std::string Decoder::get_result()
{
    std::string result;
    for (int h = 0; h < m_hash_len; h++)
    {
        result += m_hash_string[h];
        result += ',';
        result += m_data[h];
        result += '\n';
    }
    return result;
}

void Decoder::set_hash_string(const char *s)
{
    parse_hash_string(s);
    std::sort(m_hash.begin(), m_hash.end(), HASH_UTIL::compare_hash);
    dedup_sorted_hash();

    //// build host buffers
    // build input buffer
    m_input_size = m_cfg.length;
    m_p_input = new uint8_t[m_input_size]();

    // build output buffer
    m_output_size = m_cfg.kernel_work_size[0] * m_cfg.kernel_work_size[1] * m_cfg.kernel_work_size[2];
    m_p_output = new Hash[m_output_size];
    // build number buffer
    m_number_size = 10000 * 4;
    m_p_number = new char[m_number_size];
    for (size_t i = 0; i < 10000; i++)
    {
        m_p_number[i * 4 + 0] = i / 1000 + '0';
        m_p_number[i * 4 + 1] = i / 100 % 10 + '0';
        m_p_number[i * 4 + 2] = i / 10 % 10 + '0';
        m_p_number[i * 4 + 3] = i % 10 + '0';
    }
    // build hepler buffer - assume at most one gpu list section for now
    int listds_length = 0;
    std::string listds_source;
    for(auto& ds : m_cfg.gpu_sections)
    {
        if(ds.type == ds_type_list)
        {
            listds_length = ds.length;
            listds_source = ds.source;
            break;
        }
    }
    m_helper_size = listds_length * m_cfg.sources[listds_source].size();
    if (m_helper_size > 0)
    {
        m_p_helper = new char[m_helper_size];
        long offset = 0;
        for(auto& s : m_cfg.sources[listds_source])
        {
            for(int i = 0; i < listds_length; i++)
            {
                m_p_helper[offset++] = s[i];
            }
        }
    }
    else
    {
        // not used
        m_helper_size = 1;
        m_p_helper = new char[m_helper_size];
    }
}

bool Decoder::process_inputs(int section)
{
    int ds_size = m_cfg.cpu_sections.size();
    // done
    if (section >= ds_size)
    {
        TP_END("[main thread] working");
        TP_BEGIN("[main thread] wait");
        m_thread_q->add();
        TP_END("[main thread] wait");
        TP_BEGIN("[main thread] working");
        return m_done;
    }

    // fill input data and go to next section
    auto &ds = m_cfg.cpu_sections[section];
    switch (ds.type)
    {
        case ds_type_list:
        {
            auto &source = m_cfg.sources[ds.source];
            for(auto &s : source)
            {
                for (int i = 0; i < s.size(); i++)
                {
                    m_p_input[i + ds.index] = s[i];
                }
                if(process_inputs(section + 1))
                {
                    return true;
                }
            }
        }
        break;

        case ds_type_digit:
        {
            size_t digits = pow(10, ds.length);
            for( int digit = 0; digit < digits; digit++ )
            {
                int d = digit;
                for( int offset = ds.length-1; offset >= 0; offset--)
                {
                    m_p_input[ds.index + offset] = '0' + d % 10;
                    d /= 10;
                }
                if(process_inputs(section + 1))
                {
                    return true;
                }
            }
        }
        break;

        default:
        std::cerr << "ERROR: not supported cpu section type" << std::endl;
        break;
    }

    return false;
}

void Decoder::compute_thread_f(int thread_id, Device *device)
{
    while(1)
    {
        TP_BEGIN("[compute thread " + std::to_string(thread_id) + "] wait");
        m_thread_q->remove(device, [](Device *device){
            device->submit_input();
        });
        TP_END("[compute thread " + std::to_string(thread_id) + "] wait");

        if (m_done)
        {
            return;
        }

        TP_BEGIN("[compute thread " + std::to_string(thread_id) + "] compute");
        auto start = std::chrono::steady_clock::now();
        device->run(m_p_output, m_output_size*sizeof(Hash));
        if (m_benchmark)
        {
            auto end = std::chrono::steady_clock::now();
            auto e = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            auto score = m_output_size/1000/e;
            if (m_kernel_score < score)
            {
                m_kernel_score = score;
            }
        }
        TP_END("[compute thread " + std::to_string(thread_id) + "] compute");
    }
}

void Decoder::print_progress()
{
    // std::cout << " find " << total_decoded() << "/" << m_dedup_len << " @" << time(NULL) - m_start << "s\n" ;
}

std::string Decoder::decode(const std::string &devices)
{
    // start compute threads for each device
    std::vector<std::thread *> gpu_threads;
    if (!m_benchmark)
    {
        std::cout << "using compute devices:" << std::endl;
    }
    m_done = false;

    auto dp = new DevicePool();
    std::vector<std::string> list = HASH_UTIL::split(devices, ',');
    for (int i = 0; i < list.size(); i++)
    {
        auto device = dp->get_device(std::stoi(list[i]));
        device->init(m_cfg);
        device->create_buffers(
            m_p_input, m_input_size,
            m_p_number, m_number_size,
            m_p_helper, m_helper_size,
            m_output_size);
        gpu_threads.push_back(new std::thread(&Decoder::compute_thread_f, this, i, device));
        if (!m_benchmark)
        {
            std::cout << "  " << list[i] << ". " << device->info << std::endl;
        }
    }

    // start search threads

        // // alloc result buffer
        // char *p_data = new char[m_dedup_len * m_input_size]();
        // m_results.push_back(p_data);

    m_start = time(NULL);
    m_iterations = 0;
    m_iterations_len = 1;
    for(auto &ds : m_cfg.cpu_sections)
    {
        m_iterations_len *= m_cfg.sources[ds.source].size();
    }
    if (!m_benchmark) std::cout << std::endl;
    TP_BEGIN("[main thread] work");
    process_inputs(0);
    TP_END("[main thread] work");

    // notify all compute threads done
    m_done = true;
    for (int i = 0; i < list.size(); i++)
    {
        m_thread_q->add();
    }
    for (auto& thread : gpu_threads)
    {
        if(thread->joinable())thread->join();
        delete thread;
    }
    gpu_threads.clear();
    // notify all search threads done

    delete dp;

    if (!m_benchmark)
    {
        print_progress();
    }

    update_result();
    return get_result();
}

void Decoder::benchmark()
{
    m_benchmark = true;

    auto dp = new DevicePool();
    int count = dp->get_device_count();
    for (int i = 0; i < count; ++i)
    {
        if(i==0) continue;
        std::cout << i << ". " << dp->get_device(i)->info << std::endl;
        m_kernel_score = 0;
        decode(std::to_string(i));
        std::cout << "   perf: " << m_kernel_score << "MH/s" << std::endl;
    }
    delete dp;

    PRINT_TP_DATA();
}
