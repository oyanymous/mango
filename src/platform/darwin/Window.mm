/**
 * @file
 * @brief macOS (AppKit) implementation of Window.
 */

#include "Window.h"

#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>
#include <QuartzCore/CAMetalLayer.h>
#include <memory>

#include "DarwinEventLoop.h"

namespace {

/**
 * @brief The client-area size of @p window in pixels; 0 x 0 while it is minimized (as on Win32).
 */
WindowSize pixelSize(NSWindow* window) {
    if ([window isMiniaturized]) {
        return {0, 0};
    }
    const NSRect frame = [[window contentView] frame];
    const CGFloat scale = [window backingScaleFactor];
    return {static_cast<int>(frame.size.width * scale), static_cast<int>(frame.size.height * scale)};
}

} // namespace

/**
 * @brief The NSWindow delegate: forwards size changes to the resize callback and quits the loop when the window
 * closes.
 */
@interface WindowDelegate : NSObject <NSWindowDelegate> {
@public
    std::function<void(WindowSize)>*
        m_resize_callback;       /**< The owning Window's callback; may hold an empty function. */
    CAMetalLayer* m_metal_layer; /**< The content view's layer; its scale follows the screen. */
}
@end

@implementation WindowDelegate

/**
 * @brief Calls the resize callback with the window's current pixel size.
 */
- (void)notifyResize:(NSWindow*)window {
    if (m_resize_callback && *m_resize_callback) {
        (*m_resize_callback)(pixelSize(window));
    }
}

- (void)windowDidResize:(NSNotification*)notification {
    [self notifyResize:[notification object]];
}

- (void)windowDidMiniaturize:(NSNotification*)notification {
    [self notifyResize:[notification object]];
}

- (void)windowDidDeminiaturize:(NSNotification*)notification {
    [self notifyResize:[notification object]];
}

- (void)windowDidChangeBackingProperties:(NSNotification*)notification {
    // Moving to a screen with another scale changes the pixel size without changing the window frame.
    NSWindow* window = [notification object];
    m_metal_layer.contentsScale = [window backingScaleFactor];
    [self notifyResize:window];
}

- (void)windowWillClose:(NSNotification*)notification {
    darwinRequestQuit();
}

@end

/**
 * @brief Native AppKit state of a Window.
 */
struct Window::Impl {
    NSWindow* m_window = nil;                          /**< The window; retained (manual reference counting). */
    WindowDelegate* m_window_delegate = nil;           /**< Its delegate; retained. */
    CAMetalLayer* m_metal_layer = nil;                 /**< The content view's backing layer; retained by the view. */
    std::function<void(WindowSize)> m_resize_callback; /**< Called on size changes; may be empty. */
};

Window::Window(const std::string& title) : m_impl(std::make_unique<Impl>()), m_window_title(title) {
    NSRect content_rect = NSMakeRect(400.0, 400.0, 400.0, 400.0);
    NSWindowStyleMask style_mask = NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskClosable |
                                   NSWindowStyleMaskResizable | NSWindowStyleMaskTitled;
    m_impl->m_window = [[NSWindow alloc] initWithContentRect:content_rect
                                                   styleMask:style_mask
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
    // Window owns the NSWindow and releases it in its destructor, also after the user closed it.
    [m_impl->m_window setReleasedWhenClosed:NO];
    if (!title.empty()) {
        setWindowTitle(title);
    }

    // A layer-hosting content view: set the layer before wantsLayer so AppKit uses it instead of making its own.
    m_impl->m_metal_layer = [CAMetalLayer layer];
    m_impl->m_metal_layer.contentsScale = [m_impl->m_window backingScaleFactor];
    NSView* content = [m_impl->m_window contentView];
    [content setLayer:m_impl->m_metal_layer];
    [content setWantsLayer:YES];

    m_impl->m_window_delegate = [[WindowDelegate alloc] init];
    m_impl->m_window_delegate->m_resize_callback = &m_impl->m_resize_callback;
    m_impl->m_window_delegate->m_metal_layer = m_impl->m_metal_layer;
    [m_impl->m_window setDelegate:m_impl->m_window_delegate];
}

void Window::display() {
    [m_impl->m_window makeKeyAndOrderFront:m_impl->m_window];
}

void Window::setWindowTitle(const std::string& window_title) {
    m_window_title = window_title;
    NSString* objc_window_title = [NSString stringWithUTF8String:window_title.c_str()];
    [m_impl->m_window setTitle:objc_window_title];
}

Window::~Window() {
    [m_impl->m_window setDelegate:nil];
    [m_impl->m_window_delegate release];
    [m_impl->m_window release];
}

NativeWindow Window::nativeWindow() const {
    return {.metal_layer = (__bridge void*)m_impl->m_metal_layer};
}

WindowSize Window::clientSize() const {
    return pixelSize(m_impl->m_window);
}

void Window::setResizeCallback(std::function<void(WindowSize)> callback) {
    m_impl->m_resize_callback = std::move(callback);
}
