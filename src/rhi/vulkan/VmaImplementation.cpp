/// @file
/// The single translation unit that compiles the Vulkan Memory Allocator implementation.
///
/// volk.h comes first so VMA sees the Vulkan declarations without prototypes (VK_NO_PROTOTYPES).
#include <volk.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
