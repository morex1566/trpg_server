cmake_minimum_required (VERSION 3.20)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("trpg_server")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(TRPG_THIRD_PARTY_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(TRPG_THIRD_PARTY_EXTERNAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/so")
set(TRPG_THIRD_PARTY_DEBUG_RPATH
    "${TRPG_THIRD_PARTY_EXTERNAL_DIR}/spdlog/debug"
    "${TRPG_THIRD_PARTY_EXTERNAL_DIR}/tbb/debug"
    "${TRPG_THIRD_PARTY_EXTERNAL_DIR}/flatbuffers/debug"
)
set(TRPG_THIRD_PARTY_RELEASE_RPATH
    "${TRPG_THIRD_PARTY_EXTERNAL_DIR}/spdlog/release"
    "${TRPG_THIRD_PARTY_EXTERNAL_DIR}/tbb/release"
    "${TRPG_THIRD_PARTY_EXTERNAL_DIR}/flatbuffers/release"
)
set(CMAKE_BUILD_RPATH
    "$<$<CONFIG:Debug>:${TRPG_THIRD_PARTY_DEBUG_RPATH}>"
    "$<$<NOT:$<CONFIG:Debug>>:${TRPG_THIRD_PARTY_RELEASE_RPATH}>"
)
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

include_directories(${TRPG_THIRD_PARTY_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

function(trpg_add_imported_shared_library target_name debug_path release_path)
  foreach(library_path IN ITEMS "${debug_path}" "${release_path}")
    if (NOT EXISTS "${library_path}")
      message(FATAL_ERROR "Required third-party library is missing: ${library_path}")
    endif()
  endforeach()

  add_library(${target_name} SHARED IMPORTED GLOBAL)
  set_target_properties(${target_name} PROPERTIES
      IMPORTED_LOCATION_DEBUG "${debug_path}"
      IMPORTED_LOCATION_RELEASE "${release_path}"
      IMPORTED_LOCATION_RELWITHDEBINFO "${release_path}"
      IMPORTED_LOCATION_MINSIZEREL "${release_path}"
  )
endfunction()

function(trpg_add_imported_interface_library target_name)
  add_library(${target_name} INTERFACE IMPORTED GLOBAL)
endfunction()

function(trpg_add_runtime_library_symlink link_path link_target)
  list(APPEND TRPG_RUNTIME_LIBRARY_SYMLINK_COMMANDS
      COMMAND "${CMAKE_COMMAND}" -E rm -f "${link_path}"
      COMMAND "${CMAKE_COMMAND}" -E create_symlink "${link_target}" "${link_path}"
  )
  set(TRPG_RUNTIME_LIBRARY_SYMLINK_COMMANDS "${TRPG_RUNTIME_LIBRARY_SYMLINK_COMMANDS}" PARENT_SCOPE)
endfunction()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  trpg_add_imported_shared_library(
      spdlog
      "${TRPG_THIRD_PARTY_EXTERNAL_DIR}/spdlog/debug/libspdlog_debug.so"
      "${TRPG_THIRD_PARTY_EXTERNAL_DIR}/spdlog/release/libspdlog.so"
  )

  trpg_add_imported_shared_library(
      flatbuffers
      "${TRPG_THIRD_PARTY_EXTERNAL_DIR}/flatbuffers/debug/libflatbuffers_debug.so"
      "${TRPG_THIRD_PARTY_EXTERNAL_DIR}/flatbuffers/release/libflatbuffers.so"
  )

  trpg_add_imported_shared_library(
      tbb
      "${TRPG_THIRD_PARTY_EXTERNAL_DIR}/tbb/debug/libtbb_debug.so"
      "${TRPG_THIRD_PARTY_EXTERNAL_DIR}/tbb/release/libtbb.so"
  )

  trpg_add_imported_shared_library(
      tbbmalloc
      "${TRPG_THIRD_PARTY_EXTERNAL_DIR}/tbb/debug/libtbbmalloc_debug.so"
      "${TRPG_THIRD_PARTY_EXTERNAL_DIR}/tbb/release/libtbbmalloc.so"
  )

  trpg_add_imported_shared_library(
      tbbmalloc_proxy
      "${TRPG_THIRD_PARTY_EXTERNAL_DIR}/tbb/debug/libtbbmalloc_proxy_debug.so"
      "${TRPG_THIRD_PARTY_EXTERNAL_DIR}/tbb/release/libtbbmalloc_proxy.so"
  )

  trpg_add_runtime_library_symlink(
      "${TRPG_THIRD_PARTY_EXTERNAL_DIR}/spdlog/debug/libspdlog.so.1"
      "libspdlog_debug.so"
  )
  trpg_add_runtime_library_symlink(
      "${TRPG_THIRD_PARTY_EXTERNAL_DIR}/spdlog/release/libspdlog.so.1"
      "libspdlog.so"
  )
  trpg_add_runtime_library_symlink(
      "${TRPG_THIRD_PARTY_EXTERNAL_DIR}/flatbuffers/debug/libflatbuffers.so.25.12.19"
      "libflatbuffers_debug.so"
  )
  trpg_add_runtime_library_symlink(
      "${TRPG_THIRD_PARTY_EXTERNAL_DIR}/flatbuffers/release/libflatbuffers.so.25.12.19"
      "libflatbuffers.so"
  )
  trpg_add_runtime_library_symlink(
      "${TRPG_THIRD_PARTY_EXTERNAL_DIR}/tbb/debug/libtbb.so.12"
      "libtbb_debug.so"
  )
  trpg_add_runtime_library_symlink(
      "${TRPG_THIRD_PARTY_EXTERNAL_DIR}/tbb/debug/libtbbmalloc.so.2"
      "libtbbmalloc_debug.so"
  )
  trpg_add_runtime_library_symlink(
      "${TRPG_THIRD_PARTY_EXTERNAL_DIR}/tbb/debug/libtbbmalloc_proxy.so.2"
      "libtbbmalloc_proxy_debug.so"
  )
  trpg_add_runtime_library_symlink(
      "${TRPG_THIRD_PARTY_EXTERNAL_DIR}/tbb/release/libtbb.so.12"
      "libtbb.so"
  )
  trpg_add_runtime_library_symlink(
      "${TRPG_THIRD_PARTY_EXTERNAL_DIR}/tbb/release/libtbbmalloc.so.2"
      "libtbbmalloc.so"
  )
  trpg_add_runtime_library_symlink(
      "${TRPG_THIRD_PARTY_EXTERNAL_DIR}/tbb/release/libtbbmalloc_proxy.so.2"
      "libtbbmalloc_proxy.so"
  )

  add_custom_target(trpg_prepare_runtime_libraries ALL
      ${TRPG_RUNTIME_LIBRARY_SYMLINK_COMMANDS}
      COMMENT "Preparing third-party runtime library SONAME symlinks"
      VERBATIM
  )
else()
  if (MSVC)
    message(FATAL_ERROR "This project is configured for the Linux/WSL toolchain. Select the Linux Debug or Linux Release CMake preset in Visual Studio instead of x64 Debug.")
  endif()

  trpg_add_imported_interface_library(spdlog)
  trpg_add_imported_interface_library(flatbuffers)
  trpg_add_imported_interface_library(tbb)
  trpg_add_imported_interface_library(tbbmalloc)
  trpg_add_imported_interface_library(tbbmalloc_proxy)
endif()

# Include sub-projects.
add_subdirectory ("src/net.common")
add_subdirectory ("src/net.core")
add_subdirectory ("src/net.server")
add_subdirectory ("src/net.server.test")

if (TARGET trpg_prepare_runtime_libraries)
  add_dependencies(net.common trpg_prepare_runtime_libraries)
  add_dependencies(net.server trpg_prepare_runtime_libraries)
  add_dependencies(net.server.test trpg_prepare_runtime_libraries)
endif()
