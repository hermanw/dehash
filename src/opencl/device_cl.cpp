#include <iostream>
#include <vector>
#include "device_cl.h"
#include "compute_cl.h"

void _CheckCLError(cl_int error, int line)
{
    if (error != CL_SUCCESS)
    {
        std::cout << "OpenCL call failed with error " << error << " @" << line << std::endl;
        exit(1);
    }
}
#define CheckCLError(error) _CheckCLError(error, __LINE__);

void DeviceCl::enum_cl_devices(std::vector<Device*> &list)
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

void DeviceCl::init(const char *options)
{
    const cl_context_properties contextProperties[] =
        {
            CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(platform_id),
            0, 0};
    cl_uint num_devices = 0;
    clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_ALL, 0, nullptr,
                    &num_devices);
    cl_device_id *devices = new cl_device_id[num_devices];
    clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_ALL, num_devices,
                    devices, nullptr);

    cl_int error = CL_SUCCESS;
    context = clCreateContext(contextProperties, num_devices,
                              devices, nullptr, nullptr, &error);
    CheckCLError(error);

    size_t lengths[1] = {strlen(compute_cl)};
    const char *sources[1] = {compute_cl};
    program = clCreateProgramWithSource(context, 1, sources, lengths, &error);
    CheckCLError(error);
    // program = CreateProgram("kernels/compute.cl", context);

    CheckCLError(clBuildProgram(program, num_devices, devices, options, nullptr, nullptr));

    kernel = clCreateKernel(program, "compute", &error);
    CheckCLError(error);
#if CL_VERSION_2_0
    queue = clCreateCommandQueueWithProperties(context, device_id,
                                               0, &error);
#else
    queue = clCreateCommandQueue(context, device_id,
                                 0, &error);
#endif
    CheckCLError(error);

    delete[] devices;
}

DeviceCl::~DeviceCl()
{
    if(hash_buffer)clReleaseMemObject(hash_buffer);
    if(data_buffer)clReleaseMemObject(data_buffer);
    if(number_buffer)clReleaseMemObject(number_buffer);
    if(helper_buffer)clReleaseMemObject(helper_buffer);
    if(count_buffer)clReleaseMemObject(count_buffer);
    if(input_buffer)clReleaseMemObject(input_buffer);

    if(queue)clReleaseCommandQueue(queue);
    if(kernel)clReleaseKernel(kernel);
    if(program)clReleaseProgram(program);
    if(context)clReleaseContext(context);
}

void DeviceCl::create_buffers(void* p_hash, void* p_number, void* p_helper, int hash_length, int data_length,int helper_length)
{
    cl_int error = CL_SUCCESS;
    int count = 0;
    count_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(int), &count, &error);
    CheckCLError(error);
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &count_buffer);
    data_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, hash_length * data_length, 0, &error);
    CheckCLError(error);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &data_buffer);
    hash_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, hash_length * 16, p_hash, &error);
    CheckCLError(error);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &hash_buffer);
    number_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 40000, p_number, &error);
    CheckCLError(error);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &number_buffer);
    helper_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, helper_length, p_helper, &error);
    CheckCLError(error);
    clSetKernelArg(kernel, 4, sizeof(cl_mem), &helper_buffer);
    input_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY, data_length, 0, &error);
    CheckCLError(error);
    clSetKernelArg(kernel, 5, sizeof(cl_mem), &input_buffer);
}

void DeviceCl::submit(void *input, int hash_length, int data_length)
{
    clSetKernelArg(kernel, 6, sizeof(int), &hash_length);
    CheckCLError(clEnqueueWriteBuffer(queue, input_buffer, CL_TRUE, 0,
                                        data_length,
                                        input,
                                        0, nullptr, nullptr));

}

int DeviceCl::run(size_t kernel_work_size[3])
{
    CheckCLError(clEnqueueNDRangeKernel(queue, kernel, 3,
                                        nullptr,
                                        kernel_work_size,
                                        nullptr,
                                        0, nullptr, nullptr));

    int count = 0;
    CheckCLError(clEnqueueReadBuffer(queue, count_buffer, CL_TRUE, 0,
                                        sizeof(int),
                                        &count,
                                        0, nullptr, nullptr));
    return count;
}

void DeviceCl::read_results(void* p_data, int length)
{
    CheckCLError(clEnqueueReadBuffer(queue, data_buffer, CL_TRUE, 0,
                                     length,
                                     p_data,
                                     0, nullptr, nullptr));
}