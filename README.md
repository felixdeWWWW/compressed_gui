# Visualization of PSNR-bitrate of picture compressed by HEIC and JPEG-1
## HOW TO USE?

### Requirements

- CMake (version >= 3.10)
- C++ compiler (e.g., g++, clang++)
- (Optional) Git for cloning the project

### Building the Project
Setup build environment (example build on wsl for windows)
```bash
sudo apt update
sudo apt install build-essential cmake ninja-build git \
                 g++-mingw-w64-x86-64 \
                 pkg-config autoconf automake libtool
```

Clone repo, build from source (with debugging)
```bash
git clone https://github.com/felixdeWWWW/compressed_gui.git
cd compressed_gui

cmake -B build -G Ninja \
      -DCMAKE_TOOLCHAIN_FILE=mingw-toolchain.cmake \
      -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

## Contributors
[Felix Wagner](https://github.com/felixdeWWWW/)
[Roman Kobets](https://github.com/rhombus19)
