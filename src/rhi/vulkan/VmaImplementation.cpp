/**
 * @file
 * @brief The single translation unit that compiles the Vulkan Memory Allocator implementation.
 *
 * VMA includes the Vulkan C headers itself (VK_NO_PROTOTYPES is set for the target) and loads its functions
 * through the vkGetInstanceProcAddr / vkGetDeviceProcAddr that VulkanAdapter::createDevice passes it.
 */
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
