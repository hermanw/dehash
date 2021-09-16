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
                    void *p_input, int input_buffer_size,
                    void *p_number, int number_buffer_size,
                    void *p_helper, int helper_buffer_size,
                    int output_buffer_size) = 0;
    virtual void submit_input() = 0;
    virtual void run(void* p_output, int length) = 0;

public:
    DEVICE_TYPE dt;
    std::string info;

protected:
    void *m_input;
    int m_input_size;
};