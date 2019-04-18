# CMakeList.txt : CMake project for silber, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 2.6)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -std=c++14 ")

# Add source to this project's executable.
add_executable (silber "main.cpp"
"BfsSearcher.cpp" 
"DfsSearcher.cpp"
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

