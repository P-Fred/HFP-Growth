cmake_minimum_required(VERSION 2.8)

set(PACKAGE_VERSION "1.0")
set(PACKAGE_LICENSE "GPL")
SET(PROJECT_DESCRIPTION_SUMMARY "HFPgrowth and IFPgrowth are data mining algorithms to compute entropic measures such as entropy and mutual information")
set(CMAKE_BUILD_TYPE Release)

project(hfpgrowth)

#set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Boost flags
set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED OFF)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost 1.54.0 REQUIRED COMPONENTS program_options system) 

#find_package(PkgConfig)
#pkg_check_modules(GAML gaml REQUIRED)

find_package(Threads)

add_compile_options(-std=c++17 -Wall -pedantic -fdiagnostics-color)

add_subdirectory(src)
add_subdirectory(HFP-growth)
add_subdirectory(IFP-growth)
add_subdirectory(HAPriori)

