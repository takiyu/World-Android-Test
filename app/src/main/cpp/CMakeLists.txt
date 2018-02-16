cmake_minimum_required(VERSION 3.4.1)

set (CMAKE_CXX_STANDARD 11)  # C++ 11

include_directories(World/src
                    World/tools)

add_library(hello-jni SHARED

            hello-jni.cpp
            world-example.cpp

            World/tools/parameterio.cpp
            World/tools/audioio.cpp
            World/src/d4c.cpp
            World/src/cheaptrick.cpp
            World/src/common.cpp
            World/src/dio.cpp
            World/src/codec.cpp
            World/src/fft.cpp
            World/src/stonemask.cpp
            World/src/synthesisrealtime.cpp
            World/src/synthesis.cpp
            World/src/matlabfunctions.cpp
            World/src/harvest.cpp
)

# Include libraries needed for hello-jni lib
target_link_libraries(hello-jni
                      android
                      log)
