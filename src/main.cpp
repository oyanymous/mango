#include <array>

#include "GeometryNode.h"
#include "platform/Application.h"
#include "ui/Widget.h"

int main(int argc, char* argv[]) {
    Application app;

    Widget widget;
    widget.setWindowTitle("Parham is the best software developer in the world!");
    widget.setBackgroundColor(Color(245, 246, 250));

    Widget sidebar(&widget);
    sidebar.setGeometry(0, 0, 120, 2000);
    sidebar.setBackgroundColor(Color(40, 44, 52));

    Widget card(&widget);
    card.setGeometry(140, 20, 220, 140);
    card.setBackgroundColor(Color(255, 255, 255));

    Widget badge(&card);
    badge.setGeometry(150, -20, 100, 60);
    badge.setBackgroundColor(Color(230, 80, 60));

    Widget chart(&widget);
    chart.setGeometry(140, 180, 220, 100);
    chart.setBackgroundColor(Color(255, 255, 255));
    const std::array<sg::PointF, 6> points = {{{10, 80}, {50, 40}, {90, 60}, {130, 20}, {170, 50}, {210, 30}}};
    chart.contentNode()->emplaceChild<sg::GeometryNode>(sg::Geometry::polyline(points, 2.0f),
                                                        sg::Material::flatColor({0.2f, 0.4f, 0.9f, 1.0f}));

    widget.display();

    app.run();

    return 0;
}
