cmake_minimum_required(VERSION 3.10)

project(DeviceDriverApp C)

find_package(raylib 5.0 QUIET)

# If not found, fetch and build it automatically
if (NOT raylib_FOUND)
    include(FetchContent)
    FetchContent_Declare(
            raylib
            GIT_REPOSITORY https://github.com/raysan5/raylib.git
            GIT_TAG        5.5
    )
    FetchContent_MakeAvailable(raylib)
endif()

add_executable(DeviceDriverApp main.c
        tabbed_window.c
        tabbed_window.h
        key_converter.c
        key_converter.h
        cdev_reader.c
        cdev_reader.h)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_BINARY_DIR}/_deps/raylib-src/src
)
target_include_directories(${PROJECT_NAME} PRIVATE src/raygui.h)

target_link_libraries(${PROJECT_NAME} PRIVATE raylib GL m pthread dl rt X11 wayland-client)