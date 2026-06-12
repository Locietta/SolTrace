# SolTrace
![Build](https://github.com/NLR-SolTrace/SolTrace/actions/workflows/CI.yml/badge.svg)[![Coverage Status](https://coveralls.io/repos/github/NLR-SolTrace/SolTrace/badge.svg)](https://coveralls.io/github/NLR-SolTrace/SolTrace?branch=develop)


The SolTrace Open Source Project repository contains the source code, tools, and instructions to build a desktop version of the National Renewable Energy Laboratory's SolTrace. SolTrace is a software tool developed at NREL to model concentrating solar power (CSP) systems and analyze their optical performance. Although ideally suited for solar applications, the code can also be used to model and characterize many general optical systems. The creation of the code evolved out of a need to model more complex solar optical systems than could be modeled with existing tools. For more details about SolTrace's capabilities, see the [SolTrace website](https://www.nrel.gov/csp/soltrace.html). For details on integration with SAM, see the [SAM website](https://sam.nrel.gov).

The desktop version of SolTrace for Windows or Linux builds from the following open source projects:

* [LK](https://github.com/nrel/lk) is a scripting language that is integrated into SAM and allows users to add functionality to the program.

* [wxWidgets](https://www.wxwidgets.org/) is a cross-platform graphical user interface platform used for SAM's user interface, and for the development tools included with SSC (SDKtool) and LK (LKscript). SolTrace currently consumes wxWidgets 3.2.4 via [vcpkg](https://github.com/microsoft/vcpkg).

* [WEX](https://github.com/nrel/wex) is a set of extensions to wxWidgets for custom user-interface elements used by SAM, and by LKscript and DView, which are integrated into SAM.

* This repository, **SolTrace**, provides the user interface to assign values to inputs of the computational modules, run the modules in the correct order, and display calculation results. It also includes tools for editing LK scripts and viewing ray intersection and flux map data.

## SolTrace Python API

Users can also run simulations via the [`pysoltrace`](https://github.com/NREL/SolTrace/blob/develop/app/deploy/api/pysoltrace.py) SolTrace Python API found in the folder `app/deploy/api`. Example files for running the API are found in the `app/deploy/api/examples` subfolder. Documentation is available in HTML or PDF format in the corresponding API subfolder. 

The `pysoltrace` API is capable of running multi-threaded simulations, generating flux maps, creating 3D interactive trace plots, and provides other capabilities that are found in the SolTrace graphical interface. The functionality and flexibility of the API generally exceeds that of the graphical interface. 

The API requires the compiled coretrace library. Project files for building this library are generated using CMake as outlined in the steps below. It is possible to build only coretrace and not build the graphical interface by following the steps 1-7, but only building the `coretrace_api` project in step 7.vii.

## Steps for Building SolTrace (Legacy)

These are the general steps you need to follow to set up your computer for developing SolTrace. The repository now uses CMake presets and vcpkg manifest mode to fetch all third-party dependencies (wxWidgets, PCRE2, curl, etc.), so you no longer need to build wxWidgets, LK, or WEX manually.

1. Install development tools

    * **Windows:** Visual Studio 2022 Community or other editions available at [https://www.visualstudio.com/](https://www.visualstudio.com/). Ensure that the "Desktop development with C++" workload is installed.
    * **Linux:** g++ compiler available at [http://www.cprogramming.com/g++.html](http://www.cprogramming.com/g++.html) or as part of the Linux distribution.
    * **macOS:** Xcode 14+

2. Install [CMake 3.24 or newer](https://cmake.org/download/) and ensure it is on your `PATH`.

3. Install [vcpkg](https://learn.microsoft.com/vcpkg/get_started/get-started) in a convenient location and set the `VCPKG_ROOT` environment variable to that path.

4. Clone SolTrace **with submodules** so the `external/lk` and `external/wex` trees are available:

    ```bash
    git clone --recurse-submodules https://github.com/NREL/SolTrace.git
    cd SolTrace
    ```
    If you have already cloned the repository without submodules, you can initialize them with:

    ```bash
    git submodule update --init --recursive
    ```

5. Configure with one of the supplied CMake presets. Presets automatically enable manifest-mode vcpkg, set the required environment variables (`LKDIR`, `WEXDIR`), and choose an appropriate triplet. Common examples:

    * Windows GUI build: `cmake --preset windows-ninja`
    * Windows core-only build: `cmake --preset windows-core`
    * Linux GUI Release: `cmake --preset linux-ninja-release`
    * macOS GUI Debug: `cmake --preset macos-xcode-debug`

    The first configure run will trigger `vcpkg install` and may take a few minutes while dependencies such as wxWidgets 3.2.4 are compiled.

6. Build the desired configuration with the matching build preset, for example:

    ```bash
    cmake --build --preset windows-ninja-debug
    cmake --build --preset windows-ninja-release
    ```

    The GUI executable is written to `app/deploy/x64/SolTrace.exe` on Windows and to `app/deploy` on Linux/macOS. To build only the `coretrace` library, use the `*-core-*` presets (e.g., `cmake --build --preset windows-core-release`).

7. (Optional) Run the unit tests after building:

    ```bash
    ctest --preset windows-ninja-debug
    ```

For more details on configuring or adding new presets, open `CMakePresets.json` in the repository root.

## Steps for Building SolTrace (work in progress)

SolTrace has been updated to use multiple ray tracing engines in addition to the prior implementation. Currently, there is no graphical user interface (it is under development).

Building SolTrace (develop branch) requires a C++-17 capable compiler and cmake 3.19 or greater.  Once these are available, building can be done in the normal pattern of configure and build:

```sh
git clone https://github.com/NatLabRockies/SolTrace.git
cd SolTrace
mkdir build
cd build
cmake ..
cmake --build . -j4
```

Note the `-j4` instructs cmake to use 4 processes to compile the source code. This can be adjusted if the number of processors available is greater (or less) than 4.

### Building with Intel's Embree Ray Tracing Library

Information about Embree can be found on the [Embree webpage](https://www.embree.org/) or on the [Embree github page](https://github.com/RenderKit/embree).

Prior to building SolTrace, you will need to install Embree v4.x.x. On Linux this is best done with a package manager. E.g.,

```sh
sudo apt-get update
sudo apt-get install libembree-dev
```
On Mac, you can use Homebrew

```sh
brew install embree
```

On Windows (this works for Linux and MacOS as well), you need to download the binaries from the [github page](https://github.com/RenderKit/embree) (under the appropriate installation header). Follow the corresponding install instructions found there making sure to add the location of the embree DLL's to your system path.

Once Embree is installed, clone the SolTrace repo, configure with embree enabled, and build:

```sh
git clone https://github.com/NatLabRockies/SolTrace.git
cd SolTrace
mkdir build
cd build
cmake .. -DSOLTRACE_BUILD_EMBREE_SUPPORT=ON
cmake --build . -j4
```

If cmake is having trouble locating the Embree install, you specify Embree's install location passing the `embree_DIR` variable to cmake. In this case the configure command would be

```sh
cmake .. -DSOLTRACE_BUILD_EMBREE_SUPPORT=ON -Dembree_DIR=<EMBREE_INSTALL_DIR>
```

### Building with Nvidia's Optix Ray Tracing Library

SolTrace includes an OptiX-based runner built for GPU-accelerated ray tracing.

#### Prerequisites

* NVIDIA GPU with a driver compatible with your target CUDA toolkit and OptiX SDK.
* CUDA Toolkit 12.0 or newer.
* NVIDIA OptiX SDK 8.x or newer.
* CMake 3.19 or newer.
* C++17-capable compiler.

Verified build and test configurations from GPU runner development include:

* Windows: Visual Studio 2022, CUDA 12.8, OptiX 9.0
* Windows: Visual Studio 2022, CUDA 12.8, OptiX 8.1
* Windows: Visual Studio 2022, CUDA 12.3, OptiX 8.1
* Linux (Red Hat 8.0): gcc 11.2, CUDA 12.3, OptiX 8.0
* Linux (Red Hat 8.0): gcc 12.1, CUDA 12.3, OptiX 8.0
* Linux (Ubuntu 22.04): gcc 11.4, CUDA 12.8, OptiX 9.0

> Note: The OptiX runtime library is provided by the NVIDIA driver. The OptiX SDK provides the headers used at build time.

#### Quick checks

Before building, confirm the GPU, driver, and CUDA toolchain are visible:

```sh
nvidia-smi
nvcc --version
```

On Linux, you can also confirm the OptiX runtime library is available from the driver:

```sh
ldconfig -p | grep nvoptix
```

#### Install the CUDA Toolkit

Install the CUDA Toolkit from NVIDIA before configuring SolTrace: <https://developer.nvidia.com/cuda-downloads>.

Use `nvidia-smi` to check the maximum CUDA version supported by your installed NVIDIA driver, then choose a compatible CUDA Toolkit release.

#### Install the OptiX SDK

Download the OptiX SDK from NVIDIA: <https://developer.nvidia.com/designworks/optix/download>.

If your NVIDIA driver does not support the latest OptiX release, use the legacy downloads page: <https://developer.nvidia.com/designworks/optix/downloads/legacy>.

#### Configure and build SolTrace with OptiX support

Linux example:

```sh
git clone https://github.com/NREL/SolTrace.git
cd SolTrace
mkdir build
cd build
cmake .. \
    -DSOLTRACE_BUILD_OPTIX_SUPPORT=ON \
    -DOptiX_INSTALL_DIR=/path/to/NVIDIA-OptiX-SDK
cmake --build . -j4
```

Windows example:

```bat
mkdir build
cd build
cmake .. ^
    -G "Visual Studio 17 2022" ^
    -DSOLTRACE_BUILD_OPTIX_SUPPORT=ON ^
    -DOptiX_INSTALL_DIR="C:/ProgramData/NVIDIA Corporation/OptiX SDK 8.1.0"
cmake --build . --config Release -j
```

If CMake cannot find the OptiX SDK automatically, set `OptiX_INSTALL_DIR` to the SDK root containing `include/optix.h`, or add that location to `CMAKE_PREFIX_PATH`.

## Contributing

If you would like to report an issue with SolTrace or make a feature request, please let us know by adding a new issue on the [issues page](https://github.com/NREL/SolTrace/issues).

If you would like to submit code to fix an issue or add a feature, you can use GitHub to do so. Please see [Contributing](CONTRIBUTING.md) for instructions.

## License

SolTrace's open source code is copyrighted by the Alliance for Sustainable Energy and licensed under a [mixed MIT and GPLv3 license](LICENSE.md). It allows for-profit and not-for-profit organizations to develop and redistribute software based on SolTrace under terms of an MIT license and requires that research entities including national laboratories, colleges and universities, and non-profit organizations make the source code of any redistribution publicly available under terms of a GPLv3 license.

## Citing SolTrace

We appreciate your use of SolTrace, and ask that you appropriately cite the software in exchange for its open-source publication. Please use one of the following references in documentation that you provide on your work. For general usage citations, the preferred option is:

> Wendelin, T. (2003). "SolTRACE: A New Optical Modeling Tool for Concentrating Solar Optics." Proceedings of the ISEC 2003: International Solar Energy Conference, 15-18 March 2003, Kohala Coast, Hawaii. New York: American Society of Mechanical Engineers, pp. 253-260; NREL Report No. CP-550-32866.

For citations in work that involves substantial development or extension of the existing code, the preferred option is:

> Wendelin, T., Wagner, M.J. (2018). "SolTrace Open-Source Software Project: [github.com/NREL/SolTrace](https://github.com/NREL/SolTrace)". National Renewable Energy Laboratory. Golden, Colorado.
