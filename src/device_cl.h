#pragma once

#include "device.h"

#define CL_TARGET_OPENCL_VERSION 200

#ifdef __APPLE__
#include "OpenCL/opencl.h"
#else
#include "CL/cl.h"
#endif

class DeviceCl : public Device
{
public:
    DeviceCl() {dt = DT_OPENCL; info = "type:OpenCL, ";}
    virtual ~DeviceCl();
    virtual void init(const char *options);
    virtual void create_buffers(void* p_hash, void* p_number, void* p_helper, int hash_length, int data_length,int helper_length);
    virtual void submit(void *input, int hash_length, int data_length);
    virtual int run();
    // virtual void get_result();
    static void enum_cl_devices(std::vector<Device*> &list);

public:
    cl_platform_id platform_id;
    cl_device_id device_id;

private:
    cl_context context = 0;
    cl_program program = 0;
    cl_kernel kernel = 0;
    cl_command_queue queue = 0;
    cl_mem hash_buffer = 0;
    cl_mem data_buffer = 0;
    cl_mem number_buffer = 0;
    cl_mem helper_buffer = 0;
    cl_mem count_buffer = 0;
    cl_mem input_buffer = 0;
};