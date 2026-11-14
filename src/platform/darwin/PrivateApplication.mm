/// @file
/// macOS (AppKit) implementation of PrivateApplication.

#include "PrivateApplication.h"

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
    NSApplication* m_app;                     ///< `[NSApplication sharedApplication]`.
    id<NSApplicationDelegate> m_app_delegate; ///< An AppDelegate; retained (manual reference counting).
};

PrivateApplication::PrivateApplication() : m_impl(std::make_unique<Impl>()) {
    // A regular activation policy gives the app a Dock icon and menu bar even without an app bundle.
    m_impl->m_app = [NSApplication sharedApplication];
    m_impl->m_app_delegate = [[AppDelegate alloc] init];
    [m_impl->m_app setDelegate:m_impl->m_app_delegate];
    [m_impl->m_app setActivationPolicy:NSApplicationActivationPolicy::NSApplicationActivationPolicyRegular];
}

PrivateApplication::~PrivateApplication() = default;

void PrivateApplication::run() {
    [m_impl->m_app run];
}
