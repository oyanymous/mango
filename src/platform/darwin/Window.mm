#include "Window.h"

#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>
#include <QuartzCore/CAMetalLayer.h>
#include <memory>

#include "DarwinEventLoop.h"

namespace {

WindowSize pixelSize(NSWindow* window) {
    if ([window isMiniaturized]) {
        return {0, 0};
    }
    const NSRect frame = [[window contentView] frame];
    const CGFloat scale = [window backingScaleFactor];
    return {static_cast<int>(frame.size.width * scale), static_cast<int>(frame.size.height * scale)};
}

} // namespace

@interface WindowDelegate : NSObject <NSWindowDelegate> {
@public
    std::function<void(WindowSize)>* m_resize_callback;
    CAMetalLayer* m_metal_layer;
}
@end

@implementation WindowDelegate

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
    NSWindow* window = [notification object];
    m_metal_layer.contentsScale = [window backingScaleFactor];
    [self notifyResize:window];
}

- (void)windowWillClose:(NSNotification*)notification {
    darwinRequestQuit();
}

@end

struct Window::Impl {
    NSWindow* m_window = nil;
    WindowDelegate* m_window_delegate = nil;
    CAMetalLayer* m_metal_layer = nil;
    std::function<void(WindowSize)> m_resize_callback;
};

Window::Window(const std::string& title) : m_impl(std::make_unique<Impl>()), m_window_title(title) {
    NSRect content_rect = NSMakeRect(400.0, 400.0, 400.0, 400.0);
    NSWindowStyleMask style_mask = NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskClosable |
                                   NSWindowStyleMaskResizable | NSWindowStyleMaskTitled;
    m_impl->m_window = [[NSWindow alloc] initWithContentRect:content_rect
                                                   styleMask:style_mask
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];

    [m_impl->m_window setReleasedWhenClosed:NO];
    if (!title.empty()) {
        setWindowTitle(title);
    }

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

float Window::devicePixelRatio() const {
    return static_cast<float>([m_impl->m_window backingScaleFactor]);
}

void Window::setResizeCallback(std::function<void(WindowSize)> callback) {
    m_impl->m_resize_callback = std::move(callback);
}
