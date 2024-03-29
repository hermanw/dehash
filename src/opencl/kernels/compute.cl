#define BLOCK_LEN 64 // In bytes
#define LENGTH_SIZE 8 // In bytes

// macro from compiler
// #define DATA_LENGTH 18
// #define X_INDEX 0
// #define X_LENGTH 6
// #define X_TYPE 0
// #define Y_INDEX 6
// #define Y_LENGTH 3
// #define Y_TYPE 1
// #define Z_INDEX 17
// #define Z_LENGTH 1
// #define Z_TYPE 2

#ifndef X_LENGTH
#define X_LENGTH 0
#endif

#ifndef Y_LENGTH
#define Y_LENGTH 0
#endif

#ifndef Z_LENGTH
#define Z_LENGTH 0
#endif

__kernel void compute(
    __global int* p_count,
    __constant uchar* p_input,
    __global uchar* p_output,
    __global const ulong2* p_hash,
    __constant uchar* p_number,
    __constant uchar* p_helper,
    int hash_len)
{
    // fill data
    uchar data[BLOCK_LEN]= {0};
    data[DATA_LENGTH] = 0x80;
    data[BLOCK_LEN - LENGTH_SIZE] = (uchar)(DATA_LENGTH << 3);
    for (int i = 0; i < DATA_LENGTH - X_LENGTH - Y_LENGTH - Z_LENGTH; i++) {
        data[i] = p_input[i];
    }

#ifdef X_TYPE
#if X_TYPE == 0
    uint x_offset = get_global_id(0) * X_LENGTH;
    for (uint i = 0; i < X_LENGTH; i++)
    {
        data[X_INDEX + i] = p_helper[x_offset + i];
    }
#elif X_TYPE == 1
    uint x_offset = (get_global_id(0) << 2) + 4 - X_LENGTH;
    for (uint i = 0; i < X_LENGTH; i++)
    {
        data[X_INDEX + i] = p_number[x_offset + i];
    }
#endif
#endif

#ifdef Y_TYPE
#if Y_TYPE == 0
    uint y_offset = get_global_id(1) * Y_LENGTH;
    for (uint i = 0; i < Y_LENGTH; i++)
    {
        data[Y_INDEX + i] = p_helper[y_offset + i];
    }
#elif Y_TYPE == 1
    uint y_offset = (get_global_id(1) << 2) + 4 - Y_LENGTH;
    for (uint i = 0; i < Y_LENGTH; i++)
    {
        data[Y_INDEX + i] = p_number[y_offset + i];
    }
#endif
#endif

#ifdef Z_TYPE
#if Z_TYPE == 0
    uint z_offset = get_global_id(2) * Z_LENGTH;
    for (uint i = 0; i < Z_LENGTH; i++)
    {
        data[Z_INDEX + i] = p_helper[z_offset + i];
    }
#elif Z_TYPE == 1
    uint z_offset = (get_global_id(2) << 2) + 4 - Z_LENGTH;
    for (uint i = 0; i < Z_LENGTH; i++)
    {
        data[Z_INDEX + i] = p_number[z_offset + i];
    }
#elif Z_TYPE == 2
    const uchar COE[17] = {7,9,10,5,8,4,2,1,6,3,7,9,10,5,8,4,2};
    const uchar C_SUM[11] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'X'};
    uint sum = 0;
    for (uint i = 0; i < 17; i++)
    {
        sum += (data[i] - '0') * COE[i];
    }
    uint r = sum % 11;
    r = 12 - r;
    r = select(r, r-11, r>10);
    data[Z_INDEX] =C_SUM[r];
#endif
#endif

    // hash
    uint4 hash = (uint4)(0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476);

    md5(&hash, data);

    ulong2 bhash = as_ulong2(hash);
    int low = 0;
    int high = hash_len - 1;
    do
    {
        int mid = (low + high) / 2;
        ulong2 ahash = p_hash[mid];
        if(ahash.s0 < bhash.s0)
        {
            low = mid + 1;
        }
        else if (ahash.s0 > bhash.s0)
        {
            high = mid - 1;
        }
        else
        {
            if(ahash.s1 < bhash.s1) {low = mid + 1;}
            else if (ahash.s1 > bhash.s1) {high = mid - 1;}
            else
            {
                atomic_inc(p_count);
                int offset = mid * DATA_LENGTH;
                for (int j = 0; j < DATA_LENGTH; j++)
                {
                    p_output[offset + j] = data[j];
                }
                break;
            }
        }
    } while (low <= high);
}
