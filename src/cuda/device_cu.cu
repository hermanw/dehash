#include <iostream>
#include <vector>
#include "device_cu.h"

void DeviceCu::enum_cu_devices(std::vector<Device*> &list)
{
    DeviceCu *device = new DeviceCu();
    list.push_back(device);
}

void DeviceCu::init(const char *options)
{
}

DeviceCu::~DeviceCu()
{
}

void DeviceCu::create_buffers(void* p_hash, void* p_number, void* p_helper, int hash_length, int data_length,int helper_length)
{
}

void DeviceCu::submit(void *input, int hash_length, int data_length)
{
   
}

int DeviceCu::run(size_t kernel_work_size[3])
{
    return 0;
}

void DeviceCu::read_results(void* p_data, int length)
{
}