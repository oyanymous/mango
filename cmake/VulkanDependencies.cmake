# Vulkan build dependencies, fetched at pinned versions so every machine builds against the same code.
# No Vulkan SDK is needed: the loader (vulkan-1.dll / libvulkan) comes with the GPU driver and is
# loaded at runtime by volk, so nothing links against it.
#
# Defines:
#   Vulkan::Headers                 Khronos Vulkan headers
#   mango::volk                     volk header (VK_NO_PROTOTYPES); the implementation is compiled in src/rhi
#   mango::VulkanMemoryAllocator    VMA header; the implementation is compiled in src/rhi

include(FetchContent)

# Keep Vulkan-Headers and volk on the same SDK release.
set(MANGO_VULKAN_SDK_TAG vulkan-sdk-1.4.357.0)

FetchContent_Declare(
    VulkanHeaders
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG ${MANGO_VULKAN_SDK_TAG}
    GIT_SHALLOW TRUE
    SYSTEM
)

# volk and VMA are used header-only (their implementations are compiled in our own translation units),
# so SOURCE_SUBDIR points at a directory without a CMakeLists.txt to skip their own CMake projects.
FetchContent_Declare(
    volk
    GIT_REPOSITORY https://github.com/zeux/volk.git
    GIT_TAG ${MANGO_VULKAN_SDK_TAG}
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR do-not-add
)

FetchContent_Declare(
    VulkanMemoryAllocator
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG v3.5.0
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR do-not-add
)

FetchContent_MakeAvailable(VulkanHeaders volk VulkanMemoryAllocator)

add_library(mango_volk INTERFACE)
add_library(mango::volk ALIAS mango_volk)
target_include_directories(mango_volk SYSTEM INTERFACE "${volk_SOURCE_DIR}")
target_compile_definitions(mango_volk INTERFACE VK_NO_PROTOTYPES)
target_link_libraries(mango_volk INTERFACE Vulkan::Headers ${CMAKE_DL_LIBS})

add_library(mango_vma INTERFACE)
add_library(mango::VulkanMemoryAllocator ALIAS mango_vma)
target_include_directories(mango_vma SYSTEM INTERFACE "${vulkanmemoryallocator_SOURCE_DIR}/include")
# Vulkan functions are loaded by volk, so VMA must fetch them at runtime instead of linking them.
target_compile_definitions(mango_vma INTERFACE VMA_STATIC_VULKAN_FUNCTIONS=0 VMA_DYNAMIC_VULKAN_FUNCTIONS=1)
target_link_libraries(mango_vma INTERFACE mango::volk)
