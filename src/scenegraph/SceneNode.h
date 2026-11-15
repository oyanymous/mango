#pragma once

#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "Transform2D.h"

/// The scene graph: nodes describing 2D content, and the Renderer that draws them with the RHI.
namespace sg {

class Node;
class RootNode;
class LayerNode;
struct ClipContext;
struct UpdatePassAccess;

/// What changed on a node since the renderer last looked at it.
enum class DirtyFlags : std::uint32_t {
    None = 0,               /**< Nothing changed. */
    Matrix = 1u << 0,       /**< A TransformNode's matrix changed. */
    Opacity = 1u << 1,      /**< An OpacityNode's opacity changed. */
    Clip = 1u << 2,         /**< A ClipNode's rectangle changed. */
    Geometry = 1u << 3,     /**< A GeometryNode's vertices or indices changed. */
    Material = 1u << 4,     /**< A GeometryNode's material colour changed (same pipeline and texture). */
    MaterialType = 1u << 5, /**< A GeometryNode's material changed pipeline or texture: batches are rebuilt. */
    NodeAdded = 1u << 6,    /**< Set on a node that was just inserted into a tree. */
    NodeRemoved = 1u << 7,  /**< Set on a node that just lost a child. */
    Subtree = 1u << 8,      /**< Set on ancestors of a dirty node: something below changed. */
};

constexpr DirtyFlags operator|(DirtyFlags a, DirtyFlags b) {
    return static_cast<DirtyFlags>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}
constexpr DirtyFlags operator&(DirtyFlags a, DirtyFlags b) {
    return static_cast<DirtyFlags>(static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b));
}
constexpr DirtyFlags& operator|=(DirtyFlags& a, DirtyFlags b) {
    return a = a | b;
}
/// Whether @p value has at least one of @p flags.
constexpr bool hasAny(DirtyFlags value, DirtyFlags flags) {
    return (value & flags) != DirtyFlags::None;
}

/**
 * Receives notifications from a RootNode; implemented by the renderer.
 *
 * All callbacks run synchronously on the thread that changes the tree.
 */
class NodeObserver {
public:
    virtual ~NodeObserver() = default;
    /// Something in the tree became dirty (called at most once until the next update pass, usually).
    virtual void nodeDirty() = 0;
    /// @p subtree is about to be detached from the tree; it is still attached and intact during the call.
    virtual void nodeRemoved(Node& subtree) = 0;
    /// The root is being destroyed; no further callbacks follow.
    virtual void rootDestroyed() = 0;
};

/**
 * A scene graph node: owns its children and tracks what changed.
 *
 * Parents own their children (`std::unique_ptr`); a child is painted after its parent and after its earlier
 * siblings (painter's order). Setters of derived nodes call markDirty(), which flags the node and sets
 * DirtyFlags::Subtree on its ancestors, stopping at the first ancestor that already has it, and tells the root's
 * NodeObserver. The renderer's update pass (sg::UpdatePass) then visits only dirty paths and clears the flags.
 *
 * The world state (worldTransform(), worldOpacity(), clipContext(), layer()) is computed by the update pass and is
 * only valid after one ran.
 *
 * Nodes are not thread-safe; change a tree only from the thread that renders it.
 */
class Node {
public:
    /// The concrete kind of a node, for dispatch without RTTI.
    enum class Type {
        Basic, /**< A plain grouping node. */
        Root,
        Transform,
        Opacity,
        Clip,
        Geometry, /**< GeometryNode and its subclasses. */
        Layer,
    };

    /// Creates a node without children or parent.
    Node() : Node(Type::Basic) {}
    virtual ~Node();

    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    [[nodiscard]] Type type() const { return m_type; }
    /// The parent, or null for a root or a detached node.
    [[nodiscard]] Node* parent() const { return m_parent; }
    /// The children, in paint order.
    [[nodiscard]] const std::vector<std::unique_ptr<Node>>& children() const { return m_children; }
    /// The RootNode this node is attached to, or null when detached. O(depth).
    [[nodiscard]] RootNode* root();

    /**
     * Appends @p child as the last (topmost) child and takes ownership.
     *
     * @param child A detached node; must not be null or a RootNode.
     * @return The child, still owned by this node.
     */
    Node* appendChild(std::unique_ptr<Node> child);
    /**
     * Inserts @p child before @p before (painted below it), or last when @p before is null.
     *
     * @param child A detached node; must not be null or a RootNode.
     * @param before A child of this node, or null.
     * @return The child, still owned by this node.
     */
    Node* insertChildBefore(std::unique_ptr<Node> child, Node* before);
    /**
     * Creates a child of type @p T in place and appends it.
     *
     * @return The new child, owned by this node.
     */
    template <typename T, typename... Args>
    T* emplaceChild(Args&&... args) {
        static_assert(std::is_base_of_v<Node, T>, "T must derive from sg::Node");
        return static_cast<T*>(appendChild(std::make_unique<T>(std::forward<Args>(args)...)));
    }
    /**
     * Detaches @p child and hands back ownership; null if it is not a child of this node.
     *
     * The renderer is told first, so it can release the subtree's GPU data and redraw where it was.
     */
    std::unique_ptr<Node> removeChild(Node* child);

    /// Flags the node as changed and schedules an update; see the class description.
    void markDirty(DirtyFlags flags);
    /// The flags set since the last update pass.
    [[nodiscard]] DirtyFlags dirtyFlags() const { return m_dirty; }

    /// Transform from this node's coordinates to the pixels of its render target (canvas or layer).
    [[nodiscard]] const Transform2D& worldTransform() const { return m_world; }
    /// Product of every ancestor OpacityNode's opacity, up to the enclosing layer.
    [[nodiscard]] float worldOpacity() const { return m_world_opacity; }
    /// The clip applied to this node's drawing, or null for none.
    [[nodiscard]] const ClipContext* clipContext() const { return m_clip; }
    /// The LayerNode whose texture this node is drawn into, or null for the window canvas.
    [[nodiscard]] LayerNode* layer() const { return m_layer; }
protected:
    /// For derived classes: a node of the given kind.
    explicit Node(Type type) : m_type(type) {}
private:
    friend struct UpdatePassAccess;
    friend class RootNode;

    Type m_type;
    Node* m_parent = nullptr;
    std::vector<std::unique_ptr<Node>> m_children;
    DirtyFlags m_dirty = DirtyFlags::None;

    // World state, written by UpdatePass.
    Transform2D m_world;
    float m_world_opacity = 1.0f;
    const ClipContext* m_clip = nullptr;
    LayerNode* m_layer = nullptr;
};

/**
 * The top of a scene graph: the node a Renderer draws.
 *
 * Its observer (the Renderer that draws it) is told about changes. Destroying the root detaches it from its
 * renderer first.
 */
class RootNode : public Node {
public:
    RootNode() : Node(Type::Root) {}
    ~RootNode() override;

    /// Sets who is told about changes; null to stop notifications. Does not take ownership.
    void setObserver(NodeObserver* observer) { m_observer = observer; }
    /// The current observer, or null.
    [[nodiscard]] NodeObserver* observer() const { return m_observer; }
private:
    friend class Node;
    NodeObserver* m_observer = nullptr;
};

} // namespace sg
