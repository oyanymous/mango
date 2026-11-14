
add_library(UI Widget.h Widget.cpp
    Size.h
    Rect.h
    Rect.cpp
    Polygon.h
    Polygon.cpp
    Point.cpp
    Line.h
    Line.cpp
    Color.h
    Color.cpp
    Point.h)

# Widgets draw through the scene graph; Widget.h only forward-declares its types, but contentNode() hands out
# sg::Node, so users of UI get the SceneGraph headers too.
target_link_libraries(UI PUBLIC SceneGraph PRIVATE Platform Core)

target_include_directories(UI PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)