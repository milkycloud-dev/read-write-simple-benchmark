cmake_minimum_required(VERSION 3.20)
project(rw_benchmark VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)

if(MSVC)
    add_compile_options(/W4 /O2 /permissive-)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    add_compile_options(-Wall -Wextra -O3)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++ -mwindows")
endif()

set(SOURCES
    src/main.cpp
    src/benchmark.cpp
    src/drives.cpp
    src/ui.cpp
)

set(HEADERS
    src/benchmark.hpp
    src/drives.hpp
    src/ui.hpp
)

add_executable(rw-benchmark WIN32 ${SOURCES} ${HEADERS})

target_include_directories(rw-benchmark PRIVATE src)

if(WIN32)
    target_link_libraries(rw-benchmark PRIVATE gdiplus dwmapi uxtheme comctl32 shell32 ole32)
endif()

install(TARGETS rw-benchmark DESTINATION bin)
