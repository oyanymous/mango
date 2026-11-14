add_subdirectory(rhi_smoke)
if (MANGO_RHI_VULKAN OR MANGO_RHI_METAL)
    add_subdirectory(rhi_triangle)
endif ()
