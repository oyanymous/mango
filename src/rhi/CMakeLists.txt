set(RHI_PUBLIC_HEADERS
    Backend.h
    IAdapter.h
    IBindGroup.h
    IBindGroupLayout.h
    IBuffer.h
    ICommandBuffer.h
    IComputeEncoder.h
    ICopyEncoder.h
    IDevice.h
    IDeviceFactory.h
    IFence.h
    IInstance.h
    IPipeline.h
    IPipelineLayout.h
    IQueue.h
    IRenderEncoder.h
    IRenderPass.h
    IRenderTarget.h
    ISampler.h
    ISemaphore.h
    IShader.h
    ISurface.h
    ISwapchain.h
    ITexture.h
    ITextureView.h
    RHI.h
    RHIFactory.h
    RHIFwd.h
    RHITypes.h
    Result.h
)

add_library(RHI
    ${RHI_PUBLIC_HEADERS}
    Backend.cpp
    RHIFactory.cpp
    Result.cpp
)

target_link_libraries(RHI PUBLIC Core)

target_include_directories(RHI
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

if (MANGO_RHI_VULKAN)
    target_sources(RHI PRIVATE
        vulkan/VolkImplementation.cpp
        vulkan/VmaImplementation.cpp
        vulkan/VulkanCommon.h
        vulkan/VulkanCommon.cpp
        vulkan/VulkanDeviceFactory.h
        vulkan/VulkanDeviceFactory.cpp
        vulkan/VulkanInstance.h
        vulkan/VulkanInstance.cpp
        vulkan/VulkanAdapter.h
        vulkan/VulkanAdapter.cpp
        vulkan/VulkanDevice.h
        vulkan/VulkanDevice.cpp
        vulkan/VulkanQueue.h
        vulkan/VulkanQueue.cpp
        vulkan/VulkanFence.h
        vulkan/VulkanFence.cpp
        vulkan/VulkanSemaphore.h
        vulkan/VulkanSemaphore.cpp
        vulkan/VulkanBuffer.h
        vulkan/VulkanBuffer.cpp
        vulkan/VulkanCommandBuffer.h
        vulkan/VulkanCommandBuffer.cpp
        vulkan/VulkanCopyEncoder.h
        vulkan/VulkanCopyEncoder.cpp
        vulkan/VulkanTexture.h
        vulkan/VulkanTexture.cpp
        vulkan/VulkanRenderPass.h
        vulkan/VulkanRenderPass.cpp
        vulkan/VulkanPipeline.h
        vulkan/VulkanPipeline.cpp
        vulkan/VulkanRenderEncoder.h
        vulkan/VulkanRenderEncoder.cpp
        vulkan/VulkanSurface.h
        vulkan/VulkanSurface.cpp
        vulkan/VulkanSwapchain.h
        vulkan/VulkanSwapchain.cpp
    )

    # Backend headers are private to RHI and included bare ("VulkanInstance.h").
    target_include_directories(RHI PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/vulkan")

    # Vulkan stays a PRIVATE dependency: no Vulkan type may appear in a public RHI header.
    target_link_libraries(RHI PRIVATE mango::volk mango::VulkanMemoryAllocator)
    target_compile_definitions(RHI PUBLIC MANGO_RHI_VULKAN=1)
    # Surface creation needs the platform's WSI declarations, in every RHI translation unit (volk included).
    if (WIN32)
        target_compile_definitions(RHI PRIVATE VK_USE_PLATFORM_WIN32_KHR WIN32_LEAN_AND_MEAN NOMINMAX)
    elseif (APPLE)
        target_compile_definitions(RHI PRIVATE VK_USE_PLATFORM_METAL_EXT)
    endif ()

    if (MANGO_VULKAN_VALIDATION)
        # Instances created with enable_validation find the layer built from source here.
        add_dependencies(RHI mango_vulkan_validation_layers)
        target_compile_definitions(RHI PRIVATE "MANGO_VULKAN_LAYER_PATH=\"${MANGO_VULKAN_LAYER_PATH}\"")
        target_compile_definitions(RHI PUBLIC MANGO_VULKAN_VALIDATION=1)
    endif ()
endif ()

# Compiles every public header on its own (twice, to check #pragma once), so a header that misses
# an include fails the build. Only RHI's own include path is visible, which also proves that no
# public header needs Vulkan.
set(rhi_header_check_sources)
foreach (header IN LISTS RHI_PUBLIC_HEADERS)
    get_filename_component(header_name "${header}" NAME_WE)
    set(check_source "${CMAKE_CURRENT_BINARY_DIR}/header_check/${header_name}.cpp")
    file(CONFIGURE OUTPUT "${check_source}" CONTENT "#include \"${header}\"\n#include \"${header}\"\n")
    list(APPEND rhi_header_check_sources "${check_source}")
endforeach ()
add_library(RHIHeaderCheck OBJECT ${rhi_header_check_sources})
target_include_directories(RHIHeaderCheck PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(RHIHeaderCheck PRIVATE -Wall -Wextra -Wpedantic -Werror)
elseif (MSVC)
    target_compile_options(RHIHeaderCheck PRIVATE /W4 /WX)
endif ()
