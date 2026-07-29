cmake_minimum_required(VERSION 3.15)

# Set Ninja as default generator if not already specified
if(NOT CMAKE_GENERATOR)
    set(CMAKE_GENERATOR "Ninja" CACHE INTERNAL "" FORCE)
    message(STATUS "Using Ninja build system (default)")
endif()

project(DiscordBot VERSION 1.0.0 LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # Generates compile_commands.json for IDE support

# Platform and architecture detection
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(PLATFORM_WINDOWS TRUE)
    message(STATUS "Platform: Windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PLATFORM_LINUX TRUE)
    message(STATUS "Platform: Linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(PLATFORM_MACOS TRUE)
    message(STATUS "Platform: macOS")
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64)|(AMD64)|(amd64)")
    set(ARCH_X64 TRUE)
    message(STATUS "Architecture: x86_64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "(aarch64)|(ARM64)|(arm64)")
    set(ARCH_ARM64 TRUE)
    message(STATUS "Architecture: ARM64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "(armv7)|(ARM)")
    set(ARCH_ARM32 TRUE)
    message(STATUS "Architecture: ARM32")
endif()

# Resolve native libraries through the active platform's pkg-config database.
# This is intentionally preferred over global CMake package discovery: under
# WSL, Windows/MSYS2 directories can be inherited through PATH and otherwise
# produce an unusable mixed-platform link.
find_package(PkgConfig REQUIRED)
pkg_check_modules(CURL REQUIRED IMPORTED_TARGET libcurl)
pkg_check_modules(LIBWEBSOCKETS REQUIRED IMPORTED_TARGET libwebsockets)
pkg_check_modules(YAML_CPP REQUIRED IMPORTED_TARGET yaml-cpp)
pkg_check_modules(OPENSSL REQUIRED IMPORTED_TARGET openssl)

# Distribution packages expose Lua under different pkg-config names.
# pkg_search_module also recreates its imported target on a cached reconfigure;
# a hand-rolled LUA_FOUND loop can retain the cache flag but lose the target.
pkg_search_module(LUA REQUIRED IMPORTED_TARGET
    lua lua5.4 lua-5.4 lua54
    lua5.3 lua-5.3 lua53
    lua5.2 lua-5.2 lua52
    lua5.1 lua-5.1 lua51
)

find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${LUA_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    src/main.cpp
    src/websocket_client.cpp
    src/http_client.cpp
    src/rate_limiter.cpp
    src/event_dispatcher.cpp
    src/discord_bot.cpp
    src/discord_message_utils.cpp
    src/discord_interaction_utils.cpp
    src/config_loader.cpp
    src/command_handler.cpp
    src/commands.cpp
    src/module_loader.cpp
    src/lua_module.cpp
    src/extension_loader.cpp
    src/script_engine.cpp
)

# Headers
set(HEADERS
    include/websocket_client.hpp
    include/http_client.hpp
    include/rate_limiter.hpp
    include/event_dispatcher.hpp
    include/discord_bot.hpp
    include/discord_message_utils.hpp
    include/discord_interaction_utils.hpp
    include/config_loader.hpp
    include/discord_types.hpp
    include/command_handler.hpp
    include/commands.hpp
    include/module_interface.h
    include/module_loader.hpp
    include/lua_module.hpp
    include/extension_interface.h
    include/extension_loader.hpp
    include/script_engine.hpp
)

# Create executable
add_executable(routine ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(routine
    PkgConfig::CURL
    PkgConfig::OPENSSL
    PkgConfig::LIBWEBSOCKETS
    PkgConfig::LUA
    PkgConfig::YAML_CPP
    nlohmann_json::nlohmann_json
    Threads::Threads
)

# Platform-specific libraries
if(PLATFORM_WINDOWS)
    target_link_libraries(routine ws2_32 wsock32)
elseif(PLATFORM_LINUX)
    target_link_libraries(routine dl)  # For dlopen/dlclose
endif()

# Compiler flags
if(MSVC)
    target_compile_options(routine PRIVATE /W4)
else()
    target_compile_options(routine PRIVATE -Wall -Wextra -pedantic)
endif()

if(BUILD_TESTING)
    add_executable(discord_message_utils_test
        tests/discord_message_utils_test.cpp
        src/discord_message_utils.cpp
    )
    target_include_directories(discord_message_utils_test PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    if(MSVC)
        target_compile_options(discord_message_utils_test PRIVATE /W4)
    else()
        target_compile_options(discord_message_utils_test PRIVATE
            -Wall -Wextra -pedantic
        )
    endif()
    add_test(NAME discord_message_chunking
             COMMAND discord_message_utils_test)

    add_executable(discord_interaction_utils_test
        tests/discord_interaction_utils_test.cpp
        src/discord_interaction_utils.cpp
    )
    target_include_directories(discord_interaction_utils_test PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(discord_interaction_utils_test PRIVATE
        nlohmann_json::nlohmann_json
    )
    if(MSVC)
        target_compile_options(discord_interaction_utils_test PRIVATE /W4)
    else()
        target_compile_options(discord_interaction_utils_test PRIVATE
            -Wall -Wextra -pedantic
        )
    endif()
    add_test(NAME discord_interaction_parsing
             COMMAND discord_interaction_utils_test)
endif()
