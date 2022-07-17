#include <cuda_runtime.h>
__global__ void reduceNeighbored(int *g_idata, int *g_odata)
{
    unsigned int tid = threadIdx.x;
    int *g_cur_idata = g_idata + blockDim.x * blockIdx.x;
    // create 1024 int shared memory
    __shared__ int s_data[1024];
    s_data[tid] = g_cur_idata[tid];

    for (int stride = 1; stride < blockDim.x; stride *= 2)
    {
        if (tid % (stride * 2) == 0)
        {
            s_data[tid] = s_data[tid + stride];
        }
        __syncthreads();
    }
    if (tid == 0)
    {
        g_odata[blockIdx.x] = s_data[0];
    }
}