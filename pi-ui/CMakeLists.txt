cmake_minimum_required(VERSION 3.5)
project(esp_ui_pi_port C)
set(CMAKE_C_STANDARD 11)
# Find SDL2 and CURL (for HTTP client utilities)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(CURL REQUIRED libcurl)
include_directories(${SDL2_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src)
link_directories(${SDL2_LIBRARY_DIRS} ${CURL_LIBRARY_DIRS})

# Collect sources automatically, excluding backup directories
file(GLOB_RECURSE SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.c" "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_executable(pi_ui ${SRC_FILES})

# Add LVGL - Use local installation
if(EXISTS "${CMAKE_SOURCE_DIR}/lvgl")
	include_directories(${CMAKE_SOURCE_DIR}/lvgl)
	include_directories(${CMAKE_SOURCE_DIR}/lvgl/src)
	# Add LVGL source files
	file(GLOB_RECURSE LVGL_SOURCES "${CMAKE_SOURCE_DIR}/lvgl/src/*.c")
	target_sources(pi_ui PRIVATE ${LVGL_SOURCES})
	message(STATUS "LVGL found and configured")
else()
	message(FATAL_ERROR "LVGL not found. Please run: git clone https://github.com/lvgl/lvgl.git")
endif()

target_link_libraries(pi_ui ${SDL2_LIBRARIES} ${CURL_LIBRARIES} pthread m dl)
