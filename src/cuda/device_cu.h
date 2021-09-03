#pragma once

#include "device.h"
#include "cfg.h"

class DeviceCu : public Device
{
public:
    DeviceCu() {dt = DT_CUDA; info = "type:CUDA, ";}
    virtual ~DeviceCu();
    virtual void init(Cfg &cfg);
    virtual void create_buffers(
                    int input_buffer_size,
                    void *p_hash, int hash_buffer_size, int hash_length,
                    void *p_number, int number_buffer_size,
                    void *p_helper, int helper_buffer_size);
    virtual void submit(void *p_input, int input_buffer_size, int hash_buffer_size);
    virtual int run(size_t kernel_work_size[3]);
    virtual void read_results(void* p_output, int length);

public:
    int m_device;

private:
    Cfg *m_cfg;
    int m_hash_buffer_size;
    void *d_count_buffer = 0;
    void *d_input_buffer = 0;
    void *d_output_buffer = 0;
    void *d_hash_buffer = 0;
    void *d_number_buffer = 0;
    void *d_helper_buffer = 0;
};