#include <iostream>
#include <vector>
#include <cuda_runtime.h>

#include "device_cu.h"

typedef unsigned char uchar;
typedef unsigned int uint;

#define DEVICE_FUNC_PREFIX __device__
#include "md5.inc"

void check(cudaError_t e, int const line)
{
    if (e)
    {
        printf("cudaError(%d) @%d\n", e, line);
        exit(e);
    }
}

#define check_error(val) check(val, __LINE__)

#define BLOCK_LEN 64 // In bytes
#define LENGTH_SIZE 8 // In bytes
#define THREAD_NUM 500

struct ULONG2
{
    unsigned long long x;
    unsigned long long y;
};

struct GpuSection
{
    ds_type type;
    int index;
    int length;
};

__global__ void compute(
    uint* p_count,
    const uchar* p_input,
    uchar* p_output,
    const ULONG2* p_hash,
    const uchar* p_number,
    const uchar* p_helper,
    int hash_len,
    int data_length,
    int gpu_section_size,
    GpuSection *gs
    )
{
    if(*p_count >= hash_len) return;

    // fill data
    uchar data[BLOCK_LEN]= {0};
    data[data_length] = 0x80;
    data[BLOCK_LEN - LENGTH_SIZE] = (uchar)(data_length << 3);
    memcpy(data, p_input, data_length);

    for (int i = 0; i < gpu_section_size; i++)
    {
        uint index = 0;
        // THIS IS A TRICKY: always break the 1st gpu section into blocks and threads
        if(i==0) index = blockIdx.x*THREAD_NUM + threadIdx.x;
        else if(i==1) index = blockIdx.y;
        else index = blockIdx.z;

        if(gs[i].type == ds_type_list)
        {
            uint offset = index * gs[i].length;
            memcpy(data+gs[i].index, p_helper+offset, gs[i].length);
        }
        else if(gs[i].type == ds_type_digit)
        {
            uint offset = (index << 2) + 4 - gs[i].length;
            memcpy(data+gs[i].index, p_number+offset, gs[i].length);
        }
        else if(gs[i].type == ds_type_idsum)
        {
            const uchar COE[17] = {7,9,10,5,8,4,2,1,6,3,7,9,10,5,8,4,2};
            const uchar C_SUM[11] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'X'};
            uint sum = 0;
            for (uint j = 0; j < 17; j++)
            {
                sum += (data[j] - '0') * COE[j];
            }
            uint r = sum % 11;
            r = 12 - r;
            r = r>10?r-11:r;
            data[gs[i].index] =C_SUM[r];
        }
    }


    // hash
    uint4 hash = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476};

    md5(&hash, data);

    ULONG2 bhash;
    bhash.x = (unsigned long long)hash.x; bhash.x += (unsigned long long)hash.y << 32;
    bhash.y = (unsigned long long)hash.z; bhash.y += (unsigned long long)hash.w << 32;
    int low = 0;
    int high = hash_len - 1;
    do
    {
        int mid = (low + high) / 2;
        ULONG2 ahash = p_hash[mid];
        if(ahash.x < bhash.x)
        {
            low = mid + 1;
        }
        else if (ahash.x > bhash.x)
        {
            high = mid - 1;
        }
        else
        {
            if(ahash.y < bhash.y) {low = mid + 1;}
            else if (ahash.y > bhash.y) {high = mid - 1;}
            else
            {
                atomicAdd(p_count, 1);
                int offset = mid * data_length;
                for (int j = 0; j < data_length; j++)
                {
                    p_output[offset + j] = data[j];
                }
                break;
            }
        }
    } while (low <= high);
}

void DeviceCu::init(Cfg &cfg)
{
    m_cfg = &cfg;
    check_error(cudaSetDevice(m_device));

    // kernel_work_size maps to 3 gpu sections
    // THIS IS A TRICKY: always break the 1st gpu section into blocks and threads
    numBlocks.x = m_cfg->kernel_work_size[0] / THREAD_NUM;
    numBlocks.y = m_cfg->kernel_work_size[1];
    numBlocks.z = m_cfg->kernel_work_size[2];
}

DeviceCu::~DeviceCu()
{
    cudaFree(d_count_buffer);
    cudaFree(d_input_buffer);
    cudaFree(d_output_buffer);
    cudaFree(d_hash_buffer);
    cudaFree(d_number_buffer);
    cudaFree(d_helper_buffer);
    cudaFree(d_gs);
}

void DeviceCu::create_buffers(
                    int input_buffer_size,
                    void *p_hash, int hash_buffer_size, int hash_length,
                    void *p_number, int number_buffer_size,
                    void *p_helper, int helper_buffer_size)
{
    int count = 0;
    check_error(cudaMalloc((void**)&d_count_buffer, sizeof(int)));
    check_error(cudaMemcpy(d_count_buffer, &count, sizeof(int), cudaMemcpyHostToDevice));

    check_error(cudaMalloc((void**)&d_input_buffer, input_buffer_size));

    check_error(cudaMalloc((void**)&d_output_buffer, hash_buffer_size * input_buffer_size));

    check_error(cudaMalloc((void**)&d_hash_buffer, hash_buffer_size * hash_length));
    check_error(cudaMemcpy(d_hash_buffer, p_hash, hash_buffer_size * hash_length, cudaMemcpyHostToDevice));

    check_error(cudaMalloc((void**)&d_number_buffer, number_buffer_size));
    check_error(cudaMemcpy(d_number_buffer, p_number, number_buffer_size, cudaMemcpyHostToDevice));

    check_error(cudaMalloc((void**)&d_helper_buffer, helper_buffer_size));
    check_error(cudaMemcpy(d_helper_buffer, p_helper, helper_buffer_size, cudaMemcpyHostToDevice));

    int gpu_section_size = m_cfg->gpu_sections.size();
    GpuSection gs[3];
    for(int i = 0; i < gpu_section_size; i++)
    {
        auto &ds = m_cfg->gpu_sections[i];
        gs[i].index = ds.index;
        gs[i].length = ds.length;
        gs[i].type = ds.type;
    }
    check_error(cudaMalloc((void**)&d_gs, sizeof(GpuSection)*3));
    check_error(cudaMemcpy(d_gs, gs, sizeof(GpuSection)*3, cudaMemcpyHostToDevice));
}

void DeviceCu::submit(void *p_input, int input_buffer_size, int hash_buffer_size)
{
    m_hash_buffer_size = hash_buffer_size;
    check_error(cudaMemcpy(d_input_buffer, p_input, input_buffer_size, cudaMemcpyHostToDevice));
    check_error(cudaDeviceSynchronize());
}

int DeviceCu::run()
{
    int data_length = m_cfg->length;
    int gpu_section_size = m_cfg->gpu_sections.size();
    compute<<<numBlocks, THREAD_NUM>>>((uint *)d_count_buffer,
                                       (const uchar *)d_input_buffer,
                                       (uchar *)d_output_buffer,
                                       (const ULONG2 *)d_hash_buffer,
                                       (const uchar *)d_number_buffer,
                                       (const uchar *)d_helper_buffer,
                                       m_hash_buffer_size, data_length, gpu_section_size, (GpuSection *)d_gs);

    int count = 0;
    check_error(cudaMemcpy(&count, d_count_buffer, sizeof(int), cudaMemcpyDeviceToHost));
    check_error(cudaDeviceSynchronize());
    return count;
}

void DeviceCu::read_results(void* p_output, int length)
{
    check_error(cudaMemcpy(p_output, d_output_buffer, length, cudaMemcpyDeviceToHost));
    check_error(cudaDeviceSynchronize());
}