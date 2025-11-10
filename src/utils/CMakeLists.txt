file(GLOB UTILS_SOURCES CONFIGURE_DEPENDS
	"${CMAKE_SOURCE_DIR}/src/utils/*.cpp"
)

add_executable(LeafUtils ${UTILS_SOURCES})

target_link_libraries(LeafUtils PRIVATE Leaf)

set_target_properties(LeafUtils PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY 
	"${CMAKE_SOURCE_DIR}/bin/Utils/${CMAKE_BUILD_TYPE}"
	LIBRARY_OUTPUT_DIRECTORY
	"${CMAKE_SOURCE_DIR}/bin/Utils/${CMAKE_BUILD_TYPE}"
	ARCHIVE_OUTPUT_DIRECTORY 
	"${CMAKE_SOURCE_DIR}/bin/intermediates/Utils/${CMAKE_BUILD_TYPE}"
)
