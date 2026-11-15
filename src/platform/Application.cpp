#include "Application.h"

Application::Application() {
    PrivateApplication::instance();
}

void Application::run() {
    PrivateApplication::instance().run();
    for (IWidget* widget : PrivateApplication::instance().topLevelWidgets()) {}
}
