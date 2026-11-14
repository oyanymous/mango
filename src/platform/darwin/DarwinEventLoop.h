#pragma once

/**
 * @file
 * @brief Internal hooks between the macOS Window and PrivateApplication implementations.
 */

/**
 * @brief Makes PrivateApplication::run() return after the current iteration, and wakes it if it is blocked.
 *
 * @details
 * Called when a window closes, matching the Win32 backend, where destroying a window posts WM_QUIT. Main thread only.
 */
void darwinRequestQuit();
