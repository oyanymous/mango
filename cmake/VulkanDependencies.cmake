# Vulkan build dependencies, fetched at pinned versions so every machine builds against the same code.
# No Vulkan SDK is needed: the loader (vulkan-1.dll / libvulkan) comes with the GPU driver and is
# opened at runtime by Vulkan-Hpp's DynamicLoader (part of Vulkan-Headers), so nothing links against it.
#
# Defines:
#   Vulkan::Headers                 Khronos Vulkan headers, including Vulkan-Hpp (vulkan.hpp, vulkan_raii.hpp)
#   mango::VulkanMemoryAllocator    VMA header (VK_NO_PROTOTYPES); the implementation is compiled in src/rhi

include(FetchContent)

# Vulkan-Headers uses the SDK release in MANGO_VULKAN_SDK_TAG (set in the root CMakeLists.txt).

FetchContent_Declare(
    VulkanHeaders
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG ${MANGO_VULKAN_SDK_TAG}
    GIT_SHALLOW TRUE
    SYSTEM
)

# VMA is used header-only (its implementation is compiled in our own translation unit), so SOURCE_SUBDIR
# points at a directory without a CMakeLists.txt to skip its own CMake project.
FetchContent_Declare(
    VulkanMemoryAllocator
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG v3.5.0
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR do-not-add
)

FetchContent_MakeAvailable(VulkanHeaders VulkanMemoryAllocator)

add_library(mango_vma INTERFACE)
add_library(mango::VulkanMemoryAllocator ALIAS mango_vma)
target_include_directories(mango_vma SYSTEM INTERFACE "${vulkanmemoryallocator_SOURCE_DIR}/include")
# Nothing links against the loader, so VMA must fetch Vulkan functions at runtime through the pointers it is given.
target_compile_definitions(mango_vma INTERFACE VK_NO_PROTOTYPES VMA_STATIC_VULKAN_FUNCTIONS=0
                           VMA_DYNAMIC_VULKAN_FUNCTIONS=1)
target_link_libraries(mango_vma INTERFACE Vulkan::Headers)
