cmake_minimum_required(VERSION 3.10)
project(led-app)

add_definitions(-w)

# Set the C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)


enable_testing()

# Find the CURL package
find_package(CURL REQUIRED)
find_package(ImageMagick COMPONENTS Magick++ REQUIRED)


include_directories(${CURL_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/libs/rpi-rgb-led-matrix/include)
link_directories(${CMAKE_SOURCE_DIR}/libs/rpi-rgb-led-matrix/lib)

set(sources
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/BaseObject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ObjectType.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TextObject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ImageObject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MultiObject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ObjectGroup.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ObjectGroupManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DisplayManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/json.hpp
)

set(headers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/BaseObject.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ObjectType.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TextObject.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ImageObject.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MultiObject.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ObjectGroup.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ObjectGroupManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DisplayManager.h
)

add_executable(led-app ${sources})

target_link_libraries(led-app  ${CURL_LIBRARIES} rgbmatrix ${ImageMagick_LIBRARIES})
target_include_directories(led-app PRIVATE ${ImageMagick_INCLUDE_DIRS})
target_compile_features(led-app PRIVATE cxx_std_17)
target_compile_definitions(led-app PRIVATE CURL_STATICLIB JSON_ASSERT\(x\)=\(void\)0)


#add_executable(led-app-tests tests/test_dummy.cpp)
#add_test(NAME run-tests COMMAND led-app-tests)
