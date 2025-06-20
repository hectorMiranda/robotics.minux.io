#!/usr/bin/env python3

import subprocess
import shutil
import os

def check_nvidia_smi():
    print("\n🔍 Checking `nvidia-smi`...")
    if not shutil.which("nvidia-smi"):
        print("❌ `nvidia-smi` not found in PATH. Is the NVIDIA driver installed?")
        return False
    try:
        output = subprocess.check_output(["nvidia-smi"], stderr=subprocess.STDOUT)
        print("✅ `nvidia-smi` found:\n")
        print(output.decode())
        return True
    except subprocess.CalledProcessError as e:
        print("❌ Error running `nvidia-smi`:\n", e.output.decode())
        return False

def check_nvcc():
    print("\n🔍 Checking `nvcc` (CUDA compiler)...")
    if not shutil.which("nvcc"):
        print("❌ `nvcc` not found in PATH. CUDA Toolkit may not be installed.")
        return False
    try:
        output = subprocess.check_output(["nvcc", "--version"], stderr=subprocess.STDOUT)
        print("✅ `nvcc` found:")
        print(output.decode())
        return True
    except subprocess.CalledProcessError as e:
        print("❌ Error running `nvcc`:\n", e.output.decode())
        return False

def check_cuda_toolkit():
    print("\n🔍 Checking for CUDA Toolkit directory...")
    cuda_path = "/usr/local/cuda"
    if os.path.isdir(cuda_path):
        print(f"✅ CUDA Toolkit directory found at {cuda_path}")
        return True
    else:
        print(f"❌ CUDA Toolkit directory not found at {cuda_path}")
        return False

def check_cuda_runtime():
    print("\n🔍 Checking CUDA Runtime...")
    try:
        # Try to compile and run a simple CUDA program
        test_code = '''
#include <cuda_runtime.h>
#include <iostream>
int main() {
    int deviceCount;
    cudaGetDeviceCount(&deviceCount);
    std::cout << "CUDA devices: " << deviceCount << std::endl;
    return 0;
}
'''
        with open("/tmp/cuda_test.cu", "w") as f:
            f.write(test_code)
        
        # Try to compile
        result = subprocess.run(["nvcc", "/tmp/cuda_test.cu", "-o", "/tmp/cuda_test"], 
                              capture_output=True, text=True)
        if result.returncode == 0:
            # Try to run
            result = subprocess.run(["/tmp/cuda_test"], capture_output=True, text=True)
            if result.returncode == 0:
                print("✅ CUDA Runtime test passed:")
                print(result.stdout)
                return True
            else:
                print("❌ CUDA Runtime test failed to run:")
                print(result.stderr)
                return False
        else:
            print("❌ CUDA compilation failed (nvcc not working properly)")
            return False
    except Exception as e:
        print(f"⚠️ Could not test CUDA Runtime: {e}")
        return None

def check_cuda_libraries():
    print("\n🔍 Checking common CUDA library paths...")
    libs_to_check = [
        "/usr/local/cuda/lib64/libcudart.so",
        "/usr/local/cuda/lib64/libcublas.so", 
        "/usr/local/cuda/lib64/libcufft.so",
        "/usr/lib/x86_64-linux-gnu/libcudart.so"
    ]
    
    found_libs = []
    for lib in libs_to_check:
        if os.path.exists(lib):
            found_libs.append(lib)
    
    if found_libs:
        print("✅ Found CUDA libraries:")
        for lib in found_libs:
            print(f"   {lib}")
        return True
    else:
        print("❌ No CUDA libraries found in common locations")
        return False

def check_pytorch_cuda():
    print("\n🔍 Checking PyTorch CUDA support...")
    try:
        import torch
        print(f"PyTorch version: {torch.__version__}")
        if torch.cuda.is_available():
            print(f"✅ CUDA is available. Device: {torch.cuda.get_device_name(0)}")
            return True
        else:
            print("❌ CUDA not available via PyTorch.")
            return False
    except ImportError:
        print("⚠️ PyTorch is not installed. Skipping CUDA test via torch.")
        return None

def check_tensorflow_cuda():
    print("\n🔍 Checking TensorFlow CUDA support...")
    try:
        import tensorflow as tf
        print(f"TensorFlow version: {tf.__version__}")
        gpus = tf.config.list_physical_devices('GPU')
        if gpus:
            print(f"✅ CUDA is available. GPUs: {[gpu.name for gpu in gpus]}")
            return True
        else:
            print("❌ CUDA not available via TensorFlow.")
            return False
    except ImportError:
        print("⚠️ TensorFlow is not installed. Skipping CUDA test via tensorflow.")
        return None

def print_summary(results):
    print("\n================ CUDA STATUS SUMMARY ================")
    for name, result in results.items():
        if result is True:
            print(f"✅ {name}: OK")
        elif result is False:
            print(f"❌ {name}: NOT OK")
        else:
            print(f"⚠️ {name}: Not tested (missing package)")
    print("====================================================\n")

def print_installation_recommendations(results):
    print("\n🔧 INSTALLATION RECOMMENDATIONS:")
    print("=" * 50)
    
    if not results['nvcc'] or not results['CUDA Toolkit dir']:
        print("\n📦 CUDA Toolkit:")
        print("   Option 1 (Recommended): Install via conda")
        print("   conda install nvidia/label/cuda-12.9.0::cuda-toolkit")
        print("   ")
        print("   Option 2: Download from NVIDIA")
        print("   https://developer.nvidia.com/cuda-downloads")
        print("   Select: Linux > x86_64 > Ubuntu > 22.04 > deb (network)")
    
    if results['PyTorch CUDA'] is None:
        print("\n🔥 PyTorch with CUDA:")
        print("   pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu121")
        print("   Or with conda: conda install pytorch torchvision torchaudio pytorch-cuda=12.1 -c pytorch -c nvidia")
    
    if results['TensorFlow CUDA'] is None:
        print("\n🧠 TensorFlow with CUDA:")
        print("   pip install tensorflow[and-cuda]")
        print("   Or: pip install tensorflow")
    
    print("\n💡 Quick setup commands:")
    print("   # Install CUDA Toolkit")
    print("   wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/cuda-keyring_1.1-1_all.deb")
    print("   sudo dpkg -i cuda-keyring_1.1-1_all.deb")
    print("   sudo apt-get update")
    print("   sudo apt-get -y install cuda-toolkit-12-9")
    print("   ")
    print("   # Add to PATH (add to ~/.bashrc or ~/.zshrc)")
    print("   export PATH=/usr/local/cuda-12.9/bin${PATH:+:${PATH}}")
    print("   export LD_LIBRARY_PATH=/usr/local/cuda-12.9/lib64${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}")
    print("   ")
    print("   # Install Python packages")
    print("   pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu121")
    print("   pip install tensorflow[and-cuda]")
    
    print("\n🔄 After installation, restart terminal and run this script again!")
    print("=" * 50)

if __name__ == "__main__":
    results = {}
    results['nvidia-smi'] = check_nvidia_smi()
    results['nvcc'] = check_nvcc()
    results['CUDA Toolkit dir'] = check_cuda_toolkit()
    results['CUDA Libraries'] = check_cuda_libraries()
    results['CUDA Runtime'] = check_cuda_runtime()
    results['PyTorch CUDA'] = check_pytorch_cuda()
    results['TensorFlow CUDA'] = check_tensorflow_cuda()
    print_summary(results)
    print_installation_recommendations(results)
