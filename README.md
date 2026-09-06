# HALM Examples

Examples demonstrating the usage of drivers from the HALM library.

## Overview

`halm-examples` provides reference implementations for drivers included in
the HALM library, covering all platforms supported by HALM. These examples
are designed not only as ready-to-run demonstrations but also as practical
templates. Each example includes linker reference scripts tailored to
the target platform. In some cases, multiple build variants are available for
a single platform.

Special build configurations include:
- **LPC175x/LPC176x**: builds with USB call tracing enabled to facilitate
  debugging of platform-independent USB drivers.
- **LPC43xx**: builds targeting the secondary M0 core.

Some examples support building not only for on-chip flash memory but also for
other memory types, such as SRAM, SDRAM, and NOR flash, allowing code execution
from these memory regions.

## Project Structure

The project is organized as a two-level CMake project: the top-level build
script sequentially invokes lower-level build scripts for each supported
platform.

Key directories:

* `examples/` — contains individual directories for different platforms
  and a shared `helpers/` directory used during builds for all variants.
  The per-platform directories often include board-specific initialization code
  and pin configuration examples.
* `templates/` — holds Jinja2-based templates used to generate
  platform-specific C code when the corresponding template is enabled.
* `tools/` — includes auxiliary build scripts and utilities.
* `docker/` — contains a sample Dockerfile for creating a build container.

## Components

The project includes generalized examples covering the following
functionalities:

* Serial communication (with and without DMA)
* SPI (with and without DMA)
* ADC, DAC (with and without DMA)
* CAN
* External interrupts
* Single-edge and double-edge PWM
* Lifetime timer
* Periodic timers
* Timer Capture and Counter for external events
* EEPROM, Flash examples
* External parallel memories
* I2C master and slave
* I2S echo and tone examples
* MMCSD using dedicated peripherals and SDIO over SPI
* BOD, RTC, WDT
* Power Management examples (Sleep, Suspend, and Shutdown modes)
* USB CDC, MSC, UAC device classes
* Timer Factory and Tickless Timer Factory
* Work Queue and Unique Work Queue

Additionally, platform-specific examples are available for certain targets,
showcasing unique peripherals available on those devices.

## Requirements

To build the project, the following tools and packages are required:

* **GCC 13 or newer** — the GNU Compiler Collection, required for
  building the x86 version.
* **ARM GCC 13 or newer** — Arm GNU Toolchain, required for
  Cortex-M embedded targets.
* **RISC-V GCC 13 or newer** — RISC-V GNU Toolchain, required for
  RISC-V embedded targets.
* **CMake 3.21 or newer** — used for configuring and generating build
  systems across platforms.
* **Python 3.6+** with the following packages:
  * `jinja2` — for rendering code templates
  * `kconfiglib` — for generating Kconfig configuration files
* **libuv-devel** — required for building the x86 target

## Build Examples

1. Clone the Repository

```sh
git clone https://github.com/stxent/halm-examples.git
cd halm-examples
git submodule update --init --recursive
```

2. Standard Build

```sh
mkdir build
cd build
cmake ..
make
```

3. Build with Ninja

```sh
mkdir build
cd build
export CFLAGS="${CFLAGS} -fdiagnostics-color=always"
cmake .. -GNinja
ninja
```

4. Build in a Docker Container

```sh
cd docker
docker build -t halm-examples:latest .
docker run -it halm-examples
```

5. Build a Debug Version and Copy Artifacts from the Container

```sh
docker run --name halm-examples_debug -it halm-examples -DCMAKE_BUILD_TYPE=Debug
docker cp halm-examples_debug:/build/halm-examples/deploy/ .
```

## Build Options

The following build options control the build behavior:

* **CMAKE_BUILD_TYPE** — Specifies the build type. Possible values:
  empty, `Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel`.
* **KCONFIG_DEFCONFIG** — Command used as a Kconfig file generator. This allows
  you to specify a custom `defconfig` command for generating initial Kconfig
  configuration files.
* **USE_BIN** — Convert executables to Binary format.
* **USE_HEX** — Convert executables to Intel HEX format.
* **USE_DFU** — Enable memory layout compatible with a bootloader.
* **USE_LTO** — Enable Link Time Optimization.
* **TARGET_NOR** — Use external NOR flash for the artifact if available.
  This option modifies the linker script to place code and data in external
  NOR memory regions.
* **TARGET_SDRAM** — Use external SDRAM for the artifact if available.
* **TARGET_SRAM** — Use internal or external SRAM for the artifact if available.
