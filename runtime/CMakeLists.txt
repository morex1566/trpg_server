cmake_minimum_required (VERSION 3.20)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

if (POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

project ("trpg_server")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (NOT MSVC)
  add_compile_options(-Wno-narrowing -Wno-float-conversion -Wno-overflow -fpermissive)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
  endif()

  find_package(Threads REQUIRED)
endif()

find_package(Boost CONFIG REQUIRED)
find_package(flatbuffers CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)

add_library(trpg.boost INTERFACE)
if (TARGET Boost::headers)
  target_link_libraries(trpg.boost INTERFACE Boost::headers)
elseif (TARGET Boost::boost)
  target_link_libraries(trpg.boost INTERFACE Boost::boost)
else()
  target_include_directories(trpg.boost INTERFACE ${Boost_INCLUDE_DIRS})
endif()

add_library(trpg.third_party INTERFACE)
target_link_libraries(trpg.third_party
    INTERFACE
        trpg.boost
        flatbuffers::flatbuffers
        spdlog::spdlog
        TBB::tbb
)

if (TARGET TBB::tbbmalloc)
  target_link_libraries(trpg.third_party INTERFACE TBB::tbbmalloc)
endif()

if (TARGET TBB::tbbmalloc_proxy)
  target_link_libraries(trpg.third_party INTERFACE TBB::tbbmalloc_proxy)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Include sub-projects.
add_subdirectory ("src/net.common")
add_subdirectory ("src/net.core")
add_subdirectory ("src/net.server")

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/net.server.test/CMakeLists.txt")
  add_subdirectory ("src/net.server.test")
endif()
