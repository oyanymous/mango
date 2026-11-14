cmake_minimum_required(VERSION 3.24)
project(Mango LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MANGO_RHI_VULKAN "Build the Vulkan RHI backend" ON)
if(MANGO_RHI_VULKAN)
    include(cmake/VulkanDependencies.cmake)
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
