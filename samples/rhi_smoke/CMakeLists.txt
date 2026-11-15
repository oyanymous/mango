add_executable(rhi_smoke main.cpp)

target_link_libraries(rhi_smoke PRIVATE RHI)

if (MANGO_RHI_VULKAN)

    target_link_libraries(rhi_smoke PRIVATE Vulkan::Headers)
    target_compile_definitions(rhi_smoke PRIVATE VK_NO_PROTOTYPES)
endif ()
if (MANGO_RHI_VULKAN OR MANGO_RHI_METAL)
    mango_add_shaders(rhi_smoke shaders/fill.comp shaders/fullscreen.vert shaders/red.frag)
endif ()

add_test(NAME rhi_smoke COMMAND rhi_smoke)
