Installation
------------

Examples require GNU toolchain for ARM Cortex-M processors and CMake version 3.6 or newer.

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
----------

* CMAKE_BUILD_TYPE — specifies the build type. Possible values are empty, Debug and Release.
* USE_LTO — option enables Link Time Optimization.
