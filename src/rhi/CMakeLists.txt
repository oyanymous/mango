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
    )

    # Vulkan stays a PRIVATE dependency: no Vulkan type may appear in a public RHI header.
    target_link_libraries(RHI PRIVATE mango::volk mango::VulkanMemoryAllocator)
    target_compile_definitions(RHI PUBLIC MANGO_RHI_VULKAN=1)
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
