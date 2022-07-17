#include <cuda_runtime.h>
#include <stdio.h>
#include "reduction.h"
#include "util.h"

__global__ void warmup(int *g_idata, int *g_odata, unsigned int n)
{
    // set thread ID
    unsigned int tid = threadIdx.x;
    // boundary check
    if (tid >= n)
        return;
    // convert global data pointer to the
    int *idata = g_idata + blockIdx.x * blockDim.x;
    // in-place reduction in global memory
    for (int stride = 1; stride < blockDim.x; stride *= 2)
    {
        if ((tid % (2 * stride)) == 0)
        {
            idata[tid] += idata[tid + stride];
        }
        // synchronize within block
        __syncthreads();
    }
    // write result for this block to global mem
    if (tid == 0)
        g_odata[blockIdx.x] = idata[0];
}

int main(int argc, char **argv)
{
    initDevice(0);

    bool bResult = false;
    // initialization

    int size = 1 << 24;
    printf("	with array size %d  ", size);

    // execution configuration
    int blocksize = 1024;
    if (argc > 1)
    {
        blocksize = atoi(argv[1]);
    }
    dim3 block(blocksize, 1);
    dim3 grid((size - 1) / block.x + 1, 1);
    printf("grid %d block %d \n", grid.x, block.x);

    // allocate host memory
    size_t bytes = size * sizeof(int);
    int *idata_host = (int *)malloc(bytes);
    int *odata_host = (int *)malloc(grid.x * sizeof(int));
    int *tmp = (int *)malloc(bytes);

    // initialize the array
    initialData_int(idata_host, size);

    memcpy(tmp, idata_host, bytes);
    double iStart, iElaps;
    int gpu_sum = 0;

    // device memory
    int *idata_dev = NULL;
    int *odata_dev = NULL;
    CHECK(cudaMalloc((void **)&idata_dev, bytes));
    CHECK(cudaMalloc((void **)&odata_dev, grid.x * sizeof(int)));

    // cpu reduction
    int cpu_sum = 0;
    iStart = cpuSecond();
    // cpu_sum = recursiveReduce(tmp, size);
    for (int i = 0; i < size; i++)
        cpu_sum += tmp[i];
    printf("cpu sum:%d \n", cpu_sum);
    iElaps = cpuSecond() - iStart;
    printf("cpu reduce                 elapsed %lf ms cpu_sum: %d\n", iElaps, cpu_sum);

    // kernel 1:reduceNeighbored

    CHECK(cudaMemcpy(idata_dev, idata_host, bytes, cudaMemcpyHostToDevice));
    CHECK(cudaDeviceSynchronize());
    iStart = cpuSecond();
    warmup<<<grid, block>>>(idata_dev, odata_dev, size);
    cudaDeviceSynchronize();
    iElaps = cpuSecond() - iStart;
    cudaMemcpy(odata_host, odata_dev, grid.x * sizeof(int), cudaMemcpyDeviceToHost);
    gpu_sum = 0;
    for (int i = 0; i < grid.x; i++)
        gpu_sum += odata_host[i];
    printf("gpu warmup                 elapsed %lf ms gpu_sum: %d<<<grid %d block %d>>>\n",
           iElaps, gpu_sum, grid.x, block.x);

    // kernel 1:reduceNeighbored

    CHECK(cudaMemcpy(idata_dev, idata_host, bytes, cudaMemcpyHostToDevice));
    CHECK(cudaDeviceSynchronize());
    iStart = cpuSecond();
    reduceNeighbored<<<grid, block>>>(idata_dev, odata_dev);
    cudaDeviceSynchronize();
    iElaps = cpuSecond() - iStart;
    cudaMemcpy(odata_host, odata_dev, grid.x * sizeof(int), cudaMemcpyDeviceToHost);
    gpu_sum = 0;
    for (int i = 0; i < grid.x; i++)
        gpu_sum += odata_host[i];
    printf("gpu reduceNeighbored       elapsed %lf ms gpu_sum: %d<<<grid %d block %d>>>\n",
           iElaps, gpu_sum, grid.x, block.x);

    // free host memory

    free(idata_host);
    free(odata_host);
    CHECK(cudaFree(idata_dev));
    CHECK(cudaFree(odata_dev));

    // reset device
    cudaDeviceReset();

    // check the results
    if (gpu_sum == cpu_sum)
    {
        printf("Test success!\n");
    }
    return EXIT_SUCCESS;
}