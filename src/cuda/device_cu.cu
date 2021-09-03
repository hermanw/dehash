#include <iostream>
#include <vector>
#include <cuda_runtime.h>

#include "device_cu.h"

typedef unsigned char uchar;
typedef unsigned int uint;

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
    const ulong2* p_hash,
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
    for (int i = 0; i < data_length; i++) {
        data[i] = p_input[i];
    }

    for (int i = 0; i < gpu_section_size; i++)
    {
        uint index = 0;
        if(i==0) index = blockIdx.x;
        else if(i==1) index = blockIdx.y;
        else index = blockIdx.z;

        if(gs[i].type == ds_type_list)
        {
            uint offset = index * gs[i].length;
            for (uint i = 0; i < gs[i].length; i++)
            {
                data[gs[i].index + i] = p_helper[offset + i];
            }
        }
        else if(gs[i].type == ds_type_digit)
        {
            uint offset = (index << 2) + 4 - gs[i].length;
            for (uint i = 0; i < gs[i].length; i++)
            {
                data[gs[i].index + i] = p_number[offset + i];
            }
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

    ulong2 bhash;
    bhash.x = hash.x << 32 + hash.y;
    bhash.y = hash.z << 32 + hash.w;
    int low = 0;
    int high = hash_len - 1;
    do
    {
        int mid = (low + high) / 2;
        ulong2 ahash = p_hash[mid];
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
                atomicInc(p_count, 1);
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
}

DeviceCu::~DeviceCu()
{
    cudaFree(d_count_buffer);
    cudaFree(d_input_buffer);
    cudaFree(d_output_buffer);
    cudaFree(d_hash_buffer);
    cudaFree(d_number_buffer);
    cudaFree(d_helper_buffer);
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
}

void DeviceCu::submit(void *p_input, int input_buffer_size, int hash_buffer_size)
{
    m_hash_buffer_size = hash_buffer_size;
    check_error(cudaMemcpy(d_input_buffer, p_input, input_buffer_size, cudaMemcpyHostToDevice));
    check_error(cudaDeviceSynchronize());
}

int DeviceCu::run(size_t kernel_work_size[3])
{
    // TODO: need revisit
    dim3 numBlocks(kernel_work_size[0], kernel_work_size[1], kernel_work_size[2]);
    if (numBlocks.x > 100) numBlocks.x /= 100;
    else if (numBlocks.y > 100) numBlocks.y /= 100;
    else if (numBlocks.z > 100) numBlocks.z /= 100;

    int data_length = m_cfg->length;
    int gpu_section_size = m_cfg->gpu_sections.size();
    GpuSection gs[3];
    for(int i = 0; i < gpu_section_size; i++)
    {
        auto &ds = m_cfg->gpu_sections[i];
        gs[i].index = ds.index;
        gs[i].length = ds.length;
        gs[i].type = ds.type;
    }

    GpuSection *d_gs;
    check_error(cudaMalloc((void**)&d_gs, sizeof(GpuSection)*3));
    check_error(cudaMemcpy(d_gs, gs, sizeof(GpuSection)*3, cudaMemcpyHostToDevice));

    compute<<<numBlocks, 100>>> ((uint*)d_count_buffer, 
                                    (const uchar*)d_input_buffer, 
                                    (uchar*)d_output_buffer, 
                                    (const ulong2*)d_hash_buffer, 
                                    (const uchar*)d_number_buffer, 
                                    (const uchar*)d_helper_buffer,
                                    m_hash_buffer_size, data_length, gpu_section_size, d_gs);
    
    check_error(cudaDeviceSynchronize());
    cudaFree(d_gs);
    return 0;
}

void DeviceCu::read_results(void* p_output, int length)
{
    check_error(cudaMemcpy(p_output, d_output_buffer, length, cudaMemcpyDeviceToHost));
    check_error(cudaDeviceSynchronize());
}