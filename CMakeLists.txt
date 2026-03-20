cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Leaf)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug
		CACHE STRING "Choose one of build types: Debug, Release, RelWithDebInfo"
		FORCE)
	message(STATUS "CMAKE_BUILD_TYPE not specified. Defaulting to Debug.")
else ()
	message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
endif ()

message("CMake build type: ${CMAKE_BUILD_TYPE}")

file(GLOB SOURCES CONFIGURE_DEPENDS
	"${CMAKE_SOURCE_DIR}/src/frontend/*.cpp"
	"${CMAKE_SOURCE_DIR}/src/backend/*.cpp"
)

if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive-")
endif ()

option(COLLECT_SEARCH_STATS "Collect additional search info" OFF)
option(BUILD_UTILS "Build utils sources" OFF)

message("Collecting Search Stats: " ${COLLECT_SEARCH_STATS})

if (BUILD_UTILS)
	add_library(Leaf STATIC ${SOURCES})
	target_compile_definitions(Leaf PUBLIC BUILD_UTILS)
	add_subdirectory(src/utils)
	message("Building Leaf utils")
else ()
	add_executable(Leaf ${SOURCES})
	message("Building Leaf sources")
endif ()

if (COLLECT_SEARCH_STATS)
	target_compile_definitions(Leaf PUBLIC _COLLECT_SEARCH_STATS)
endif ()

if (INCLUDE_TESTS)
	target_compile_definitions(Leaf PUBLIC _INCLUDE_TESTS)
endif ()

target_compile_definitions(Leaf PUBLIC 
	$<$<CONFIG:Debug>:DEBUG>
	$<$<CONFIG:Release>:RELEASE>
	$<$<CONFIG:RelWithDebInfo>:RELEASE>
)

set_target_properties(Leaf PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

target_include_directories(Leaf PUBLIC
	"${CMAKE_SOURCE_DIR}/src"
	"${CMAKE_SOURCE_DIR}/src/backend"
	"${CMAKE_SOURCE_DIR}/src/frontend"
	"${CMAKE_SOURCE_DIR}/src/util"
)

# Platform detection
if (WIN32)
	target_compile_definitions(Leaf PUBLIC PLATFORM_WINDOWS)
elseif (APPLE)
	target_compile_definitions(Leaf PUBLIC PLATFORM_MACOS)
elseif (UNIX)
	target_compile_definitions(Leaf PUBLIC PLATFORM_LINUX)
else ()
	message(FATAL_ERROR "Platform not recognised.")
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	if (NOT MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Og -O0")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-stack-protector")

		if (NOT BUILD_UTILS)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
		endif ()
	else ()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Od")
	endif ()
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
	if (NOT MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
	else ()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2 /Oi /Ot /Oy /GL /fp:fast")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
	endif ()
elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
	if (NOT MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
	else ()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2")
	endif()
endif ()

if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
else ()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wpedantic")
endif()

if (MSVC)
	# 
else ()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-shift-count-negative")
endif ()

option(ENABLE_SIMD_SSE2   "Enable SSE2 optimizations"	  OFF)
option(ENABLE_SIMD_SSE3   "Enable SSE3 optimizations"	  OFF)
option(ENABLE_SIMD_SSSE3  "Enable SSSE3 optimizations"	  OFF)
option(ENABLE_SIMD_SSE4_1 "Enable SSE 4.1 optimizations"  OFF)
option(ENABLE_SIMD_SSE4_2 "Enable SSE 4.2 optimizations"  OFF)
option(ENABLE_SIMD_AVX    "Enable AVX optimizations"	  OFF)
option(ENABLE_SIMD_AVX2   "Enable AVX2 optimizations"     OFF)
option(ENABLE_SIMD_AVX512 "Enable AVX512 optimizations"   OFF)
option(FORCE_PURE         "Force 'pure' instructions"	  OFF)

if (FORCE_PURE)
	target_compile_definitions(Leaf PUBLIC LEAF_FORCE_PURE)

	message(STATUS "SIMD instructions disabled")
elseif (ENABLE_SIMD_SSE2)
	target_compile_definitions(Leaf PUBLIC LEAF_SIMD_SSE2)

	if (MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
	endif()

	message(STATUS "SIMD instructions enabled - SSE2 instruction set")
elseif (ENABLE_SIMD_SSE3)
	target_compile_definitions(Leaf PUBLIC LEAF_SIMD_SSE3)

	if (MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE3")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse3")
	endif()

	message(STATUS "SIMD instructions enabled - SSE3 instruction set")
elseif (ENABLE_SIMD_SSSE3)
	target_compile_definitions(Leaf PUBLIC LEAF_SIMD_SSSE3)

	if (MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSSE3")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mssse3")
	endif()

	message(STATUS "SIMD instructions enabled - SSSE3 instruction set")
elseif (ENABLE_SIMD_SSE4_1)
	target_compile_definitions(Leaf PUBLIC LEAF_SIMD_SSE4_1)

	if (MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE4.1")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.1")
	endif()

	message(STATUS "SIMD instructions enabled - SSE4.1 instruction set")
elseif (ENABLE_SIMD_SSE4_2)
	target_compile_definitions(Leaf PUBLIC LEAF_SIMD_SSE4_2)
	
	if (MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE4.2")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2")
	endif()

	message(STATUS "SIMD instructions enabled - SSE4.2 instruction set")
elseif (ENABLE_SIMD_AVX)
	target_compile_definitions(Leaf PUBLIC LEAF_SIMD_AVX)

	if (MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx")
	endif()

	message(STATUS "SIMD instructions enabled - AVX instruction set")
elseif (ENABLE_SIMD_AVX2)
	target_compile_definitions(Leaf PUBLIC LEAF_SIMD_AVX2)

	if (MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")
	endif()

	message(STATUS "SIMD instructions enabled - AVX2 instruction set")
elseif (ENABLE_SIMD_AVX512)
	target_compile_definitions(Leaf PUBLIC LEAF_SIMD_AVX512)

	if (MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX512")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx512f -mavx512bw -mavx512dq")	
	endif()

	message(STATUS "SIMD instructions enabled - AVX512 instruction set")
endif()

message(STATUS "Compiler flags: ${CMAKE_CXX_FLAGS}")

set_target_properties(Leaf PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY 
	"${CMAKE_SOURCE_DIR}/bin/${PROJECT_NAME}/${CMAKE_BUILD_TYPE}"
	LIBRARY_OUTPUT_DIRECTORY 
	"${CMAKE_SOURCE_DIR}/bin/${PROJECT_NAME}/${CMAKE_BUILD_TYPE}"
	ARCHIVE_OUTPUT_DIRECTORY 
	"${CMAKE_SOURCE_DIR}/bin/intermediates/${PROJECT_NAME}/${CMAKE_BUILD_TYPE}"
)
