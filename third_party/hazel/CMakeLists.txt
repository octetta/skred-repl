cmake_minimum_required(VERSION 3.14)
project(hazel VERSION 0.2.0)

set(CMAKE_CXX_STANDARD 17)

# Optimize FLTK build
set(FLTK_BUILD_TEST OFF CACHE BOOL "" FORCE)
set(FLTK_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FLTK_BUILD_FLUID OFF CACHE BOOL "" FORCE)
set(FLTK_BUILD_FLTK_OPTIONS OFF CACHE BOOL "" FORCE)
set(FLTK_BUILD_FORMS OFF CACHE BOOL "" FORCE)
set(FLTK_BUILD_GL OFF CACHE BOOL "" FORCE)
set(FLTK_BUILD_HTML_DOCS OFF CACHE BOOL "" FORCE)
set(FLTK_BUILD_PDF_DOCS OFF CACHE BOOL "" FORCE)
set(FLTK_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
# SVGs are good for notebooks if we ever want graphics
set(FLTK_OPTION_SVG ON CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(
    fltk
    GIT_REPOSITORY https://github.com/fltk/fltk.git
    GIT_TAG release-1.4.5
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(fltk)

add_library(hazel
    src/HazelApp.cpp
    src/api.cpp
)

target_include_directories(hazel PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(hazel PUBLIC fltk::fltk fltk::images)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    add_executable(hazel_demo examples/demo.cpp)
    target_include_directories(hazel_demo PRIVATE third_party/ueval)
    target_link_libraries(hazel_demo PRIVATE hazel m)
endif()
add_executable(test_eval test_eval.cpp src/api.cpp src/HazelApp.cpp)
target_link_libraries(test_eval PRIVATE fltk::fltk fltk::images)
target_include_directories(test_eval PRIVATE include)
add_executable(test_eval2 test_eval2.cpp src/api.cpp src/HazelApp.cpp)
target_link_libraries(test_eval2 PRIVATE fltk::fltk fltk::images)
target_include_directories(test_eval2 PRIVATE include)
add_executable(test_eval3 test_eval3.cpp src/api.cpp src/HazelApp.cpp)
target_link_libraries(test_eval3 PRIVATE fltk::fltk fltk::images)
target_include_directories(test_eval3 PRIVATE include)
add_executable(test_eval4 test_eval4.cpp src/api.cpp src/HazelApp.cpp)
target_link_libraries(test_eval4 PRIVATE fltk::fltk fltk::images)
target_include_directories(test_eval4 PRIVATE include)
add_executable(tests tests.cpp src/api.cpp src/HazelApp.cpp)
target_link_libraries(tests PRIVATE fltk::fltk fltk::images m)
target_include_directories(tests PRIVATE include third_party/ueval)
