#include "enumerator_cl.h"
#include "device_cl.h"

void EnumeratorCl::enum_devices(std::vector<Device*> &list)
{
    cl_uint num_platforms = 0;
    clGetPlatformIDs(0, nullptr, &num_platforms);

    if (num_platforms == 0)
    {
        // std::cout << "No OpenCL platform found" << std::endl;
        return;
    }

    cl_platform_id *platforms = new cl_platform_id[num_platforms];
    clGetPlatformIDs(num_platforms, platforms, nullptr);

    for (size_t i = 0; i < num_platforms; i++)
    {
        // enum devices
        cl_uint num_devices = 0;
        clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, nullptr,
                       &num_devices);

        if (num_devices == 0)
        {
            continue;
        }

        // get platform name
        size_t size = 0;
        clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 0, nullptr, &size);
        auto platform_name = new char[size];
        clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, size, platform_name, nullptr);

        cl_device_id *devices = new cl_device_id[num_devices];
        clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, num_devices,
                       devices, nullptr);

        for (size_t j = 0; j < num_devices; j++)
        {
            DeviceCl *device = new DeviceCl();
            device->platform_id = platforms[i];
            device->device_id = devices[j];
            // device name
            size_t size = 0;
            clGetDeviceInfo(devices[j], CL_DEVICE_NAME, 0, nullptr, &size);
            auto device_name = new char[size];
            clGetDeviceInfo(devices[j], CL_DEVICE_NAME, size, device_name, nullptr);
            device->info += "platform:";
            device->info += platform_name;
            device->info += ", device:";
            device->info += device_name;
            delete[] device_name;
            //
            list.push_back(device);
        }
        delete[] devices;
        delete[] platform_name;
    }
    delete[] platforms;
}