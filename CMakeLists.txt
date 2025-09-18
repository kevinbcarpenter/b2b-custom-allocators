cmake_minimum_required(VERSION 3.16)
project(CustomAllocators)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the output directory for executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Function to add executable from cpp file
function(add_demo_executable cpp_file)
    # Get the filename without extension for the executable name
    get_filename_component(exec_name ${cpp_file} NAME_WE)
    
    # Get the directory name to create unique executable names
    get_filename_component(dir_name ${cpp_file} DIRECTORY)
    string(REPLACE "src/" "" dir_name ${dir_name})
    string(REPLACE "/" "_" dir_name ${dir_name})
    
    # Create unique executable name: directory_filename
    if(NOT dir_name STREQUAL "")
        set(exec_name "${dir_name}_${exec_name}")
    endif()
    
    add_executable(${exec_name} ${cpp_file})
    
    # Special case: set C++17 for 01_basic_17.cpp
    if(exec_name STREQUAL "2_std_allocator_01_basic_17")
        set_property(TARGET ${exec_name} PROPERTY CXX_STANDARD 17)
        set_property(TARGET ${exec_name} PROPERTY CXX_STANDARD_REQUIRED ON)
    endif()
endfunction()

# Add all C++ files as executables
add_demo_executable(src/allocator.cpp)
add_demo_executable(src/1_introduction/00_new_and_delete.cpp)
add_demo_executable(src/1_introduction/01_in_memory.cpp)
add_demo_executable(src/1_introduction/02_pointers.cpp)
add_demo_executable(src/1_introduction/missaligned-vs-aligned.cpp)
add_demo_executable(src/1_introduction/padding-alignment.cpp)
add_demo_executable(src/1_introduction/raii-instead.cpp)
add_demo_executable(src/2_std_allocator/01_basic_17.cpp)
add_demo_executable(src/2_std_allocator/02_basic_after_20.cpp)
add_demo_executable(src/3_pooling/pool-test.cpp)
add_demo_executable(src/3_pooling/pooling-allocator-v2.cpp)
add_demo_executable(src/3_pooling/pooling-allocator.cpp)
add_demo_executable(src/3_pooling/simple-pool-allocator.cpp)
add_demo_executable(src/3_pooling/simple-pool-allocator-manual.cpp)
add_demo_executable(src/3_pooling/tracking-pool-allocator.cpp)
add_demo_executable(src/4_stack/basic_stack.cpp)
add_demo_executable(src/4_stack/basic_stack_traits.cpp)
add_demo_executable(src/8_pmr/pmr-allocator.cpp)