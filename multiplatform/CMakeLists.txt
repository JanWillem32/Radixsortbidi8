cmake_minimum_required(VERSION 4.2)
project(multiplatform)

set(CMAKE_CXX_STANDARD 26)
add_compile_options(-march=x86-64-v2 -Ofast -fpermissive -Wa,-mbig-obj -save-temps)

add_executable(multiplatform
	main.cpp
	../Radixsortbidi8.hpp
)