/*
g++ -std=c++17 -o cuda_top cuda_top.cpp \
    -I/usr/local/cuda/include \
    -L/usr/local/cuda/lib64 -lcudart -lcuda -lnvidia-ml

*/

#include <cuda_runtime.h>
#include <nvml.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <vector>
#include <string>
#include <cstdlib>

void clear_screen() {
    std::cout << "\033[2J\033[1;1H";
}

void print_headers() {
    std::cout << std::left
        << std::setw(5) << "ID"
        << std::setw(25) << "Name"
        << std::setw(10) << "Compute"
        << std::setw(12) << "Mem Total"
        << std::setw(12) << "Mem Used"
        << std::setw(12) << "Mem Free"
        << std::setw(8) << "Temp"
        << std::setw(10) << "Util (%)"
        << "\n";
    std::cout << std::string(94, '-') << "\n";
}

int main() {
    nvmlReturn_t nvmlRes = nvmlInit();
    bool nvml_available = (nvmlRes == NVML_SUCCESS);

    int device_count = 0;
    cudaError_t err = cudaGetDeviceCount(&device_count);
    if (err != cudaSuccess) {
        std::cerr << "cudaGetDeviceCount failed: " << cudaGetErrorString(err) << std::endl;
        return 1;
    }
    std::vector<nvmlDevice_t> nvml_handles(device_count);

    if (nvml_available) {
        for (int i = 0; i < device_count; ++i) {
            nvmlRes = nvmlDeviceGetHandleByIndex(i, &nvml_handles[i]);
            if (nvmlRes != NVML_SUCCESS) {
                nvml_available = false;
                break;
            }
        }
    }

    while (true) {
        clear_screen();
        std::cout << "CUDA Top (" << device_count << " GPU"
                  << (device_count == 1 ? "" : "s") << ")\n\n";
        print_headers();

        for (int dev = 0; dev < device_count; ++dev) {
            cudaDeviceProp prop;
            cudaSetDevice(dev);
            cudaGetDeviceProperties(&prop, dev);

            size_t free_mem = 0, total_mem = 0;
            cudaMemGetInfo(&free_mem, &total_mem);

            int temp = -1, util = -1;
            if (nvml_available) {
                unsigned int t;
                nvmlUtilization_t utilinfo;
                if (nvmlDeviceGetTemperature(nvml_handles[dev], NVML_TEMPERATURE_GPU, &t) == NVML_SUCCESS)
                    temp = t;
                if (nvmlDeviceGetUtilizationRates(nvml_handles[dev], &utilinfo) == NVML_SUCCESS)
                    util = utilinfo.gpu;
            }

            std::cout << std::left
                << std::setw(5) << dev
                << std::setw(25) << prop.name
                << std::setw(10) << (std::to_string(prop.major) + "." + std::to_string(prop.minor))
                << std::setw(12) << (std::to_string(total_mem / (1024 * 1024)) + " MB")
                << std::setw(12) << (std::to_string((total_mem - free_mem) / (1024 * 1024)) + " MB")
                << std::setw(12) << (std::to_string(free_mem / (1024 * 1024)) + " MB")
                << std::setw(8) << (temp == -1 ? "N/A" : std::to_string(temp) + "C")
                << std::setw(10) << (util == -1 ? "N/A" : std::to_string(util))
                << "\n";
        }
        std::cout << "\n[Ctrl+C to exit]\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (nvml_available)
        nvmlShutdown();
    return 0;
}
