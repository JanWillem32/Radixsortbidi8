cmake_minimum_required(VERSION 4.2)
set(CMAKE_CXX_STANDARD 26)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
add_compile_definitions("_DEBUG")
if(WIN32)
add_compile_options(/Gv /bigobj /volatile:iso /vd0)
add_definitions(-D _CRT_SECURE_NO_WARNINGS)
add_compile_definitions(
	"NOMINMAX"
	"WIN32_LEAN_AND_MEAN"
	"VC_EXTRALEAN"
)
endif()

else()
add_compile_definitions("NDEBUG")
if(WIN32)
add_compile_options(/O2 /Ob3 /GL /LTCG /Gv /bigobj /volatile:iso /vd0 /Gw)
add_definitions(-D _CRT_SECURE_NO_WARNINGS)
add_compile_definitions(
	"NOMINMAX"
	"WIN32_LEAN_AND_MEAN"
	"VC_EXTRALEAN"
)
else()
add_compile_options(-Ofast -save-temps)
endif()
endif()

project(multiplatform)
add_executable(multiplatform
	main.cpp
	../Radixsortbidi8.hpp
)