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

    add_library(RHIVulkan OBJECT
        vulkan/VmaImplementation.cpp
        vulkan/VulkanHpp.h
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
        vulkan/VulkanComputeEncoder.h
        vulkan/VulkanComputeEncoder.cpp
        vulkan/VulkanCopyEncoder.h
        vulkan/VulkanCopyEncoder.cpp
        vulkan/VulkanTexture.h
        vulkan/VulkanTexture.cpp
        vulkan/VulkanRenderPass.h
        vulkan/VulkanRenderPass.cpp
        vulkan/VulkanBindGroup.h
        vulkan/VulkanBindGroup.cpp
        vulkan/VulkanPipeline.h
        vulkan/VulkanPipeline.cpp
        vulkan/VulkanRenderEncoder.h
        vulkan/VulkanRenderEncoder.cpp
        vulkan/VulkanSurface.h
        vulkan/VulkanSurface.cpp
        vulkan/VulkanSwapchain.h
        vulkan/VulkanSwapchain.cpp
    )

    target_include_directories(RHIVulkan PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/vulkan" "${CMAKE_CURRENT_SOURCE_DIR}")

    target_link_libraries(RHIVulkan PRIVATE Core mango::VulkanMemoryAllocator ${CMAKE_DL_LIBS})

    target_precompile_headers(RHIVulkan PRIVATE vulkan/VulkanHpp.h)
    if (MSVC)

        target_compile_options(RHIVulkan PRIVATE /bigobj)
    endif ()

    set_source_files_properties(vulkan/VmaImplementation.cpp TARGET_DIRECTORY RHIVulkan
                                PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

    if (WIN32)
        target_compile_definitions(RHIVulkan PRIVATE VK_USE_PLATFORM_WIN32_KHR WIN32_LEAN_AND_MEAN NOMINMAX)
    elseif (APPLE)
        target_compile_definitions(RHIVulkan PRIVATE VK_USE_PLATFORM_METAL_EXT)
    endif ()
    target_compile_definitions(RHIVulkan PRIVATE VK_NO_PROTOTYPES MANGO_RHI_VULKAN=1)

    target_link_libraries(RHI PRIVATE RHIVulkan)
    target_compile_definitions(RHI PUBLIC MANGO_RHI_VULKAN=1)

    if (MANGO_VULKAN_VALIDATION)

        add_dependencies(RHIVulkan mango_vulkan_validation_layers)
        target_compile_definitions(RHIVulkan PRIVATE "MANGO_VULKAN_LAYER_PATH=\"${MANGO_VULKAN_LAYER_PATH}\""
                                   MANGO_VULKAN_VALIDATION=1)
        target_compile_definitions(RHI PUBLIC MANGO_VULKAN_VALIDATION=1)
    endif ()

    target_include_directories(RHI PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/vulkan")
endif ()

if (MANGO_RHI_METAL)
    set(RHI_METAL_SOURCES
        metal/MetalCommon.h
        metal/MetalCommon.mm
        metal/MetalDeviceFactory.h
        metal/MetalDeviceFactory.mm
        metal/MetalInstance.h
        metal/MetalInstance.mm
        metal/MetalAdapter.h
        metal/MetalAdapter.mm
        metal/MetalDevice.h
        metal/MetalDevice.mm
        metal/MetalQueue.h
        metal/MetalQueue.mm
        metal/MetalFence.h
        metal/MetalFence.mm
        metal/MetalSemaphore.h
        metal/MetalSemaphore.mm
        metal/MetalBuffer.h
        metal/MetalBuffer.mm
        metal/MetalCommandBuffer.h
        metal/MetalCommandBuffer.mm
        metal/MetalCopyEncoder.h
        metal/MetalCopyEncoder.mm
        metal/MetalTexture.h
        metal/MetalTexture.mm
        metal/MetalRenderPass.h
        metal/MetalRenderPass.mm
        metal/MetalRenderEncoder.h
        metal/MetalRenderEncoder.mm
        metal/MetalSwapchain.h
        metal/MetalSwapchain.mm
        metal/MetalShader.h
        metal/MetalShader.mm
        metal/MetalPipeline.h
        metal/MetalPipeline.mm
        metal/MetalBindGroup.h
        metal/MetalBindGroup.mm
        metal/MetalComputeEncoder.h
        metal/MetalComputeEncoder.mm
    )
    target_sources(RHI PRIVATE ${RHI_METAL_SOURCES})

    target_include_directories(RHI PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/metal")

    set_source_files_properties(${RHI_METAL_SOURCES} PROPERTIES COMPILE_OPTIONS "-fobjc-arc")

    target_link_libraries(RHI PRIVATE "-framework Metal" "-framework QuartzCore" "-framework Foundation"
                          spirv-cross-msl)
    target_compile_definitions(RHI PUBLIC MANGO_RHI_METAL=1)
endif ()

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
