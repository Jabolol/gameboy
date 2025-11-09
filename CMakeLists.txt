cmake_minimum_required(VERSION 3.20)

project(gameboy)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SDL2_PATH "${CMAKE_SOURCE_DIR}/external/sdl2")
set(EMSDK_PATH "${CMAKE_SOURCE_DIR}/external/emsdk")
set(EMSCRIPTEN_PATH "${EMSDK_PATH}/upstream/emscripten")

if(NOT EXISTS "${EMSCRIPTEN_PATH}")
    execute_process(COMMAND "${EMSDK_PATH}/emsdk" install latest)
    execute_process(COMMAND "${EMSDK_PATH}/emsdk" activate latest)
    if(EXISTS "${EMSCRIPTEN_PATH}")
        message(STATUS "Emscripten installed at ${EMSCRIPTEN_PATH}")
    else()
        message(FATAL_ERROR "Emscripten not installed")
    endif()
endif()

set(SRCDIR "${CMAKE_SOURCE_DIR}/sources")
file(GLOB SRC "${SRCDIR}/*.c")

add_executable(${PROJECT_NAME} ${SRC})
target_compile_options(${PROJECT_NAME} PRIVATE -flto)

add_subdirectory(${SDL2_PATH})
include_directories(${SDL2_PATH}/include)
target_link_libraries(${PROJECT_NAME} SDL2)

set(WASM_FLAGS
    -O3
    -s WASM=1
    -s USE_SDL=2
    -s EXPORT_ES6=1
    -s MODULARIZE=1
    -s MALLOC=emmalloc
    -s MAX_WEBGL_VERSION=2
    -s GL_UNSAFE_OPTS=1
    -s LZ4=1
    -s AUDIO_WORKLET=1
    -s WASM_WORKERS=1
    -s ENVIRONMENT=worker
    -s ELIMINATE_DUPLICATE_FUNCTIONS=1
    -o www/static/gameboy.js
    -pthread
    -s PTHREAD_POOL_SIZE=1
    -s PTHREAD_POOL_DELAY_LOAD=1
    -s ASYNCIFY
    -s ALLOW_MEMORY_GROWTH=1
    -s SHARED_MEMORY=1
    -s WASM_BIGINT=1
    --closure 1
    -flto
)

if(EXISTS "${PROJECT_SOURCE_DIR}/ROMs")
    list(APPEND WASM_FLAGS --preload-file ROMs@/ROMs)
else()
    message(WARNING "ROMs directory not found, no runnable ROMs will be available")
endif()

add_custom_target(wasm
    COMMAND ${EMSCRIPTEN_PATH}/emcc ${SRC} ${WASM_FLAGS}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_dependencies(${PROJECT_NAME} wasm)
