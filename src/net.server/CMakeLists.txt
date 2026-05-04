cmake_minimum_required(VERSION 3.20)
project(net.server)

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

add_executable(net.server ${SOURCES} ${HEADERS} ${GENERATED_SOURCES} ${GENERATED_HEADERS})

target_link_libraries(net.server PRIVATE net.core)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(TRPG_NET_SERVER_RPATH ${CMAKE_BUILD_RPATH})
  if (TRPG_LINUX_CONFIG_ROOT)
    list(PREPEND TRPG_NET_SERVER_RPATH
        "${TRPG_LINUX_CONFIG_ROOT}"
        "${TRPG_LINUX_CONFIG_ROOT}/lib"
    )
  endif()

  set_target_properties(net.server PROPERTIES
      BUILD_RPATH "${TRPG_NET_SERVER_RPATH}"
  )
  target_link_options(net.server PRIVATE "-Wl,--disable-new-dtags")
endif()
