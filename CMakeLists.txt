cmake_minimum_required(VERSION 3.14)
project(skred-repl CXX C)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configure hazel
add_subdirectory(third_party/hazel)

# Fetch skred API
include(FetchContent)
set(SKRED_VERSION "0.64.1" CACHE STRING "Version of skred API to download")

if(WIN32)
    set(SKRED_ASSET "skred-${SKRED_VERSION}-maxed-windows-x86_64.zip")
elseif(APPLE)
    set(SKRED_ASSET "skred-${SKRED_VERSION}-maxed-macos-universal.tar.gz")
else()
    set(SKRED_ASSET "skred-${SKRED_VERSION}-maxed-linux-x86_64.tar.gz")
endif()

message(STATUS "Fetching skred API: ${SKRED_ASSET}")
FetchContent_Declare(
    skred_api
    URL "https://github.com/octetta/pulp/releases/download/v${SKRED_VERSION}/${SKRED_ASSET}"
)
FetchContent_MakeAvailable(skred_api)

add_executable(skred-repl src/main.cpp)
target_include_directories(skred-repl PRIVATE ${skred_api_SOURCE_DIR}/include)

if(WIN32)
    target_link_libraries(skred-repl PRIVATE hazel ${skred_api_SOURCE_DIR}/lib/libapi.a ws2_32 winmm ole32)
elseif(APPLE)
    target_link_libraries(skred-repl PRIVATE hazel ${skred_api_SOURCE_DIR}/lib/libapi.a "-framework CoreFoundation" "-framework CoreMIDI" "-framework CoreAudio")
else()
    target_link_libraries(skred-repl PRIVATE hazel ${skred_api_SOURCE_DIR}/lib/libapi.a asound)
endif()
