cmake_minimum_required(VERSION 3.10)

project(gameboy)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SDL2_PATH "${CMAKE_SOURCE_DIR}/external/sdl2")

set(SRCDIR "${CMAKE_SOURCE_DIR}/sources")

file(GLOB SRC "${SRCDIR}/*.c")

add_executable(${PROJECT_NAME} ${SRC})

add_subdirectory(${SDL2_PATH})

include_directories(${SDL2_PATH}/include)

target_link_libraries(${PROJECT_NAME} SDL2)
