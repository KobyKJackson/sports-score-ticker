cmake_minimum_required(VERSION 3.10)
project(led_ticker C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS_RELEASE "-O2")
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG")

# rpi-rgb-led-matrix library
set(RGB_LIB_DIR ${CMAKE_SOURCE_DIR}/libs/rpi-rgb-led-matrix)
set(RGB_INCDIR ${RGB_LIB_DIR}/include)
set(RGB_LIBDIR ${RGB_LIB_DIR}/lib)

# Check that the library has been built
if(NOT EXISTS ${RGB_LIBDIR}/librgbmatrix.a)
    message(FATAL_ERROR
        "rpi-rgb-led-matrix library not found at ${RGB_LIBDIR}/librgbmatrix.a\n"
        "Please build it first:\n"
        "  cd ${RGB_LIB_DIR} && make -j$(nproc)")
endif()

# Source files
set(SOURCES
    src/main.c
    src/score_reader.c
    src/ticker.c
    src/logo_cache.c
)

add_executable(led_ticker ${SOURCES})

# Include paths
target_include_directories(led_ticker PRIVATE
    ${RGB_INCDIR}
    ${CMAKE_SOURCE_DIR}/src
)

# Link rpi-rgb-led-matrix (static library, written in C++ with C bindings)
target_link_libraries(led_ticker
    ${RGB_LIBDIR}/librgbmatrix.a
    m
    pthread
    rt
    stdc++
)

# Install target
install(TARGETS led_ticker DESTINATION bin)
