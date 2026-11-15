add_library(SceneGraph
    ClipNode.h
    ColorF.h
    DamageRegion.h
    DamageRegion.cpp
    Geometry.h
    Geometry.cpp
    GeometryNode.h
    LayerNode.h
    Material.h
    OpacityNode.h
    PointF.h
    RangeAllocator.h
    RangeAllocator.cpp
    RectF.h
    Renderer.h
    Renderer.cpp
    SceneNode.h
    SceneNode.cpp
    SceneTexture.h
    SceneTexture.cpp
    Transform2D.h
    TransformNode.h
    UpdatePass.h
    UpdatePass.cpp
)

target_link_libraries(SceneGraph PUBLIC RHI Core)

target_include_directories(SceneGraph
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

if (MANGO_RHI_VULKAN OR MANGO_RHI_METAL)
    mango_add_shaders(SceneGraph
        shaders/node.vert
        shaders/color.frag
        shaders/texture.frag
        shaders/composite.vert
        shaders/composite.frag
    )
    target_compile_definitions(SceneGraph PRIVATE MANGO_SCENEGRAPH_SHADERS=1)
endif ()
