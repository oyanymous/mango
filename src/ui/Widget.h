#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Color.h"
#include "IWidget.h"

class Window;

namespace sg {
class ClipNode;
class Node;
class RectangleNode;
class Renderer;
class RootNode;
class TransformNode;
} // namespace sg

class Widget : public IWidget {
public:
    explicit Widget(Widget* parent = nullptr);

    ~Widget() override;

    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;

    [[nodiscard]] bool isTopLevel() const { return m_parent == nullptr; }

    void display() override;

    void setWindowTitle(const std::string& title);

    [[nodiscard]] const std::string& getWindowTitle() const { return m_window_title; }

    void setGeometry(float x, float y, float width, float height);

    void setBackgroundColor(const Color& color);

    [[nodiscard]] sg::Node* contentNode() const;
private:
    Widget* m_parent = nullptr;
    std::vector<Widget*> m_children;
    std::unique_ptr<Window> m_window;
    std::string m_window_title;

    float m_x = 0.0f, m_y = 0.0f, m_width = 0.0f, m_height = 0.0f;
    std::unique_ptr<sg::RootNode> m_root;
    sg::TransformNode* m_node = nullptr;
    sg::RectangleNode* m_background = nullptr;
    sg::ClipNode* m_clip = nullptr;
    sg::Node* m_content = nullptr;
    std::unique_ptr<sg::Renderer> m_renderer;

    void updateNodes();

    void detachFromDestroyedParent();

    void createWindow();
};
