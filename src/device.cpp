#include "device.h"
#ifdef USE_OPENCL
#include "opencl/device_cl.h"
#endif
#ifdef USE_CUDA
#include "cuda/device_cu.h"
#endif

void Device::enum_devices(std::vector<Device*> &list)
{
#ifdef USE_OPENCL
    DeviceCl::enum_cl_devices(list);
#endif
#ifdef USE_CUDA
    DeviceCu::enum_cu_devices(list);
#endif
}