#!/bin/bash
mkdir CMakeWorkDir_toDelete
cd CMakeWorkDir_toDelete
# you can set CMAKE_BINARY_DIR for folder other than CMakeWorkDir_toDelete
cmake ..
cmake --build .