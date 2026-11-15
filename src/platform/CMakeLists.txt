
if (APPLE)
    set(PLATFORM_FILES
        darwin/DarwinEventLoop.h
        darwin/PrivateApplication.mm
        darwin/Window.mm)
elseif (WIN32)
    set(PLATFORM_FILES
        windows/PrivateApplication.cpp
        windows/Window.cpp
        windows/win32_helpers.h
        windows/win32_helpers.cpp)
endif ()

add_library(Platform
    ${PLATFORM_FILES}
    Application.h
    Application.cpp
    BaseApplication.h
    Window.h
    PrivateApplication.h
)

if (APPLE)
    target_link_libraries(Platform PUBLIC "-framework Cocoa" Core)
    # The content view hosts a CAMetalLayer for GPU surfaces.
    target_link_libraries(Platform PRIVATE "-framework QuartzCore")
elseif (WIN32)
    target_link_libraries(Platform PUBLIC user32 Core)
    # MinGW's headers default to an older Windows; GetDpiForWindow needs Windows 10.
    target_compile_definitions(Platform PRIVATE _WIN32_WINNT=0x0A00 WINVER=0x0A00)
endif ()

target_include_directories(Platform
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
