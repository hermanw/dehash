#pragma once

#include "device.h"

class DevicePool
{
public:
    DevicePool();
    ~DevicePool();
    Device* get_device(int index);
    int get_device_count();
    void print_info();

private:
    std::vector<Device*> m_devices_list;
};