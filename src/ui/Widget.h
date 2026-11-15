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

/**
 * The basic UI element: a rectangle with a background colour that clips its children. A widget without a
 * parent is a top-level widget with its own native Window.
 *
 * Top-level widgets register themselves with the Application when constructed. The native window, and the
 * sg::Renderer that draws the widget tree into it, are created on the first display().
 *
 * Every widget owns a small scene graph subtree: a TransformNode at its position, holding its background rectangle
 * and a ClipNode (the widget's bounds) for its content and its child widgets. Changing a widget only marks its own
 * nodes dirty, so the renderer redraws just that widget's pixels. The application renders on demand: nothing is drawn
 * while nothing changes.
 *
 * A child widget must not outlive its parent's event loop; destroying a parent detaches its children (they stop
 * being drawn).
 */
class Widget : public IWidget {
public:
    /**
     * Creates a widget.
     *
     * @param parent Parent widget, or null for a top-level widget. Not owned.
     */
    explicit Widget(Widget* parent = nullptr);
    /// Removes the widget (and its children's drawing) from the screen; a top-level widget closes its window.
    ~Widget() override;

    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;

    /// True if the widget has no parent and therefore gets its own window.
    [[nodiscard]] bool isTopLevel() const { return m_parent == nullptr; }
    /// Shows the widget. A top-level widget creates its native window and renderer on the first call.
    void display() override;
    /**
     * Sets the window title. Applied now if the window exists, otherwise when it is created.
     *
     * @param title UTF-8 title.
     */
    void setWindowTitle(const std::string& title);
    /// The window title last set with setWindowTitle().
    [[nodiscard]] const std::string& getWindowTitle() const { return m_window_title; }

    /**
     * Moves and resizes the widget, in logical units relative to its parent's top-left corner.
     *
     * A top-level widget ignores the position and always fills its window; the size is used until the window
     * reports its own.
     */
    void setGeometry(float x, float y, float width, float height);
    /// Sets the background colour (transparent by default for child widgets, white for top-level ones).
    void setBackgroundColor(const Color& color);

    /// The node to add custom scene graph content to: drawn above the background and below child widgets, in widget
    /// coordinates, clipped to the widget.
    [[nodiscard]] sg::Node* contentNode() const;
private:
    Widget* m_parent = nullptr;
    std::vector<Widget*> m_children;
    std::unique_ptr<Window> m_window;
    std::string m_window_title;

    float m_x = 0.0f, m_y = 0.0f, m_width = 0.0f, m_height = 0.0f;
    std::unique_ptr<sg::RootNode> m_root; // top-level widgets only
    sg::TransformNode* m_node = nullptr;  // owned by the parent's clip node, or by m_root
    sg::RectangleNode* m_background = nullptr;
    sg::ClipNode* m_clip = nullptr;
    sg::Node* m_content = nullptr;
    std::unique_ptr<sg::Renderer> m_renderer; // top-level widgets, once displayed

    /// Applies the geometry to the nodes.
    void updateNodes();
    /// Forgets the nodes after the parent's subtree (which owned them) was destroyed.
    void detachFromDestroyedParent();
    /// Creates the window, surface and renderer of a top-level widget.
    void createWindow();
};
