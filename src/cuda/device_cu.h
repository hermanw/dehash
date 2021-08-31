#pragma once

#include "device.h"

class DeviceCu : public Device
{
public:
    DeviceCu() {dt = DT_CUDA; info = "type:CUDA, ";}
    virtual ~DeviceCu();
    virtual void init(const char *options);
    virtual void create_buffers(void* p_hash, void* p_number, void* p_helper, int hash_length, int data_length,int helper_length);
    virtual void submit(void *input, int hash_length, int data_length);
    virtual int run(size_t kernel_work_size[3]);
    virtual void read_results(void* p_data, int length);
    static void enum_cu_devices(std::vector<Device*> &list);

public:

private:
};