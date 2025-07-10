/*
Simple CUDA GPU test - compile with:
g++ -std=c++17 -o cuda_simple cuda_simple.cpp \
    -I/usr/local/cuda/include \
    -L/usr/local/cuda/lib64 -L/usr/lib/x86_64-linux-gnu \
    -lcudart -lcuda
*/

#include <cuda_runtime.h>
#include <iostream>
#include <iomanip>

int main() {
    int device_count = 0;
    cudaError_t err = cudaGetDeviceCount(&device_count);
    
    if (err != cudaSuccess) {
        std::cerr << "CUDA Error: " << cudaGetErrorString(err) << std::endl;
        return 1;
    }
    
    std::cout << "CUDA Simple GPU Test\n";
    std::cout << "====================\n\n";
    std::cout << "Found " << device_count << " CUDA device(s)\n\n";
    
    for (int dev = 0; dev < device_count; ++dev) {
        cudaDeviceProp prop;
        cudaSetDevice(dev);
        cudaGetDeviceProperties(&prop, dev);
        
        size_t free_mem = 0, total_mem = 0;
        cudaMemGetInfo(&free_mem, &total_mem);
        
        std::cout << "Device " << dev << ": " << prop.name << "\n";
        std::cout << "  Compute Capability: " << prop.major << "." << prop.minor << "\n";
        std::cout << "  Total Memory: " << (total_mem / (1024 * 1024)) << " MB\n";
        std::cout << "  Free Memory: " << (free_mem / (1024 * 1024)) << " MB\n";
        std::cout << "  Used Memory: " << ((total_mem - free_mem) / (1024 * 1024)) << " MB\n";
        std::cout << "  Multiprocessors: " << prop.multiProcessorCount << "\n";
        std::cout << "  Max Threads per Block: " << prop.maxThreadsPerBlock << "\n";
        std::cout << "  Clock Rate: " << (prop.clockRate / 1000) << " MHz\n\n";
    }
    
    return 0;
}
