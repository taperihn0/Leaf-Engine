cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Leaf LANGUAGES CXX)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug
		CACHE STRING "Choose one of build types: Debug, Release, RelWithDebInfo"
		FORCE)
	message(STATUS "CMAKE_BUILD_TYPE not specified. Defaulting to Debug.")
else ()
	message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
endif ()

message(STATUS "CMake build type: ${CMAKE_BUILD_TYPE}")

file(GLOB SOURCES CONFIGURE_DEPENDS
	"${CMAKE_SOURCE_DIR}/src/frontend/*.cpp"
	"${CMAKE_SOURCE_DIR}/src/backend/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/utils/Paths.cpp"
	"${CMAKE_SOURCE_DIR}/src/vendor/tbprobe.c"
)

set_source_files_properties(${SOURCES} PROPERTIES LANGUAGE CXX)

if (WIN32)
    enable_language(RC)

	set_source_files_properties(
	    "${CMAKE_SOURCE_DIR}/src/backend/win-embed/resource.rc"
	    PROPERTIES
	    LANGUAGE RC
	)
    
    set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I\"${CMAKE_SOURCE_DIR}\"")

    set_property(SOURCE "${CMAKE_SOURCE_DIR}/src/backend/win-embed/resource.rc"
                 PROPERTY COMPILE_DEFINITIONS 
                 "RESOURCE_DIR=\"${CMAKE_SOURCE_DIR}\"")

	list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/src/backend/win-embed/resource.rc")
	list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/src/backend/win-embed/WinEmbed.hpp")
endif()

if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive-")
endif ()

option(COLLECT_SEARCH_STATS "Collect additional search info" OFF)
option(BUILD_UTILS     	    "Build utils sources"            OFF)

message(STATUS "Collecting Search Stats: " ${COLLECT_SEARCH_STATS})

set(DEFAULT_NEURAL_NET_FILE_NAME "nn128_60_h.bin")

if (BUILD_UTILS)
	add_library(Leaf STATIC ${SOURCES})
	target_compile_definitions(Leaf PUBLIC LEAF_BUILD_UTILS)
	add_subdirectory(src/utils)
	message(STATUS "Building Leaf utils")
else ()
	add_executable(Leaf ${SOURCES})
	message(STATUS "Building Leaf sources")
endif ()

if (COLLECT_SEARCH_STATS)
	target_compile_definitions(Leaf PUBLIC LEAF_COLLECT_SEARCH_STATS)
endif ()

if (INCLUDE_TESTS)
	target_compile_definitions(Leaf PUBLIC LEAF_INCLUDE_TESTS)
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
	"${CMAKE_SOURCE_DIR}/src/utils"
	"${CMAKE_SOURCE_DIR}/src/vendor"
)

if (WIN32)
	target_compile_definitions(Leaf PUBLIC LEAF_PLATFORM_WINDOWS)
elseif (APPLE)
	target_compile_definitions(Leaf PUBLIC LEAF_PLATFORM_MACOS)
elseif (UNIX)
	target_compile_definitions(Leaf PUBLIC LEAF_PLATFORM_LINUX)
else ()
	message(FATAL_ERROR "Platform not recognised.")
endif ()

option(ARCHITECTURE_ARM 	      "Specify ARM architecture"  		      OFF)
option(ARCHITECTURE_X86_64_V3     "Specify x86-64-v3 target archiecture"  OFF)
option(ARCHITECTURE_X86_64_LEGACY "Specify x86-64-v2 target archiecture"  OFF)

if (ARCHITECTURE_X86_64_V3 AND ARCHITECTURE_X86_64_LEGACY)
    message(FATAL_ERROR "Pick only one architecture flag.")
endif ()

if (MSVC AND (ARCHITECTURE_X86_64_V3 OR ARCHITECTURE_X86_64_LEGACY))
	message(WARNING "ARCHITECTURE flags are ignored on MSVC.")
endif ()

if (ARCHITECTURE_ARM AND (ARCHITECTURE_X86_64_V3 OR ARCHITECTURE_X86_64_LEGACY))
	message(FATAL_ERROR "ARCHITECTURE_ARM is not compatible with ARCHITECTURE_X86_64_V*.")
endif ()

if (ARCHITECTURE_ARM)
	target_compile_definitions(Leaf PUBLIC LEAF_ARCHITECTURE_ARM)
else ()
	target_compile_definitions(Leaf PUBLIC LEAF_ARCHITECTURE_X86)
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
	include(CheckIPOSupported)
	check_ipo_supported(RESULT result OUTPUT output)

	if (result)
		set_target_properties(Leaf PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
	endif ()

	if (NOT MSVC)
        if (ARCHITECTURE_X86_64_V3)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=x86-64-v3")
            target_compile_definitions(Leaf PUBLIC LEAF_ENABLE_BMI2)
        elseif (ARCHITECTURE_X86_64_LEGACY)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=x86-64-v2")
        elseif (ARCHITECTURE_ARM)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=native")
        endif ()

		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")
	else ()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2 /Oi /Ot /Oy /GL /fp:fast")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
	endif ()
elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
	if (NOT MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
	else ()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2")
	endif ()
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

option(ENABLE_SIMD_SSE2   "Enable SSE2 optimizations on x86"	OFF)
option(ENABLE_SIMD_SSE3   "Enable SSE3 optimizations on x86"	OFF)
option(ENABLE_SIMD_SSSE3  "Enable SSSE3 optimizations on x86"	OFF)
option(ENABLE_SIMD_SSE4_1 "Enable SSE 4.1 optimizations on x86" OFF)
option(ENABLE_SIMD_SSE4_2 "Enable SSE 4.2 optimizations on x86" OFF)
option(ENABLE_SIMD_AVX    "Enable AVX optimizations on x86"	    OFF)
option(ENABLE_SIMD_AVX2   "Enable AVX2 optimizations on x86"    OFF)
option(ENABLE_SIMD_AVX512 "Enable AVX512 optimizations on x86"  OFF)
option(ENABLE_SIMD_NEON   "Enable ARM NEON optimizations"       OFF)
option(FORCE_PURE         "Disable SIMD optimizations"		    OFF)

if (ARCHITECTURE_ARM AND (
    ENABLE_SIMD_SSE2   OR
    ENABLE_SIMD_SSE3   OR
    ENABLE_SIMD_SSSE3  OR
    ENABLE_SIMD_SSE4_1 OR
    ENABLE_SIMD_SSE4_2 OR
    ENABLE_SIMD_AVX    OR
    ENABLE_SIMD_AVX2   OR
    ENABLE_SIMD_AVX512))
	message(WARNING "ARCHITECTURE_ARM do not support x86 SIMD extension set.")
endif ()

if (FORCE_PURE)
	target_compile_definitions(Leaf PUBLIC LEAF_FORCE_PURE)

	message(STATUS "SIMD optimizations disabled")
elseif (ARCHITECTURE_ARM)
	target_compile_definitions(Leaf PUBLIC LEAF_SIMD_NEON)

	message(STATUS "ARM NEON SIMD optimizations enabled")
elseif (ENABLE_SIMD_SSE2)
	target_compile_definitions(Leaf PUBLIC LEAF_SIMD_SSE2)

	if (MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
	else ()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
	endif ()

	message(STATUS "SIMD optimizations enabled - SSE2 instruction set")
elseif (ENABLE_SIMD_SSE3)
	target_compile_definitions(Leaf PUBLIC LEAF_SIMD_SSE3)

	if (MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE3")
	else ()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse3")
	endif ()

	message(STATUS "SIMD optimizations enabled - SSE3 instruction set")
elseif (ENABLE_SIMD_SSSE3)
	target_compile_definitions(Leaf PUBLIC LEAF_SIMD_SSSE3)

	if (MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSSE3")
	else ()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mssse3")
	endif ()

	message(STATUS "SIMD optimizations enabled - SSSE3 instruction set")
elseif (ENABLE_SIMD_SSE4_1)
	target_compile_definitions(Leaf PUBLIC LEAF_SIMD_SSE4_1)

	if (MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE4.1")
	else ()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.1")
	endif ()

	message(STATUS "SIMD optimizations enabled - SSE4.1 instruction set")
elseif (ENABLE_SIMD_SSE4_2)
	target_compile_definitions(Leaf PUBLIC LEAF_SIMD_SSE4_2)
	
	if (MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE4.2")
	else ()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2")
	endif ()

	message(STATUS "SIMD optimizations enabled - SSE4.2 instruction set")
elseif (ENABLE_SIMD_AVX)
	target_compile_definitions(Leaf PUBLIC LEAF_SIMD_AVX)

	if (MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX")
	else ()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx")
	endif ()

	message(STATUS "SIMD optimizations enabled - AVX instruction set")
elseif (ENABLE_SIMD_AVX2)
	target_compile_definitions(Leaf PUBLIC LEAF_SIMD_AVX2)

	if (MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")
		target_compile_definitions(Leaf PUBLIC LEAF_ENABLE_BMI2)
	else ()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")
	endif ()

	message(STATUS "SIMD optimizations enabled - AVX2 instruction set")
elseif (ENABLE_SIMD_AVX512)
	target_compile_definitions(Leaf PUBLIC LEAF_SIMD_AVX512)

	if (MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX512")
		target_compile_definitions(Leaf PUBLIC LEAF_ENABLE_BMI2)
	else ()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx512f -mavx512bw -mavx512dq")
	endif ()

	message(STATUS "SIMD optimizations enabled - AVX512 instruction set")
else ()
	message(FATAL_ERROR "Did not specified SIMD optimization support")
endif ()

message(STATUS "Compiler flags: ${CMAKE_CXX_FLAGS}")

set_target_properties(Leaf PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY 
	"${CMAKE_SOURCE_DIR}/bin/${PROJECT_NAME}/${CMAKE_BUILD_TYPE}"
	LIBRARY_OUTPUT_DIRECTORY 
	"${CMAKE_SOURCE_DIR}/bin/${PROJECT_NAME}/${CMAKE_BUILD_TYPE}"
	ARCHIVE_OUTPUT_DIRECTORY 
	"${CMAKE_SOURCE_DIR}/bin/intermediates/${PROJECT_NAME}/${CMAKE_BUILD_TYPE}"
)

if (NOT BUILD_UTILS)
	set(DESTINATION_PATH "${CMAKE_SOURCE_DIR}/bin/${PROJECT_NAME}/${CMAKE_BUILD_TYPE}")
else ()
	set(DESTINATION_PATH "${CMAKE_SOURCE_DIR}/bin/Utils/${CMAKE_BUILD_TYPE}")
endif ()

configure_file(
	"${CMAKE_SOURCE_DIR}/assets/nets/${DEFAULT_NEURAL_NET_FILE_NAME}"
	"${DESTINATION_PATH}/${DEFAULT_NEURAL_NET_FILE_NAME}"
	COPYONLY
)

configure_file(
	"${CMAKE_SOURCE_DIR}/assets/nets/${DEFAULT_NEURAL_NET_FILE_NAME}"
	"${DESTINATION_PATH}/${CMAKE_BUILD_TYPE}/${DEFAULT_NEURAL_NET_FILE_NAME}"
	COPYONLY
)

message(STATUS "Copying neural net file from:
				${CMAKE_SOURCE_DIR}/assets/nets/${DEFAULT_NEURAL_NET_FILE_NAME}
			as:
				${DESTINATION_PATH}/${DEFAULT_NEURAL_NET_FILE_NAME}
				${DESTINATION_PATH}/${CMAKE_BUILD_TYPE}/${DEFAULT_NEURAL_NET_FILE_NAME}")

target_compile_definitions(Leaf PUBLIC 
	DEFAULT_NEURAL_NET_FILE_NAME="${DESTINATION_PATH}/${DEFAULT_NEURAL_NET_FILE_NAME}")

message(STATUS "Defaulting to neural net: ${UTILS_DESTINATION_PATH}/${DEFAULT_NEURAL_NET_FILE_NAME}")
