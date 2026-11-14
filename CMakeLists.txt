cmake_minimum_required(VERSION 3.24)
# Oldest macOS the Metal backend supports (Metal 3). Must be set before project(); ignored on other systems.
# FORCE only replaces an empty value (which project() caches when none is given), never a developer's choice.
if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Minimum macOS version to target" FORCE)
endif()
project(Mango LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Windows builds use MSVC (the `windows` preset), the toolchain Khronos supports for the validation layer.
if(MSVC)
    set(MANGO_MSVC_MINIMUM_VERSION 19.40) # Visual Studio 2022 17.10
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS MANGO_MSVC_MINIMUM_VERSION)
        message(FATAL_ERROR "MSVC ${CMAKE_CXX_COMPILER_VERSION} is too old; Mango needs MSVC "
                            "${MANGO_MSVC_MINIMUM_VERSION} (Visual Studio 2022 17.10) or newer.")
    endif()
    # Standard-conforming mode, a correct __cplusplus value, and UTF-8 source and execution character sets.
    add_compile_options(/permissive- /Zc:__cplusplus /utf-8)
endif()

option(MANGO_RHI_VULKAN "Build the Vulkan RHI backend" ON)
option(MANGO_RHI_METAL "Build the Metal RHI backend (Apple only)" ${APPLE})
# Metal exists only on Apple systems; force off a value copied from another machine's cache or command line.
if(MANGO_RHI_METAL AND NOT APPLE)
    message(WARNING "Disabling MANGO_RHI_METAL: Metal is only available on Apple systems.")
    set(MANGO_RHI_METAL OFF CACHE BOOL "" FORCE)
endif()
# Off by default with MinGW: the layer's SPIRV-Tools dependency forces -static on MinGW, and CLion's bundled MinGW
# cannot link statically (missing __ms_vsnprintf / __intrinsic_setjmpex). Use the MSVC `windows` preset instead.
if(MINGW)
    set(mango_validation_default OFF)
else()
    set(mango_validation_default ON)
endif()
option(MANGO_VULKAN_VALIDATION "Build the Khronos validation layer from source (slow first build)"
       ${mango_validation_default})
# The default above does not change an ON value already in a CMake cache, so force it off here.
if(MINGW AND MANGO_VULKAN_VALIDATION)
    message(WARNING "Disabling MANGO_VULKAN_VALIDATION: the validation layer cannot be built with MinGW.")
    set(MANGO_VULKAN_VALIDATION OFF CACHE BOOL "" FORCE)
endif()
if(MANGO_RHI_VULKAN)
    include(cmake/VulkanDependencies.cmake)
    include(cmake/Shaders.cmake)
    if(MANGO_VULKAN_VALIDATION)
        include(cmake/VulkanValidationLayers.cmake)
    endif()
endif()

enable_testing()

# Point git at the repo's hooks (.githooks/: Conventional Commits message check
# and clang-format check) so every clone gets them on its first configure.
find_package(Git QUIET)
if(PROJECT_IS_TOP_LEVEL AND GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" config core.hooksPath .githooks
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE git_hooks_result)
    if(NOT git_hooks_result EQUAL 0)
        message(WARNING "Could not set core.hooksPath to .githooks")
    endif()
endif()

add_subdirectory(src)
add_subdirectory(samples)

option(MANGO_BUILD_DOCS "Add a 'docs' target that generates API documentation with Doxygen (downloads Doxygen)" OFF)
if(MANGO_BUILD_DOCS)
    include(cmake/Docs.cmake)
endif()
