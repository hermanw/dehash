#pragma once

#include <vector>
#include <string>
#include "cfg.h"

enum DEVICE_TYPE
{
    DT_OPENCL,
    DT_CUDA
};

class Device
{
public:
    Device() {};
    virtual ~Device(){};
    virtual void init(Cfg &cfg) = 0;
    virtual void create_buffers(
                    int input_buffer_size,
                    void *p_hash, int hash_buffer_size, int hash_length,
                    void *p_number, int number_buffer_size,
                    void *p_helper, int helper_buffer_size) = 0;
    virtual void submit(void *p_input, int input_buffer_size, int hash_buffer_size) = 0;
    virtual int run(size_t kernel_work_size[3]) = 0;
    virtual void read_results(void* p_output, int length) = 0;

public:
    DEVICE_TYPE dt;
    std::string info;
};