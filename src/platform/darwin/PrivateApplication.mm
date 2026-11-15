#include "PrivateApplication.h"

#include "DarwinEventLoop.h"

#include <AppKit/AppKit.h>
#include <memory>

@interface AppDelegate : NSObject <NSApplicationDelegate>
@end

@interface AppDelegate ()

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

struct PrivateApplication::Impl {
    NSApplication* m_app;
    id<NSApplicationDelegate> m_app_delegate;
};

PrivateApplication::PrivateApplication() : m_impl(std::make_unique<Impl>()) {
    m_impl->m_app = [NSApplication sharedApplication];
    m_impl->m_app_delegate = [[AppDelegate alloc] init];
    [m_impl->m_app setDelegate:m_impl->m_app_delegate];
    [m_impl->m_app setActivationPolicy:NSApplicationActivationPolicy::NSApplicationActivationPolicyRegular];
}

PrivateApplication::~PrivateApplication() = default;

namespace {

bool g_quit_requested = false;

}

void darwinRequestQuit() {
    g_quit_requested = true;

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
    [m_impl->m_app finishLaunching];
    [m_impl->m_app activateIgnoringOtherApps:YES];
    g_quit_requested = false;
    while (!g_quit_requested) {
        @autoreleasepool {
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
