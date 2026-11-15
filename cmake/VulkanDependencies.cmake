include(FetchContent)

FetchContent_Declare(
    VulkanHeaders
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG ${MANGO_VULKAN_SDK_TAG}
    GIT_SHALLOW TRUE
    SYSTEM
)

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

target_compile_definitions(mango_vma INTERFACE VK_NO_PROTOTYPES VMA_STATIC_VULKAN_FUNCTIONS=0
                           VMA_DYNAMIC_VULKAN_FUNCTIONS=1)
target_link_libraries(mango_vma INTERFACE Vulkan::Headers)
