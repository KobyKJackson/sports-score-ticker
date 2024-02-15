cmake_minimum_required(VERSION 3.10)
project(led-app)

# Set the C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

enable_testing()

# Find the CURL package
find_package(CURL REQUIRED)

# If you have installed nlohmann_json via a package manager and it's findable
# find_package(nlohmann_json 3.2.0 REQUIRED)

# Include directories for CURL and JSON
include_directories(${CURL_INCLUDE_DIR})
# Replace this with the correct path to your nlohmann/json.hpp if not using find_package
#include_directories(${CMAKE_SOURCE_DIR}/src/nlohmann)

link_directories(${CMAKE_SOURCE_DIR}/libs/rpi-rgb-led-matrix/lib)

# Define the executable and its source files
add_executable(led-app src/main.cpp)

# Link against the CURL library and nlohmann_json if using find_package
target_link_libraries(led-app  ${CURL_LIBRARIES} rgbmatrix)
# If using find_package for nlohmann_json, uncomment the following
# target_link_libraries(MyApp nlohmann_json::nlohmann_json)



add_executable(led-app-tests tests/test_dummy.cpp)
add_test(NAME run-tests COMMAND led-app-tests)
