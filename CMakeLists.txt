cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(Leaf)

file(GLOB SOURCES CONFIGURE_DEPENDS
	"frontend/*.cpp"
	"backend/*.cpp"
)

add_executable(Leaf ${SOURCES})

# MSVS IS NOT SPECIFIED FOR NOW

if (MSVC)
	# pass
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wpedantic")
endif()

if (MSVC)
	# pass
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-shift-count-negative")
endif()
