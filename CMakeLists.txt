cmake_minimum_required (VERSION 3.20)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("us2d_server")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# External Libraries: spdlog
add_library(spdlog STATIC IMPORTED GLOBAL)
set_target_properties(spdlog PROPERTIES
    IMPORTED_LOCATION_DEBUG "${CMAKE_SOURCE_DIR}/external/spdlog/debug/libspdlogd.a"
    IMPORTED_LOCATION_RELEASE "${CMAKE_SOURCE_DIR}/external/spdlog/release/libspdlog.a"
)

# External Libraries: flatbuffers
add_library(flatbuffers STATIC IMPORTED GLOBAL)
set_target_properties(flatbuffers PROPERTIES
    IMPORTED_LOCATION_DEBUG "${CMAKE_SOURCE_DIR}/external/flatbuffers/debug/libflatbuffersd.a"
    IMPORTED_LOCATION_RELEASE "${CMAKE_SOURCE_DIR}/external/flatbuffers/release/libflatbuffers.a"
)

# Include sub-projects.
add_subdirectory ("src/net.common")
add_subdirectory ("src/net.core")
add_subdirectory ("src/net.server")
