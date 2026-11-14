/**
 * @file
 * @brief macOS (AppKit) implementation of Window.
 */

#include "Window.h"

#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>
#include <memory>

/**
 * @brief The NSWindow delegate. Empty for now; window events will be handled here.
 */
@interface WindowDelegate : NSObject <NSWindowDelegate>
@end

@implementation WindowDelegate
@end


/**
 * @brief Native AppKit state of a Window.
 */
struct Window::Impl {
    NSWindow* m_window;                /**< The window; retained (manual reference counting). */
    WindowDelegate* m_window_delegate; /**< Its delegate; retained. */
};

Window::Window(const std::string& title) : m_window_title(title), m_impl(std::make_unique<Impl>()) {
    NSRect content_rect = NSMakeRect(400.0, 400.0, 400.0, 400.0);
    NSWindowStyleMask style_mask = NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskClosable |
                                   NSWindowStyleMaskResizable | NSWindowStyleMaskTitled;
    m_impl->m_window = [[NSWindow alloc] initWithContentRect:content_rect
                                                   styleMask:style_mask
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
    m_impl->m_window_delegate = [[WindowDelegate alloc] init];
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

Window::~Window() = default;

// shortcut: no CAMetalLayer or resize notifications on macOS yet; add them with MoltenVK (first macOS build).
NativeWindow Window::nativeWindow() const {
    return {};
}

WindowSize Window::clientSize() const {
    const NSRect frame = [[m_impl->m_window contentView] frame];
    const CGFloat scale = [m_impl->m_window backingScaleFactor];
    return {static_cast<int>(frame.size.width * scale), static_cast<int>(frame.size.height * scale)};
}

void Window::setResizeCallback(std::function<void(WindowSize)>) {
}
