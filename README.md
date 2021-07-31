Installation
------------

Examples require GNU toolchain for ARM Cortex-M processors and CMake version 3.13 or newer.

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
* USE_LTO — option enables Link Time Optimization.
