# Headless checks of the RHI. Runs every check and exits non-zero if any failed.
add_executable(rhi_smoke main.cpp)

target_link_libraries(rhi_smoke PRIVATE RHI)

if (MANGO_RHI_VULKAN)
    # Only the smoke test talks to Vulkan directly, to report the header and loader versions.
    target_link_libraries(rhi_smoke PRIVATE mango::volk)
endif ()
if (MANGO_RHI_VULKAN OR MANGO_RHI_METAL)
    mango_add_shaders(rhi_smoke shaders/fill.comp)
endif ()

add_test(NAME rhi_smoke COMMAND rhi_smoke)
