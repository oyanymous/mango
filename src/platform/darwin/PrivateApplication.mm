/**
 * @file
 * @brief macOS (AppKit) implementation of PrivateApplication.
 */

#include "PrivateApplication.h"

#include "DarwinEventLoop.h"

#include <AppKit/AppKit.h>
#include <memory>


/// The NSApplication delegate. Currently only opts in to secure restorable state.
@interface AppDelegate : NSObject <NSApplicationDelegate>
@end

/// Private AppDelegate state.
@interface AppDelegate ()

/// Window outlet from the Xcode application template; unused (windows are created by Window).
@property (strong) IBOutlet NSWindow* window;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)aNotification {
}


- (void)applicationWillTerminate:(NSNotification*)aNotification {
}


- (BOOL)applicationSupportsSecureRestorableState:(NSApplication*)app {
    return YES;
}

@end

/// Native AppKit state of the application.
struct PrivateApplication::Impl {
    NSApplication* m_app;                     /**< `[NSApplication sharedApplication]`. */
    id<NSApplicationDelegate> m_app_delegate; /**< An AppDelegate; retained (manual reference counting). */
};

PrivateApplication::PrivateApplication() : m_impl(std::make_unique<Impl>()) {
    // A regular activation policy gives the app a Dock icon and menu bar even without an app bundle.
    m_impl->m_app = [NSApplication sharedApplication];
    m_impl->m_app_delegate = [[AppDelegate alloc] init];
    [m_impl->m_app setDelegate:m_impl->m_app_delegate];
    [m_impl->m_app setActivationPolicy:NSApplicationActivationPolicy::NSApplicationActivationPolicyRegular];
}

PrivateApplication::~PrivateApplication() = default;

namespace {

bool g_quit_requested = false; // set by darwinRequestQuit(); main thread only

} // namespace

void darwinRequestQuit() {
    g_quit_requested = true;
    // Wake a loop blocked in nextEventMatchingMask so it sees the flag.
    NSEvent* wake = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                       location:NSZeroPoint
                                  modifierFlags:0
                                      timestamp:0
                                   windowNumber:0
                                        context:nil
                                        subtype:0
                                          data1:0
                                          data2:0];
    [NSApp postEvent:wake atStart:NO];
}

void PrivateApplication::quit() {
    darwinRequestQuit();
}

void PrivateApplication::run() {
    // A hand-written loop instead of [NSApp run], so the frame callback can run between events (like the Win32
    // PeekMessage loop) and run() returns when a window closes.
    [m_impl->m_app finishLaunching];
    [m_impl->m_app activateIgnoringOtherApps:YES];
    g_quit_requested = false;
    while (!g_quit_requested) {
        // Drained every iteration: Metal drawables and other per-frame objects are autoreleased.
        @autoreleasepool {
            // Block for the first event only when there is no frame to render; then drain the rest.
            NSDate* until = frameDue() ? [NSDate distantPast] : [NSDate distantFuture];
            while (NSEvent* event = [m_impl->m_app nextEventMatchingMask:NSEventMaskAny
                                                               untilDate:until
                                                                  inMode:NSDefaultRunLoopMode
                                                                 dequeue:YES]) {
                [m_impl->m_app sendEvent:event];
                until = [NSDate distantPast];
            }
            [m_impl->m_app updateWindows];
            if (!g_quit_requested) {
                runFrameIfDue();
            }
        }
    }
}
