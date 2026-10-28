cmake_minimum_required(VERSION 3.20)
project(fsyncd VERSION 0.0.1 LANGUAGES C)

# C11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# default to Debug
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

option(FSYNCD_WERROR "Treat warnings as errors" ON)
option(FSYNCD_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" ON)

# common flags
add_library(fsyncd_flags INTERFACE)
target_compile_options(fsyncd_flags INTERFACE -Wall -Wextra -Wshadow)
if(FSYNCD_WERROR)
  target_compile_options(fsyncd_flags INTERFACE -Werror)
endif()

# needed for fsync, pread etc. with -std=c11, mac needs _DARWIN_C_SOURCE for F_FULLFSYNC
target_compile_definitions(fsyncd_flags INTERFACE _POSIX_C_SOURCE=200809L)
if(APPLE)
  target_compile_definitions(fsyncd_flags INTERFACE _DARWIN_C_SOURCE)
endif()

if(FSYNCD_SANITIZE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(SAN_FLAGS -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
  target_compile_options(fsyncd_flags INTERFACE ${SAN_FLAGS})
  target_link_options(fsyncd_flags INTERFACE ${SAN_FLAGS})
endif()

add_library(fsyncd
  src/fsyncd.c
)
target_include_directories(fsyncd PUBLIC include)
target_link_libraries(fsyncd PRIVATE fsyncd_flags)

# each tests/test_*.c is a separate test binary
enable_testing()
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tests/test_*.c)
foreach(test_src ${TEST_SOURCES})
  get_filename_component(test_name ${test_src} NAME_WE)
  add_executable(${test_name} ${test_src})
  target_include_directories(${test_name} PRIVATE src tests)
  target_link_libraries(${test_name} PRIVATE fsyncd fsyncd_flags)
  add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
