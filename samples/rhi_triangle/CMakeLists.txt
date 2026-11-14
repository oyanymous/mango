# Windowed sample; needs a window system, so it is not registered with CTest.
add_executable(rhi_triangle main.cpp)
target_link_libraries(rhi_triangle PRIVATE RHI Platform)
mango_add_shaders(rhi_triangle shaders/triangle.vert shaders/triangle.frag)
