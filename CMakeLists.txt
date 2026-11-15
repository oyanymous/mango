cmake_minimum_required(VERSION 3.24)

if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Minimum macOS version to target" FORCE)
endif()
project(Mango LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    set(MANGO_MSVC_MINIMUM_VERSION 19.40)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS MANGO_MSVC_MINIMUM_VERSION)
        message(FATAL_ERROR "MSVC ${CMAKE_CXX_COMPILER_VERSION} is too old; Mango needs MSVC "
                            "${MANGO_MSVC_MINIMUM_VERSION} (Visual Studio 2022 17.10) or newer.")
    endif()

    add_compile_options(/permissive- /Zc:__cplusplus /utf-8)
endif()

option(MANGO_RHI_VULKAN "Build the Vulkan RHI backend" ON)
option(MANGO_RHI_METAL "Build the Metal RHI backend (Apple only)" ${APPLE})

if(MANGO_RHI_METAL AND NOT APPLE)
    message(WARNING "Disabling MANGO_RHI_METAL: Metal is only available on Apple systems.")
    set(MANGO_RHI_METAL OFF CACHE BOOL "" FORCE)
endif()

if(MINGW)
    set(mango_validation_default OFF)
else()
    set(mango_validation_default ON)
endif()
option(MANGO_VULKAN_VALIDATION "Build the Khronos validation layer from source (slow first build)"
       ${mango_validation_default})

if(MINGW AND MANGO_VULKAN_VALIDATION)
    message(WARNING "Disabling MANGO_VULKAN_VALIDATION: the validation layer cannot be built with MinGW.")
    set(MANGO_VULKAN_VALIDATION OFF CACHE BOOL "" FORCE)
endif()

set(MANGO_VULKAN_SDK_TAG vulkan-sdk-1.4.357.0)
if(MANGO_RHI_VULKAN)
    include(cmake/VulkanDependencies.cmake)
    if(MANGO_VULKAN_VALIDATION)
        include(cmake/VulkanValidationLayers.cmake)
    endif()
endif()
if(MANGO_RHI_METAL)
    include(cmake/MetalDependencies.cmake)
endif()

if(MANGO_RHI_VULKAN OR MANGO_RHI_METAL)
    include(cmake/Shaders.cmake)
endif()

enable_testing()

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
