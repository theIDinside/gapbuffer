cmake_minimum_required(VERSION 3.17)
project(gapbuffer)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")
include_directories(.)

include(FetchContent)
FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG master
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/deps/fmt
)

# This has to be put here, essentially making MakeAvailable(...) pointless
# And the reason for having to put it here, is if we are using CLion and we have multiple profiles
# one for Debug, RelWithDebInfo, Release... etc, it will try to clone the repo 3 times, at the same time, to the same
# folder. Therefore
if (NOT fmt_POPULATED)
    FetchContent_Populate(fmt)
    add_subdirectory(${fmt_SOURCE_DIR} ${fmt_BINARY_DIR})
endif ()

FetchContent_MakeAvailable(fmt)



if (CMAKE_CONFIGURATION_TYPES)
    string(TOLOWER "${CMAKE_CONFIGURATION_TYPES}" CMAKE_CONFIGURATION_TYPES_LOWER)
else()
    string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_CONFIGURATION_TYPES_LOWER)
endif()


add_executable(gapbuffer main.cpp gb/gap_buffer.cpp gb/gap_buffer.hpp gb/movement.cpp gb/movement.hpp gb/text.cpp gb/text.hpp unittest/unit_test.cpp unittest/unit_test.hpp)
add_executable(test_gapbuffer main.cpp gb/gap_buffer.cpp gb/gap_buffer.hpp gb/movement.cpp gb/movement.hpp gb/text.cpp gb/text.hpp unittest/unit_test.cpp unittest/unit_test.hpp)

target_include_directories(test_gapbuffer PRIVATE ./unittest)
target_include_directories(gapbuffer PRIVATE ./unittest)

target_link_libraries(gapbuffer fmt)
target_link_libraries(test_gapbuffer fmt)

# TODO: add checking for MSVC / G++ / Clang++, so the correct flags are set

if(INTRINSICS_SET)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2 -march=avx2")
    message("Intrinsics setting: ${INTRINSICS_SET}")
    target_compile_definitions(gapbuffer PRIVATE -DINTRINSICS=1)
endif()


foreach(config ${CMAKE_CONFIGURATION_TYPES_LOWER})
    message("Config type: ${config}")
    if (${config} MATCHES "relwithdebinfo")
        #add preprocessor definition something like this bellow
        message("Setting GapBuffer debug.")
        target_compile_definitions(gapbuffer PRIVATE -DGBDEBUG)
        message("Adding AVX intrinsics features")
    elseif(${config} MATCHES "release")

    else()
        target_compile_definitions(gapbuffer PRIVATE -DGBDEBUG)
        message("No configuration type matched for ${CMAKE_CONFIGURATION_TYPES_LOWER}")
    endif()
endforeach()