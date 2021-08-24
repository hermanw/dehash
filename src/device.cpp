#include "device.h"
#include "device_cl.h"

void Device::enum_devices(std::vector<Device*> &list)
{
    DeviceCl::enum_cl_devices(list);
}