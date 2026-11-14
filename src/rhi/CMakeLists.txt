add_library(RHI
    Backend.h
    Backend.cpp
    Result.h
    Result.cpp
    RHITypes.h
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
