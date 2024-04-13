cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(pico-crsf)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# There is another CMakeLists.txt in the src directory
add_subdirectory(src)
