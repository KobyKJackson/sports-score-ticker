cmake_minimum_required(VERSION 3.10)
project(led_ticker CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")

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
    src/main.cpp
    src/score_reader.cpp
    src/ticker.cpp
    src/logo_cache.cpp
)

add_executable(led_ticker ${SOURCES})

# Include paths
target_include_directories(led_ticker PRIVATE
    ${RGB_INCDIR}
    ${CMAKE_SOURCE_DIR}/src
)

# Link rpi-rgb-led-matrix
target_link_libraries(led_ticker
    ${RGB_LIBDIR}/librgbmatrix.a
    m
    pthread
    rt
)

# Install target
install(TARGETS led_ticker DESTINATION bin)
