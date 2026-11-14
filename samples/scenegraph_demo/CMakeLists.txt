# Windowed scene graph sample; needs a window system, so it is not registered with CTest.
add_executable(scenegraph_demo main.cpp)
target_link_libraries(scenegraph_demo PRIVATE SceneGraph Platform)
