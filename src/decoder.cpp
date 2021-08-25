#include <iostream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <mutex>
#include "decoder.h"
#include "device_pool.h"

Decoder::Decoder(Cfg &cfg)
:m_cfg(cfg)
,m_benchmark(false)
{
}

Decoder::~Decoder()
{
}

int Decoder::is_valid_digit(const char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
char Decoder::hexToNibble(char n)
{
    if (n >= 'a' && n <= 'f')
    {
        return n - 'a' + 10;
    }
    else if (n >= 'A' && n <= 'F')
    {
        return n - 'A' + 10;
    }
    else
    {
        return n - '0';
    }
}
void Decoder::hex_to_bytes(uint8_t *to, const char *from, int len)
{
    for (int i = 0; i < len / 2; i++)
    {
        to[i] = (hexToNibble(from[i * 2]) << 4) + hexToNibble(from[i * 2 + 1]);
    }
}

void Decoder::update_hash(const char *a_hash_string, int index)
{
    m_hash_string.push_back(std::string(a_hash_string));
    SortedHash sh;
    sh.index = index;
    hex_to_bytes((uint8_t *)(sh.hash.value), a_hash_string, HASH_LEN);
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
        else if (is_valid_digit(*s))
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

int Decoder::compare_hash_binary(const uint64_t *a, const uint64_t *b)
{
    for (int i = 0; i < STATE_LEN; i++)
    {
        if (a[i] < b[i])
        {
            return -1;
        }
        else if (a[i] > b[i])
        {
            return 1;
        }
    }

    return 0;
}

bool Decoder::compare_hash(SortedHash &a, SortedHash &b)
{
    return compare_hash_binary(a.hash.value, b.hash.value) < 0;
}

void Decoder::dedup_sorted_hash()
{
    m_dedup_len = m_hash_len;
    for (int i = 1; i < m_dedup_len; i++)
    {
        if (compare_hash_binary(m_hash[i].hash.value, m_hash[i - 1].hash.value) == 0)
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

void Decoder::update_result(char *p_data, int data_length)
{
    m_data.resize(m_hash_len);
    for (int i = 0; i < m_hash_len; i++)
    {
        int index = m_hash[i].index;
        if (i < m_dedup_len)
        {
            for (int j = 0; j < data_length; j++)
            {
                m_data[index] += p_data[i*data_length + j];
            }
        }
        else
        {
            int index_dup = m_hash[i].index_dup;
            m_data[index] = m_data[index_dup];
        }
    }
}

void Decoder::get_result(std::string& result)
{
    for (int h = 0; h < m_hash_len; h++)
    {
        result += m_hash_string[h];
        result += ',';
        result += m_data[h];
        result += '\n';
    }
}

void Decoder::set_hash_string(const char *s)
{
    parse_hash_string(s);
    std::sort(m_hash.begin(), m_hash.end(), compare_hash);
    dedup_sorted_hash();

    // build options string
    data_length = m_cfg.length;
    m_options << "-D DATA_LENGTH=" << data_length;
    std::vector<std::string> XYZ = {"X", "Y", "Z"};
    for(int i = 0; i < m_cfg.gpu_sections.size(); i++)
    {
        auto &ds = m_cfg.gpu_sections[i];
        m_options << " -D " << XYZ[i] << "_INDEX=" << ds.index;
        m_options << " -D " << XYZ[i] << "_LENGTH=" << ds.length;
        m_options << " -D " << XYZ[i] << "_TYPE=" << ds.type;
    }

    //// build host buffers
    // build hash buffer
    m_p_hash = new Hash[m_dedup_len];
    for (int i = 0; i < m_dedup_len; i++)
    {
        m_p_hash[i] = m_hash[i].hash;
    }
    // build number buffer
    long numbers_length = 10000 * 4;
    m_p_number = new char[numbers_length];
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
    long list_length = listds_length * m_cfg.sources[listds_source].size();
    char* m_p_helper = new char[list_length];
    helper_length = list_length;
    if (list_length > 0)
    {
        long offset = 0;
        for(auto& s : m_cfg.sources[listds_source])
        {
            for(int i = 0; i < listds_length; i++)
            {
                m_p_helper[offset++] = s[i];
            }
        }
    }
}

bool Decoder::run_in_host(int index)
{
    int ds_size = m_cfg.cpu_sections.size();
    // done
    if (index >= ds_size)
    {
        return run_in_kernel();
    }
    // fill host data and go to next section
    auto &ds = m_cfg.cpu_sections[index];
    auto &source = m_cfg.sources[ds.source];
    for(auto &s : source)
    {
        for (int i = 0; i < s.size(); i++)
        {
            m_input[i + ds.index] = s[i];
        }
        if(run_in_host(index + 1))
        {
            return true;
        }
    }

    return false;
}

bool Decoder::run_in_kernel()
{
    m_input_ready = true;
    bool ready = false;
    do
    {
        m_mtx.lock();
        ready = m_input_ready;
        m_mtx.unlock();
    } while (ready);

    // int count = kernel->run(input, m_dedup_len, data_length, m_cfg.kernel_work_size);

    // m_iterations++;
    // std::cout << "\033[1A" << count << "/" << m_dedup_len << " @" << time(NULL) - m_start << "s - searching " ;
    // int data_length = m_cfg.length;
    // for(int i = 0; i < data_length; i++)
    // {
    //     char c = input[i];
    //     std::cout << (c ? c : '*');
    // }
    // std::cout << ", " << m_iterations * 100 / m_iterations_len << "%\n";

    // return (count == m_dedup_len);
    return false;
}

void Decoder::thread_function(Device *device, std::mutex *mutex)
{
    std::lock_guard<std::mutex> guard(*mutex);

    device->init(m_options.str().c_str());
    device->create_buffers(m_p_hash, m_p_number, m_p_helper, hash_length, data_length, helper_length);

    while(1)
    {
        m_mtx.lock();
        if (m_done)
        {
            m_mtx.unlock();
            return;
        }
        else if (m_input_ready)
        {
            auto start = std::chrono::steady_clock::now();
            std::cout << m_input;
            device->submit(m_input, hash_length, data_length);
            m_input_ready = false;
            m_mtx.unlock();
            int count = device->run(m_cfg.kernel_work_size);
            std::cout << ":" << count << std::endl;
            if (m_benchmark)
            {
                auto end = std::chrono::steady_clock::now();
                auto e = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                if (m_kernel_score == 0 || m_kernel_score > e)
                {
                    m_kernel_score = e;
                }
            }
        }
        else
        {
            m_mtx.unlock();
        }        
    }
}

std::vector<std::string> split(const std::string& s, char delimiter)
{
   std::vector<std::string> tokens;
   std::string token;
   std::istringstream tokenStream(s);
   while (std::getline(tokenStream, token, delimiter))
   {
      tokens.push_back(token);
   }
   return tokens;
}

void Decoder::decode(const std::string &devices)
{
    // start threads for each device
    m_input = new uint8_t[data_length]();
    m_input_ready = false;
    m_done = false;

    std::vector<std::mutex*> thread_mutex;
    auto dp = new DevicePool();
    std::vector<std::string> list = split(devices, ',');
    for(auto &index :list)
    {
        auto mutex = new std::mutex();
        thread_mutex.push_back(mutex);
        auto device = dp->get_device(std::stoi(index));
        std::thread t(&Decoder::thread_function, this, device, mutex);
        t.detach();       
    }

    m_start = time(NULL);
    m_iterations = 0;
    m_iterations_len = 1;
    for(auto &ds : m_cfg.cpu_sections)
    {
        m_iterations_len *= m_cfg.sources[ds.source].size();
    }
    run_in_host(0);
    m_mtx.lock();
    m_done = true;
    m_mtx.unlock();
    for(auto mutex : thread_mutex)
    {
        mutex->lock();
        delete mutex;
    }

    delete dp;
    delete[] m_input;


    // read results
    // int result_length = m_dedup_len * data_length;
    // char *p_data = new char[result_length];
    // kernel->read_results(p_data, result_length);
    // update_result(p_data, data_length);
    // delete[] p_data;
}

void Decoder::benchmark()
{
    m_benchmark = true;

    auto dp = new DevicePool();
    int count = dp->get_device_count();
    for (int i = 0; i < count; ++i)
    {
        std::cout << i << ". " << dp->get_device(i)->info << std::endl;
        m_kernel_score = 0;
        decode(std::to_string(i));
        std::cout << "----score: " << m_kernel_score << std::endl;
    }
    delete dp;
}
