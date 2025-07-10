#ifndef CUDA_UTILS_H
#define CUDA_UTILS_H

// CUDA utilities for MINUX
// Provides GPU information and monitoring capabilities

#ifdef __cplusplus
extern "C" {
#endif

// Function prototypes
int cuda_is_available(void);
void cuda_print_device_info(void);
void cuda_print_memory_info(void);
void cuda_print_temperature_info(void);

#ifdef __cplusplus
}
#endif

#endif // CUDA_UTILS_H
