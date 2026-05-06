cmake_minimum_required(VERSION 3.20)
project(net.common)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "*.cpp" "*.c")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "*.h" "*.hpp")

set(GENERATED_SOURCES ${SOURCES})
set(GENERATED_HEADERS ${HEADERS})
list(FILTER GENERATED_SOURCES INCLUDE REGEX "\\.generated\\.(c|cc|cpp|cxx)$")
list(FILTER GENERATED_HEADERS INCLUDE REGEX "\\.generated\\.(h|hh|hpp|hxx)$")

if (GENERATED_SOURCES)
  list(REMOVE_ITEM SOURCES ${GENERATED_SOURCES})
endif()

if (GENERATED_HEADERS)
  list(REMOVE_ITEM HEADERS ${GENERATED_HEADERS})
endif()

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source Files" FILES ${SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Header Files" FILES ${HEADERS})

if (GENERATED_SOURCES OR GENERATED_HEADERS)
  source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Generated Files" FILES ${GENERATED_SOURCES} ${GENERATED_HEADERS})
endif()

add_library(net.common STATIC ${SOURCES} ${HEADERS} ${GENERATED_SOURCES} ${GENERATED_HEADERS})

set_target_properties(net.common PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/lib/debug"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/lib/release"
)



target_link_libraries(net.common PUBLIC trpg.third_party)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(net.common PUBLIC atomic)
endif()
