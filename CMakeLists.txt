cmake_minimum_required(VERSION 3.13.2)
project(geocode VERSION 0.1.0)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
include(recmake/recmake.cmake)
recmkConfigureProject()
set(Boost_USE_STATIC_LIBS TRUE)
find_package(Boost 1.68.0 REQUIRED
    date_time
    filesystem
    program_options
    regex
    thread
)
find_package(OpenSSL REQUIRED)
add_subdirectory(src)
recmkInstallProject()
include(CPack)
