cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(fuse-example VERSION 0.0.1 LANGUAGES C)

# Set C compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FILE_OFFSET_BITS=64")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall --pedantic -g")

# Define output directories for the compiled binaries
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# Specify where to look for CMake modules
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})

# Find the FUSE package
find_package(FUSE REQUIRED)

# Find json-c library
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONC REQUIRED json-c)

# Include directories for json-c
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${FUSE_INCLUDE_DIR})
include_directories(${JSONC_INCLUDE_DIRS})

# Add the source files located in the 'src' directory
add_executable(fuse-example src/fuse-example.c src/device_manager.c)

# Link libraries: FUSE and json-c
target_link_libraries(fuse-example ${FUSE_LIBRARIES} ${JSONC_LIBRARIES})

# Optional: If you are on a system where pkg-config cannot find json-c, you can manually link:
# target_link_libraries(fuse-example ${FUSE_LIBRARIES} json-c)
