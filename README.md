# Simple Add Max External

This is a simple Max/MSP external that adds two numbers together.

## Overview

The `simple_add` object has two inlets and one outlet:
- **Left Inlet (0)**: Accepts a number (int or float). Adds it to the value stored in the right operand and outputs the result.
- **Right Inlet (1)**: Accepts a number (int or float). Stores this value as the right operand to be added to subsequent left inlet values.

## Building

To build this external, you need the Max SDK.

### Prerequisites

- CMake (3.10 or later)
- C Compiler (GCC, Clang, or MSVC)
- [Max SDK](https://github.com/cycling74/max-sdk-base) (Download and extract it)

### Instructions

1.  Create a build directory:
    ```bash
    mkdir build
    cd build
    ```

2.  Run CMake, pointing to your Max SDK location:
    ```bash
    cmake -DMAX_SDK_PATH=/path/to/max-sdk ..
    ```
    *Replace `/path/to/max-sdk` with the actual path to the SDK.*

3.  Build the project:
    ```bash
    cmake --build .
    ```

4.  The output file (`simple_add.mxo` on macOS or `simple_add.mxe64` on Windows) will be generated. Copy this file to your Max packages or externals folder.

## Usage

1.  Create a new object in Max patcher.
2.  Type `simple_add` (ensure the external is in Max's search path).
3.  Connect number boxes to the inlets and outlet.
    - Send a number to the right inlet to set the "addend".
    - Send a number to the left inlet to compute the sum.

### Example

[ number ]     [ number ]
    |              |
    | (Left)       | (Right)
    +-------+------+
            |
       [ simple_add ]
            |
            |
       [ number ]
