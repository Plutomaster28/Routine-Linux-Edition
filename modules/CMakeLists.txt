cmake_minimum_required(VERSION 3.15)
project(RoutineModules)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

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
    # ARM-specific flags for better compatibility
    add_compile_options(-march=armv8-a)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "(armv7)|(ARM)")
    set(ARCH_ARM32 TRUE)
    add_compile_options(-march=armv7-a -mfpu=neon)
endif()

# Include parent's include directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

# Automatically find all C source files
file(GLOB C_MODULES "*.c")

# Automatically find all C++ source files
file(GLOB CXX_MODULES "*.cpp")

# Automatically find all Assembly source files
file(GLOB ASM_MODULES "*.asm" "*.s")

# Automatically find all FORTRAN source files
file(GLOB FORTRAN_MODULES "*.f90" "*.f" "*.F90" "*.F")

# Build C modules
foreach(module_src ${C_MODULES})
    get_filename_component(module_name ${module_src} NAME_WE)
    
    # Create shared library
    add_library(${module_name} SHARED ${module_src})
    
    # Set output name
    set_target_properties(${module_name} PROPERTIES
        PREFIX ""  # No 'lib' prefix
        OUTPUT_NAME ${module_name}
    )
    
    # Windows: .dll, Linux: .so
    if(WIN32)
        set_target_properties(${module_name} PROPERTIES SUFFIX ".dll")
    else()
        set_target_properties(${module_name} PROPERTIES SUFFIX ".so")
    endif()
    
    message(STATUS "Added C module: ${module_name}")
endforeach()

# Build C++ modules
foreach(module_src ${CXX_MODULES})
    get_filename_component(module_name ${module_src} NAME_WE)
    
    # Create shared library
    add_library(${module_name} SHARED ${module_src})
    
    # Set output name
    set_target_properties(${module_name} PROPERTIES
        PREFIX ""
        OUTPUT_NAME ${module_name}
    )
    
    if(WIN32)
        set_target_properties(${module_name} PROPERTIES SUFFIX ".dll")
    else()
        set_target_properties(${module_name} PROPERTIES SUFFIX ".so")
    endif()
    
    message(STATUS "Added C++ module: ${module_name}")
endforeach()

if(TARGET local_economy_module)
    include(CTest)
    if(BUILD_TESTING)
        add_executable(economy_module_test
            ${CMAKE_CURRENT_SOURCE_DIR}/../tests/economy_module_test.cpp
        )
        target_include_directories(economy_module_test PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../include
        )
        if(UNIX AND NOT APPLE)
            target_link_libraries(economy_module_test PRIVATE dl)
        endif()
        add_dependencies(economy_module_test local_economy_module)
        add_test(
            NAME economy_module_commands_and_guild_routing
            COMMAND economy_module_test $<TARGET_FILE:local_economy_module>
        )
    endif()
endif()

# Build Assembly modules
if(ASM_MODULES)
    enable_language(ASM)
    
    foreach(module_src ${ASM_MODULES})
        get_filename_component(module_name ${module_src} NAME_WE)
        
        # Create shared library
        add_library(${module_name} SHARED ${module_src})
        # Set output name
        set_target_properties(${module_name} PROPERTIES
            PREFIX ""
            OUTPUT_NAME ${module_name}
        )
        
        if(WIN32)
            set_target_properties(${module_name} PROPERTIES SUFFIX ".dll")
        else()
            set_target_properties(${module_name} PROPERTIES SUFFIX ".so")
        endif()
        
        message(STATUS "Added Assembly module: ${module_name}")
    endforeach()
endif()

# Build FORTRAN modules
if(FORTRAN_MODULES)
    enable_language(Fortran)
    
    foreach(module_src ${FORTRAN_MODULES})
        get_filename_component(module_name ${module_src} NAME_WE)
        
        # Create shared library
        add_library(${module_name} SHARED ${module_src})
        if(CMAKE_Fortran_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${module_name} PRIVATE -Wno-unused-dummy-argument)
        endif()
        
        # Set output name
        set_target_properties(${module_name} PROPERTIES
            PREFIX ""
            OUTPUT_NAME ${module_name}
        )
        
        if(WIN32)
            set_target_properties(${module_name} PROPERTIES SUFFIX ".dll")
        else()
            set_target_properties(${module_name} PROPERTIES SUFFIX ".so")
        endif()
        
        message(STATUS "Added FORTRAN module: ${module_name}")
    endforeach()
endif()
