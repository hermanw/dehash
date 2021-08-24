#include <iostream>
#include "device_pool.h"

DevicePool::DevicePool()
{
    Device::enum_devices(m_devices_list);
}

DevicePool::~DevicePool()
{
    for (auto device : m_devices_list)
    {
        delete device;
    }
    m_devices_list.clear();
}

Device* DevicePool::get_device(int index)
{
    return m_devices_list[index];
}

void DevicePool::print_info()
{
    int index = 0;
    for (auto device : m_devices_list)
    {
        std::cout << index++ << ". " << device->info << std::endl;
    }
}