cmake_minimum_required(VERSION 3.21)

project(matching_engine
    VERSION 0.1.0
    DESCRIPTION "Learning project: limit order book matching engine"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ME_BUILD_TESTS "Build unit tests" ON)
option(ME_ENABLE_SANITIZERS "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(ME_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

# Compile options shared by every target in the project.
add_library(me_options INTERFACE)
if(MSVC)
    target_compile_options(me_options INTERFACE /W4 $<$<BOOL:${ME_WARNINGS_AS_ERRORS}>:/WX>)
else()
    target_compile_options(me_options INTERFACE
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
        $<$<BOOL:${ME_WARNINGS_AS_ERRORS}>:-Werror>)
endif()

if(ME_ENABLE_SANITIZERS)
    if(MSVC)
        message(WARNING "ME_ENABLE_SANITIZERS is not supported with MSVC; ignoring")
    else()
        set(ME_SANITIZER_FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
        target_compile_options(me_options INTERFACE ${ME_SANITIZER_FLAGS})
        target_link_options(me_options INTERFACE ${ME_SANITIZER_FLAGS})
    endif()
endif()

# Core engine library. Later phases add their sources here.
add_library(me_core
    src/version.cpp)
target_include_directories(me_core PUBLIC include)
target_link_libraries(me_core PUBLIC me_options)
target_compile_definitions(me_core PRIVATE ME_VERSION="${PROJECT_VERSION}")

# Command-line tool.
add_executable(me_cli apps/me_cli.cpp)
target_link_libraries(me_cli PRIVATE me_core)

if(ME_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
