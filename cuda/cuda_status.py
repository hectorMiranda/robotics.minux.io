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

if __name__ == "__main__":
    results = {}
    results['nvidia-smi'] = check_nvidia_smi()
    results['nvcc'] = check_nvcc()
    results['CUDA Toolkit dir'] = check_cuda_toolkit()
    results['PyTorch CUDA'] = check_pytorch_cuda()
    results['TensorFlow CUDA'] = check_tensorflow_cuda()
    print_summary(results)
