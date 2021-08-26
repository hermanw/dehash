#pragma once

#include <vector>
#include <string>

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
    virtual void init(const char *options) = 0;
    virtual void create_buffers(void* p_hash, void* p_number, void* p_helper, int hash_length, int data_length,int helper_length) = 0;
    virtual void submit(void *input, int hash_length, int data_length) = 0;
    virtual int run(size_t kernel_work_size[3]) = 0;
    virtual void read_results(void* p_data, int length) = 0;

    static void enum_devices(std::vector<Device*> &list);

public:
    DEVICE_TYPE dt;
    std::string info;
};