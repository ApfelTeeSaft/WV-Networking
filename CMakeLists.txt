cmake_minimum_required(VERSION 3.24)
project(WVNet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add GLM dependency (shared with WVCore)
include(FetchContent)
if(NOT TARGET glm)
    FetchContent_Declare(GLM
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        1.0.1
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(GLM)
endif()

# WVNet library
add_library(WVNet STATIC
    # Platform
    src/platform/Socket.cpp

    # Core networking
    src/BitStream.cpp
    src/Packet.cpp
    src/NetConnection.cpp
    src/NetDriver.cpp
    src/NetworkManager.cpp

    # Actor system
    src/Actor.cpp
    src/World.cpp

    # Replication & RPC
    src/ReplicationManager.cpp
    src/RPCManager.cpp
)

target_include_directories(WVNet PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(WVNet PUBLIC ${glm_SOURCE_DIR})

target_compile_features(WVNet PUBLIC cxx_std_20)

# Platform-specific definitions and libraries
if(WIN32)
    target_compile_definitions(WVNet PUBLIC PLATFORM_WINDOWS)
    target_link_libraries(WVNet PRIVATE ws2_32)
elseif(APPLE)
    target_compile_definitions(WVNet PUBLIC PLATFORM_MACOS PLATFORM_UNIX)
elseif(UNIX)
    target_compile_definitions(WVNet PUBLIC PLATFORM_LINUX PLATFORM_UNIX)
endif()

# Optional: Build samples
option(WVNET_BUILD_SAMPLES "Build WVNet samples" ON)
if(WVNET_BUILD_SAMPLES)
    add_subdirectory(samples)
endif()

# Optional: Build tests
option(WVNET_BUILD_TESTS "Build WVNet tests" OFF)
if(WVNET_BUILD_TESTS)
    add_subdirectory(tests)
endif()
