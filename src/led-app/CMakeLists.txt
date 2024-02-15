cmake_minimum_required(VERSION 3.10)
project(led-app)

add_definitions(-w)

# Set the C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

enable_testing()

# Find the CURL package
find_package(CURL REQUIRED)


# Include directories for CURL and JSON
include_directories(${CURL_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/libs/rpi-rgb-led-matrix/include)

link_directories(${CMAKE_SOURCE_DIR}/libs/rpi-rgb-led-matrix/lib)

# Define the executable and its source files
add_executable(led-app src/main.cpp)

# Link against the CURL library and nlohmann_json if using find_package
target_link_libraries(led-app  ${CURL_LIBRARIES} rgbmatrix)
# If using find_package for nlohmann_json, uncomment the following
# target_link_libraries(MyApp nlohmann_json::nlohmann_json)



add_executable(led-app-tests tests/test_dummy.cpp)
add_test(NAME run-tests COMMAND led-app-tests)
