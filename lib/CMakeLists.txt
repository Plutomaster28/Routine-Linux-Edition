cmake_minimum_required(VERSION 3.15)
project(RoutineExtensions)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Platform detection
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(PLATFORM_WINDOWS TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PLATFORM_LINUX TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(PLATFORM_MACOS TRUE)
endif()

# Architecture detection
if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64)|(AMD64)|(amd64)")
    set(ARCH_X64 TRUE)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "(aarch64)|(ARM64)|(arm64)")
    set(ARCH_ARM64 TRUE)
    # ARM-specific optimizations
    add_compile_options(-march=armv8-a)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "(armv7)|(ARM)")
    set(ARCH_ARM32 TRUE)
    add_compile_options(-march=armv7-a -mfpu=neon)
endif()

# Include parent's include directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

# Automatically find all C source files
file(GLOB C_EXTENSIONS "*.c")

# Automatically find all C++ source files
file(GLOB CXX_EXTENSIONS "*.cpp")

# Automatically find all Assembly source files
file(GLOB ASM_EXTENSIONS "*.asm" "*.s")

# Automatically find all FORTRAN source files
file(GLOB FORTRAN_EXTENSIONS "*.f90" "*.f" "*.F90" "*.F")
# The legacy Fortran sample predates the ExtensionAPI table ABI and is kept as
# numerical interop reference source only. Do not produce a loadable artifact
# that the runtime would correctly reject.
list(FILTER FORTRAN_EXTENSIONS EXCLUDE REGEX "fortran_extension\\.(f90|f|F90|F)$")

# Build C extensions
foreach(ext_src ${C_EXTENSIONS})
    get_filename_component(ext_name ${ext_src} NAME_WE)
    
    # Create shared library
    add_library(${ext_name} SHARED ${ext_src})
    
    # Set output name
    set_target_properties(${ext_name} PROPERTIES
        PREFIX ""  # No 'lib' prefix
        OUTPUT_NAME ${ext_name}
    )
    
    # Windows: .dll, Linux: .so
    if(WIN32)
        set_target_properties(${ext_name} PROPERTIES SUFFIX ".dll")
    else()
        set_target_properties(${ext_name} PROPERTIES SUFFIX ".so")
    endif()
    
    message(STATUS "Added C extension: ${ext_name}")
endforeach()

# Build C++ extensions
foreach(ext_src ${CXX_EXTENSIONS})
    get_filename_component(ext_name ${ext_src} NAME_WE)
    
    # Create shared library
    add_library(${ext_name} SHARED ${ext_src})
    
    # Set output name
    set_target_properties(${ext_name} PROPERTIES
        PREFIX ""
        OUTPUT_NAME ${ext_name}
    )
    
    if(WIN32)
        set_target_properties(${ext_name} PROPERTIES SUFFIX ".dll")
    else()
        set_target_properties(${ext_name} PROPERTIES SUFFIX ".so")
    endif()
    
    message(STATUS "Added C++ extension: ${ext_name}")
endforeach()

if(TARGET local_economy_extension)
    include(CTest)
    if(BUILD_TESTING)
        add_executable(economy_extension_test
            ${CMAKE_CURRENT_SOURCE_DIR}/../tests/economy_extension_test.cpp
        )
        target_include_directories(economy_extension_test PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../include
        )
        if(UNIX AND NOT APPLE)
            target_link_libraries(economy_extension_test PRIVATE dl)
        endif()
        add_test(
            NAME economy_extension_transactions_and_persistence
            COMMAND economy_extension_test
                $<TARGET_FILE:local_economy_extension>
                ${CMAKE_CURRENT_BINARY_DIR}/economy_extension_test.db
        )
    endif()
endif()

# Build Assembly extensions
if(ASM_EXTENSIONS)
    enable_language(ASM)
    
    foreach(ext_src ${ASM_EXTENSIONS})
        get_filename_component(ext_name ${ext_src} NAME_WE)
        
        # Create shared library
        add_library(${ext_name} SHARED ${ext_src})
        # Set output name
        set_target_properties(${ext_name} PROPERTIES
            PREFIX ""
            OUTPUT_NAME ${ext_name}
        )
        
        if(WIN32)
            set_target_properties(${ext_name} PROPERTIES SUFFIX ".dll")
        else()
            set_target_properties(${ext_name} PROPERTIES SUFFIX ".so")
        endif()
        
        message(STATUS "Added Assembly extension: ${ext_name}")
    endforeach()
endif()

# Build FORTRAN extensions
if(FORTRAN_EXTENSIONS)
    enable_language(Fortran)
    
    foreach(ext_src ${FORTRAN_EXTENSIONS})
        get_filename_component(ext_name ${ext_src} NAME_WE)
        
        # Create shared library
        add_library(${ext_name} SHARED ${ext_src})
        if(CMAKE_Fortran_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${ext_name} PRIVATE -Wno-unused-dummy-argument)
        endif()
        
        # Set output name
        set_target_properties(${ext_name} PROPERTIES
            PREFIX ""
            OUTPUT_NAME ${ext_name}
        )
        
        if(WIN32)
            set_target_properties(${ext_name} PROPERTIES SUFFIX ".dll")
        else()
            set_target_properties(${ext_name} PROPERTIES SUFFIX ".so")
        endif()
        
        message(STATUS "Added FORTRAN extension: ${ext_name}")
    endforeach()
endif()
