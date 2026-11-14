/**
 * @file
 * @brief The demo application: opens one top-level window and runs the event loop.
 */

#include "platform/Application.h"
#include "ui/Widget.h"


/**
 * @brief Creates the application and one titled top-level widget, then blocks in the event loop.
 *
 * @param argc Argument count (unused).
 * @param argv Arguments (unused).
 * @return 0 when the event loop ends.
 */
int main(int argc, char* argv[]) {
    Application app;

    Widget widget;
    widget.setWindowTitle("Parham is the best software developer in the world!");
    widget.display();

    app.run();

    return 0;
}
