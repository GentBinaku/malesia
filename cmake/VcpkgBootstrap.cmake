# VcpkgBootstrap.cmake
# Automatically clones and bootstraps vcpkg if not present.
# Include this BEFORE project() in your root CMakeLists.txt.
#
# Usage:
#   include(cmake/VcpkgBootstrap.cmake)
#   vcpkg_bootstrap(BASELINE "2024.12.16")

include_guard(GLOBAL)

function(_vcpkg_bootstrap_run_git)
  cmake_parse_arguments(PARSE_ARGV 0 ARG "" "WORKING_DIRECTORY;ERROR_MESSAGE" "COMMAND")

  if(NOT ARG_WORKING_DIRECTORY)
    set(ARG_WORKING_DIRECTORY "${_VCPKG_DIR}")
  endif()

  execute_process(
        COMMAND ${ARG_COMMAND}
        WORKING_DIRECTORY "${ARG_WORKING_DIRECTORY}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

  if(NOT result EQUAL 0)
    message(FATAL_ERROR "${ARG_ERROR_MESSAGE}\n${error}")
  endif()

  set(_VCPKG_GIT_OUTPUT "${output}" PARENT_SCOPE)
endfunction()

function(_vcpkg_bootstrap_clone baseline)
  message(STATUS "[vcpkg] Cloning (baseline: ${baseline})...")

  file(MAKE_DIRECTORY "${_VCPKG_DIR}")

  _vcpkg_bootstrap_run_git(
        COMMAND ${GIT_EXECUTABLE} clone --depth 1 --branch ${baseline}
            https://github.com/microsoft/vcpkg.git "${_VCPKG_DIR}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        ERROR_MESSAGE "[vcpkg] Failed to clone"
    )
endfunction()

function(_vcpkg_bootstrap_get_current_tag out_var)
  execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --exact-match HEAD
        WORKING_DIRECTORY "${_VCPKG_DIR}"
        OUTPUT_VARIABLE tag
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE result
    )

  if(result EQUAL 0)
    set(${out_var} "${tag}" PARENT_SCOPE)
  else()
    set(${out_var} "" PARENT_SCOPE)
  endif()
endfunction()

function(_vcpkg_bootstrap_update current_tag new_tag)
  message(STATUS "[vcpkg] Updating from ${current_tag} to ${new_tag}...")

  _vcpkg_bootstrap_run_git(
        COMMAND ${GIT_EXECUTABLE} fetch origin tag ${new_tag} --no-tags
        ERROR_MESSAGE "[vcpkg] Failed to fetch tag ${new_tag}"
    )

  _vcpkg_bootstrap_run_git(
        COMMAND ${GIT_EXECUTABLE} checkout ${new_tag}
        ERROR_MESSAGE "[vcpkg] Failed to checkout tag ${new_tag}"
    )
endfunction()

function(_vcpkg_bootstrap_run)
  message(STATUS "[vcpkg] Bootstrapping...")

  if(WIN32)
    set(script "${_VCPKG_DIR}/bootstrap-vcpkg.bat")
    set(command cmd /c "${script}" -disableMetrics)
  else()
    set(script "${_VCPKG_DIR}/bootstrap-vcpkg.sh")
    set(command sh "${script}" -disableMetrics)
  endif()

  execute_process(
        COMMAND ${command}
        WORKING_DIRECTORY "${_VCPKG_DIR}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
    )

  if(NOT result EQUAL 0)
    message(FATAL_ERROR "[vcpkg] Bootstrap failed:\n${error}")
  endif()

  message(STATUS "[vcpkg] Ready")
endfunction()

# Public API
function(vcpkg_bootstrap)
  cmake_parse_arguments(PARSE_ARGV 0 ARG "" "BASELINE" "")

  if(NOT DEFINED ARG_BASELINE)
    message(FATAL_ERROR "vcpkg_bootstrap() requires BASELINE argument")
  endif()

  find_package(Git REQUIRED)

  set(_VCPKG_DIR "${CMAKE_SOURCE_DIR}/tools/vcpkg")
  set(needs_bootstrap FALSE)

  if(NOT EXISTS "${_VCPKG_DIR}/.git")
    _vcpkg_bootstrap_clone("${ARG_BASELINE}")
    set(needs_bootstrap TRUE)
  else()
    _vcpkg_bootstrap_get_current_tag(current_tag)

    if(NOT current_tag STREQUAL ARG_BASELINE)
      _vcpkg_bootstrap_update("${current_tag}" "${ARG_BASELINE}")
      set(needs_bootstrap TRUE)
    endif()
  endif()

  if(WIN32)
    set(vcpkg_executable "${_VCPKG_DIR}/vcpkg.exe")
  else()
    set(vcpkg_executable "${_VCPKG_DIR}/vcpkg")
  endif()

  if(needs_bootstrap OR NOT EXISTS "${vcpkg_executable}")
    _vcpkg_bootstrap_run()
  endif()
endfunction()

