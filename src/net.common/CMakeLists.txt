cmake_minimum_required(VERSION 3.20)
project(net.common)

file(GLOB_RECURSE SOURCES "*.cpp" "*.c")
file(GLOB_RECURSE HEADERS "*.h" "*.hpp")

source_group("Source Files" FILES ${SOURCES})
source_group("Header Files" FILES ${HEADERS})

add_library(net.common STATIC ${SOURCES} ${HEADERS})

set_target_properties(net.common PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/lib/debug"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/lib/release"
)

if (CMAKE_VERSION VERSION_GREATER 3.20)
  set_property(TARGET net.common PROPERTY CXX_STANDARD 20)
endif()

target_include_directories(net.common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

# External Libraries
target_link_libraries(net.common PUBLIC spdlog flatbuffers)