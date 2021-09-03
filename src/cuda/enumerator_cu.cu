#include "enumerator_cu.h"
#include "device_cu.h"

void EnumeratorCu::enum_devices(std::vector<Device*> &list)
{
    int count = 0;
    if (cudaGetDeviceCount(&count))
    {
        return;
    }

    for (int i = 0; i< count; i++)
    {
        cudaDeviceProp deviceProps;
        if (cudaGetDeviceProperties(&deviceProps, i) == cudaSuccess)
        {
            DeviceCu *device = new DeviceCu();
            device->m_device = i;
            device->info += "name:";
            device->info += deviceProps.name;
            list.push_back(device);
        }
    }
}