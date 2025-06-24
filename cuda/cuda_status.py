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
    recommendations_shown = False
    
    # Check if NVIDIA driver is missing
    if not results.get('nvidia-smi', False):
        if not recommendations_shown:
            print("\n🔧 Issues:")
            print("=" * 50)
            recommendations_shown = True
        print("\n� NVIDIA Driver:")
        print("   sudo apt update")
        print("   sudo apt install nvidia-driver-535")
        print("   sudo reboot")
    
    # Check if CUDA Toolkit is completely missing
    if not results.get('CUDA Toolkit dir', False) and not results.get('CUDA Libraries', False):
        if not recommendations_shown:
            print("\n� INSTALLATION RECOMMENDATIONS:")
            print("=" * 50)
            recommendations_shown = True
        print("\n📦 CUDA Toolkit:")
        print("   Option 1 (Recommended): Install via conda")
        print("   conda install nvidia/label/cuda-12.9.0::cuda-toolkit")
        print("   ")
        print("   Option 2: Download from NVIDIA")
        print("   wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/cuda-keyring_1.1-1_all.deb")
        print("   sudo dpkg -i cuda-keyring_1.1-1_all.deb")
        print("   sudo apt-get update")
        print("   sudo apt-get -y install cuda-toolkit-12-9")
        print("   ")
        print("   # Add to PATH (add to ~/.bashrc or ~/.zshrc)")
        print("   export PATH=/usr/local/cuda-12.9/bin${PATH:+:${PATH}}")
        print("   export LD_LIBRARY_PATH=/usr/local/cuda-12.9/lib64${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}")
    
    # Check if nvcc is missing but CUDA is installed (PATH issue)
    elif not results.get('nvcc', False) and (results.get('CUDA Toolkit dir', False) or results.get('CUDA Libraries', False)):
        if not recommendations_shown:
            print("\n🔧 INSTALLATION RECOMMENDATIONS:")
            print("=" * 50)
            recommendations_shown = True
        print("\n🔧 CUDA PATH Configuration:")
        print("   CUDA is installed but nvcc not in PATH. Add to ~/.bashrc or ~/.zshrc:")
        print("   export PATH=/usr/local/cuda/bin:$PATH")
        print("   export LD_LIBRARY_PATH=/usr/local/cuda/lib64:$LD_LIBRARY_PATH")
        print("   ")
        print("   Then run: source ~/.bashrc  (or source ~/.zshrc)")
    
    # Check if PyTorch is missing
    if results.get('PyTorch CUDA') is None:
        if not recommendations_shown:
            print("\n🔧 INSTALLATION RECOMMENDATIONS:")
            print("=" * 50)
            recommendations_shown = True
        print("\n🔥 PyTorch with CUDA:")
        print("   pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu121")
        print("   Or with conda: conda install pytorch torchvision torchaudio pytorch-cuda=12.1 -c pytorch -c nvidia")
    
    # Check if TensorFlow is missing
    if results.get('TensorFlow CUDA') is None:
        if not recommendations_shown:
            print("\n🔧 Pending items:")
            print("=" * 50)
            recommendations_shown = True
        print("\n🧠 TensorFlow with CUDA:")
        print("   pip install tensorflow[and-cuda]")
        print("   Or: pip install tensorflow")
    
    if recommendations_shown:
        print("\n Install pending items!")
        print("=" * 50)
    else:
        print("\n🎉 All CUDA components are working properly! No installation needed.")

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
