cmake_minimum_required(VERSION 3.20)
project(net.server)

file(GLOB_RECURSE SOURCES "*.cpp" "*.c")
file(GLOB_RECURSE HEADERS "*.h" "*.hpp")

source_group("Source Files" FILES ${SOURCES})
source_group("Header Files" FILES ${HEADERS})

add_executable(net.server ${SOURCES} ${HEADERS})

target_include_directories(net.server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(net.server PRIVATE net.core net.common spdlog flatbuffers)
if (CMAKE_VERSION VERSION_GREATER 3.20)
  set_property(TARGET net.server PROPERTY CXX_STANDARD 20)
endif()
