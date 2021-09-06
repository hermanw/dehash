#include <iostream>
#include <sstream>
#include <vector>
#include <cstring>
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


void DeviceCl::init(Cfg &cfg)
{
    m_cfg = &cfg;
    // build options string
    std::ostringstream options;
    options << "-D DEVICE_FUNC_PREFIX=static";
    options << " -D DATA_LENGTH=" << cfg.length;
    std::vector<std::string> XYZ = {"X", "Y", "Z"};
    for(int i = 0; i < cfg.gpu_sections.size(); i++)
    {
        auto &ds = cfg.gpu_sections[i];
        options << " -D " << XYZ[i] << "_INDEX=" << ds.index;
        options << " -D " << XYZ[i] << "_LENGTH=" << ds.length;
        options << " -D " << XYZ[i] << "_TYPE=" << ds.type;
    }

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

    CheckCLError(clBuildProgram(program, num_devices, devices, options.str().c_str(), nullptr, nullptr));

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
    if(count_buffer)clReleaseMemObject(count_buffer);
    if(input_buffer)clReleaseMemObject(input_buffer);
    if(output_buffer)clReleaseMemObject(output_buffer);
    if(hash_buffer)clReleaseMemObject(hash_buffer);
    if(number_buffer)clReleaseMemObject(number_buffer);
    if(helper_buffer)clReleaseMemObject(helper_buffer);

    if(queue)clReleaseCommandQueue(queue);
    if(kernel)clReleaseKernel(kernel);
    if(program)clReleaseProgram(program);
    if(context)clReleaseContext(context);
}

void DeviceCl::create_buffers(
    int input_buffer_size,
    void *p_hash, int hash_buffer_size, int hash_length,
    void *p_number, int number_buffer_size,
    void *p_helper, int helper_buffer_size)
{
    cl_int error = CL_SUCCESS;
    
    int count = 0;
    count_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(int), &count, &error);
    CheckCLError(error);
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &count_buffer);

    input_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY, input_buffer_size, 0, &error);
    CheckCLError(error);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &input_buffer);

    output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, hash_buffer_size * input_buffer_size, 0, &error);
    CheckCLError(error);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &output_buffer);

    hash_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, hash_buffer_size * hash_length, p_hash, &error);
    CheckCLError(error);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &hash_buffer);

    number_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, number_buffer_size, p_number, &error);
    CheckCLError(error);
    clSetKernelArg(kernel, 4, sizeof(cl_mem), &number_buffer);

    helper_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, helper_buffer_size, p_helper, &error);
    CheckCLError(error);
    clSetKernelArg(kernel, 5, sizeof(cl_mem), &helper_buffer);
}

void DeviceCl::submit(void *p_input, int input_buffer_size, int hash_buffer_size)
{
    clSetKernelArg(kernel, 6, sizeof(int), &hash_buffer_size);
    CheckCLError(clEnqueueWriteBuffer(queue, input_buffer, CL_TRUE, 0,
                                        input_buffer_size,
                                        p_input,
                                        0, nullptr, nullptr));

}

int DeviceCl::run()
{
    CheckCLError(clEnqueueNDRangeKernel(queue, kernel, 3,
                                        nullptr,
                                        m_cfg->kernel_work_size,
                                        nullptr,
                                        0, nullptr, nullptr));

    int count = 0;
    CheckCLError(clEnqueueReadBuffer(queue, count_buffer, CL_TRUE, 0,
                                        sizeof(int),
                                        &count,
                                        0, nullptr, nullptr));
    return count;
}

void DeviceCl::read_results(void* p_output, int length)
{
    CheckCLError(clEnqueueReadBuffer(queue, output_buffer, CL_TRUE, 0,
                                     length,
                                     p_output,
                                     0, nullptr, nullptr));
}