cmake_minimum_required(VERSION 3.12)
project(iqraa)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)

if(DEFINED ENV{HOME})
    set(HOME_DIR $ENV{HOME})
    set(iqra_DIR "/.iqraa")
    set(IQRAA_VERSION "1.0")
    else()
    message(FATAL_ERROR "HOME environment variable is not defined")
    endif()
    
# Define the OUT_DIR configuration variable
add_definitions(-DOUT_DIR="${HOME_DIR}${iqra_DIR}")

# Find the curl library
find_package(CURL REQUIRED)

# Add source files to be compiled
set(SOURCES
    htmlParser.cpp
    iqraa.cpp
)

# Add executable and link with curl library
add_executable(iqraa ${SOURCES})
target_link_libraries(iqraa CURL::libcurl)