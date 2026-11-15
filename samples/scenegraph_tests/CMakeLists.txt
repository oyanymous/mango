add_executable(scenegraph_tests main.cpp)
target_link_libraries(scenegraph_tests PRIVATE SceneGraph)
add_test(NAME scenegraph_tests COMMAND scenegraph_tests)
