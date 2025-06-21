# CUDA Status Checker

A comprehensive Python script to test and verify CUDA installation and support across different frameworks.

## Features

🔍 **Comprehensive CUDA Testing:**
- NVIDIA driver status (`nvidia-smi`)
- CUDA Toolkit installation (`nvcc` compiler)
- CUDA runtime libraries
- PyTorch CUDA support
- TensorFlow CUDA support
- Installation recommendations

## Usage

```bash
python3 cuda_status.py
```

## Sample Output

```
🔍 Checking `nvidia-smi`...
✅ `nvidia-smi` found:
[NVIDIA GPU information displayed]

🔍 Checking `nvcc` (CUDA compiler)...
❌ `nvcc` not found in PATH. CUDA Toolkit may not be installed.

🔍 Checking for CUDA Toolkit directory...
❌ CUDA Toolkit directory not found at /usr/local/cuda

🔍 Checking CUDA Libraries...
❌ No CUDA libraries found in common locations

🔍 Checking CUDA Runtime...
⚠️ Could not test CUDA Runtime: [Errno 2] No such file or directory: 'nvcc'

🔍 Checking PyTorch CUDA support...
⚠️ PyTorch is not installed. Skipping CUDA test via torch.

🔍 Checking TensorFlow CUDA support...
⚠️ TensorFlow is not installed. Skipping CUDA test via tensorflow.

================ CUDA STATUS SUMMARY ================
✅ nvidia-smi: OK
❌ nvcc: NOT OK
❌ CUDA Toolkit dir: NOT OK
❌ CUDA Libraries: NOT OK
⚠️ CUDA Runtime: Not tested (missing package)
⚠️ PyTorch CUDA: Not tested (missing package)
⚠️ TensorFlow CUDA: Not tested (missing package)
====================================================

🔧 INSTALLATION RECOMMENDATIONS:
==================================================

📦 CUDA Toolkit:
   Option 1 (Recommended): Install via conda
   conda install nvidia/label/cuda-12.9.0::cuda-toolkit
   
   Option 2: Download from NVIDIA
   https://developer.nvidia.com/cuda-downloads
   Select: Linux > x86_64 > Ubuntu > 22.04 > deb (network)

🔥 PyTorch with CUDA:
   pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu121
   Or with conda: conda install pytorch torchvision torchaudio pytorch-cuda=12.1 -c pytorch -c nvidia

🧠 TensorFlow with CUDA:
   pip install tensorflow[and-cuda]
   Or: pip install tensorflow

💡 Quick setup commands:
   # Install CUDA Toolkit
   wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/cuda-keyring_1.1-1_all.deb
   sudo dpkg -i cuda-keyring_1.1-1_all.deb
   sudo apt-get update
   sudo apt-get -y install cuda-toolkit-12-9
   
   # Add to PATH (add to ~/.bashrc or ~/.zshrc)
   export PATH=/usr/local/cuda-12.9/bin${PATH:+:${PATH}}
   export LD_LIBRARY_PATH=/usr/local/cuda-12.9/lib64${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}
   
   # Install Python packages
   pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu121
   pip install tensorflow[and-cuda]

🔄 After installation, restart terminal and run this script again!
==================================================
```

## What Gets Checked

### ✅ Driver Level
- **nvidia-smi**: Checks if NVIDIA driver is installed and working
- Shows GPU information, memory usage, and running processes

### 🛠️ Development Tools
- **nvcc**: CUDA compiler availability
- **CUDA Toolkit**: Installation directory check
- **CUDA Libraries**: Runtime libraries in common locations
- **CUDA Runtime**: Compile and run test to verify full functionality

### 🤖 ML Frameworks
- **PyTorch**: CUDA device availability and compatibility
- **TensorFlow**: GPU device detection and CUDA support

## Installation Requirements

### Prerequisites
- NVIDIA GPU with compatible driver
- Python 3.6+

### CUDA Toolkit Installation
Choose one of these methods:

#### Method 1: APT (Ubuntu/Debian)
```bash
wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/cuda-keyring_1.1-1_all.deb
sudo dpkg -i cuda-keyring_1.1-1_all.deb
sudo apt-get update
sudo apt-get -y install cuda-toolkit-12-9
```

#### Method 2: Conda
```bash
conda install nvidia/label/cuda-12.9.0::cuda-toolkit
```

#### Method 3: Direct Download
Visit [NVIDIA CUDA Downloads](https://developer.nvidia.com/cuda-downloads)

### Environment Setup
Add to your shell configuration file (`~/.bashrc` or `~/.zshrc`):
```bash
export PATH=/usr/local/cuda-12.9/bin${PATH:+:${PATH}}
export LD_LIBRARY_PATH=/usr/local/cuda-12.9/lib64${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}
```

### Python ML Libraries
```bash
# PyTorch with CUDA support
pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu121

# TensorFlow with CUDA support
pip install tensorflow[and-cuda]
```

## Troubleshooting

### Common Issues

**`nvidia-smi` not found:**
- Install NVIDIA drivers: `sudo apt install nvidia-driver-535`
- Reboot after installation

**`nvcc` not found:**
- Install CUDA Toolkit (see installation methods above)
- Add CUDA to PATH

**PyTorch/TensorFlow not detecting CUDA:**
- Ensure you installed the CUDA-enabled versions
- Check CUDA version compatibility
- Verify LD_LIBRARY_PATH includes CUDA libraries

**Permission issues:**
- Make sure your user is in the appropriate groups
- Some operations may require sudo

### Version Compatibility

| CUDA Version | PyTorch | TensorFlow |
|--------------|---------|------------|
| 12.1         | ✅      | ✅         |
| 12.0         | ✅      | ✅         |
| 11.8         | ✅      | ✅         |

## Contributing

Feel free to submit issues or pull requests to improve the CUDA detection capabilities.

## License

This script is part of the robotics.minux.io project.
