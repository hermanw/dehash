#include <iostream>
#include "device_pool.h"
#ifdef USE_OPENCL
#include "opencl/enumerator_cl.h"
#endif
#ifdef USE_CUDA
#include "cuda/enumerator_cu.h"
#endif

DevicePool::DevicePool()
{
    enum_devices();
}

DevicePool::~DevicePool()
{
    for (auto device : m_devices_list)
    {
        delete device;
    }
    m_devices_list.clear();
}

Device *DevicePool::get_device(int index)
{
    return m_devices_list[index];
}

int DevicePool::get_device_count()
{
    return m_devices_list.size();
}

void DevicePool::print_info()
{
    int index = 0;
    for (auto device : m_devices_list)
    {
        std::cout << index++ << ". " << device->info << std::endl;
    }
}

void DevicePool::enum_devices()
{
#ifdef USE_OPENCL
    EnumeratorCl::enum_devices(m_devices_list);
#endif
#ifdef USE_CUDA
    EnumeratorCu::enum_devices(m_devices_list);
#endif
}