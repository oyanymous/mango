# Metal backend build dependencies, fetched at pinned versions.
#
# Defines:
#   spirv-cross-msl    SPIRV-Cross (static), which translates the SPIR-V the RHI takes into Metal Shading Language at
#                      pipeline creation. On the same SDK release as the other Khronos projects (MANGO_VULKAN_SDK_TAG).

include(FetchContent)

# Only the libraries the MSL backend needs: core, GLSL (its base class) and MSL.
set(SPIRV_CROSS_CLI OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_GLSL ON CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_MSL ON CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_HLSL OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_CPP OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_REFLECT OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_C_API OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_UTIL OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_SHARED OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_STATIC ON CACHE BOOL "" FORCE)
set(SPIRV_CROSS_SKIP_INSTALL ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    SPIRV-Cross
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Cross.git
    GIT_TAG ${MANGO_VULKAN_SDK_TAG}
    GIT_SHALLOW TRUE
    SYSTEM
)
FetchContent_MakeAvailable(SPIRV-Cross)
