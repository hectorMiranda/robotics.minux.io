#!/usr/bin/env python3

import subprocess
import shutil

def check_nvidia_smi():
    print("🔍 Checking `nvidia-smi`...")
    if not shutil.which("nvidia-smi"):
        print("❌ `nvidia-smi` not found in PATH. Is the NVIDIA driver installed?")
        return
    try:
        output = subprocess.check_output(["nvidia-smi"], stderr=subprocess.STDOUT)
        print("✅ `nvidia-smi` found:\n")
        print(output.decode())
    except subprocess.CalledProcessError as e:
        print("❌ Error running `nvidia-smi`:\n", e.output.decode())

def check_pytorch_cuda():
    print("\n🔍 Checking PyTorch CUDA support...")
    try:
        import torch
        print(f"PyTorch version: {torch.__version__}")
        if torch.cuda.is_available():
            print(f"✅ CUDA is available. Device: {torch.cuda.get_device_name(0)}")
        else:
            print("❌ CUDA not available via PyTorch.")
    except ImportError:
        print("⚠️ PyTorch is not installed. Skipping CUDA test via torch.")

if __name__ == "__main__":
    check_nvidia_smi()
    check_pytorch_cuda()
