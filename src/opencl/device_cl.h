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
    virtual void init(Cfg &cfg);
    virtual void create_buffers(
                    void *p_input, int input_buffer_size,
                    void *p_number, int number_buffer_size,
                    void *p_helper, int helper_buffer_size,
                    int output_buffer_size);
    virtual void submit_input();
    virtual void run(void* p_output, int length);

public:
    cl_platform_id platform_id;
    cl_device_id device_id;

private:
    Cfg *m_cfg;
    cl_context context = 0;
    cl_program program = 0;
    cl_kernel kernel = 0;
    cl_command_queue queue = 0;
    cl_mem input_buffer = 0;
    cl_mem output_buffer = 0;
    cl_mem number_buffer = 0;
    cl_mem helper_buffer = 0;
};