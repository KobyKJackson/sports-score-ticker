cmake_minimum_required(VERSION 3.10)

project(object-class-example LANGUAGES CXX)

set(sources
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/BaseObject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ObjectType.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TextObject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ImageObject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MultiObject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ObjectGroup.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ObjectGroupManager.cpp
)

set(headers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/BaseObject.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ObjectType.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TextObject.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ImageObject.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MultiObject.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ObjectGroup.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ObjectGroupManager.h
)

add_executable(object-class-example ${sources})
target_compile_features(object-class-example  PRIVATE cxx_std_17)
