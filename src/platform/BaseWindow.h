#pragma once

/**
 * @file
 * @brief The OS-independent window interface.
 */

#include <string>


/**
 * @brief The operations every native window supports. Window implements it per OS.
 */
class BaseWindow {
public:
    virtual ~BaseWindow() = default;

    /**
     * @brief Shows the window and brings it to the front.
     */
    virtual void display() = 0;

    /**
     * @brief Sets the text in the window's title bar.
     *
     * @param title UTF-8 title.
     */
    virtual void setWindowTitle(const std::string& title) = 0;
};
