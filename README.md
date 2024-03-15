Installation
------------

Examples require:

* Arm GNU toolchain for bare-metal targets version 11 or newer.
* CMake version 3.21 or newer.
* Python 3 packages Jinja2 and Kconfiglib.

Quickstart
----------

Clone git repository:

```sh
git clone https://github.com/stxent/halm-examples.git
cd halm-examples
git submodule update --init --recursive
```

Build project with CMake:

```sh
mkdir build
cd build
cmake ..
make
```

Useful settings
---------------

* CMAKE_BUILD_TYPE — option specifies the build type.
* TARGET_NOR — place executables in an external NOR Flash.
* TARGET_SDRAM — place executables in an external SDRAM.
* TARGET_SRAM — place executables in the embedded SRAM.
* USE_BIN — enable generation of executables in Binary format.
* USE_HEX — enable generation of executables in Intel HEX format.
* USE_DFU — enable memory layout compatible with a bootloader.
* USE_LTO — option enables Link Time Optimization.
