#pragma once

#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "Transform2D.h"

namespace sg {

class Node;
class RootNode;
class LayerNode;
struct ClipContext;
struct UpdatePassAccess;

enum class DirtyFlags : std::uint32_t {
    None = 0,
    Matrix = 1u << 0,
    Opacity = 1u << 1,
    Clip = 1u << 2,
    Geometry = 1u << 3,
    Material = 1u << 4,
    MaterialType = 1u << 5,
    NodeAdded = 1u << 6,
    NodeRemoved = 1u << 7,
    Subtree = 1u << 8,
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

constexpr bool hasAny(DirtyFlags value, DirtyFlags flags) {
    return (value & flags) != DirtyFlags::None;
}

class NodeObserver {
public:
    virtual ~NodeObserver() = default;

    virtual void nodeDirty() = 0;

    virtual void nodeRemoved(Node& subtree) = 0;

    virtual void rootDestroyed() = 0;
};

class Node {
public:
    enum class Type {
        Basic,
        Root,
        Transform,
        Opacity,
        Clip,
        Geometry,
        Layer,
    };

    Node() : Node(Type::Basic) {}
    virtual ~Node();

    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    [[nodiscard]] Type type() const { return m_type; }

    [[nodiscard]] Node* parent() const { return m_parent; }

    [[nodiscard]] const std::vector<std::unique_ptr<Node>>& children() const { return m_children; }

    [[nodiscard]] RootNode* root();

    Node* appendChild(std::unique_ptr<Node> child);

    Node* insertChildBefore(std::unique_ptr<Node> child, Node* before);

    template <typename T, typename... Args>
    T* emplaceChild(Args&&... args) {
        static_assert(std::is_base_of_v<Node, T>, "T must derive from sg::Node");
        return static_cast<T*>(appendChild(std::make_unique<T>(std::forward<Args>(args)...)));
    }

    std::unique_ptr<Node> removeChild(Node* child);

    void markDirty(DirtyFlags flags);

    [[nodiscard]] DirtyFlags dirtyFlags() const { return m_dirty; }

    [[nodiscard]] const Transform2D& worldTransform() const { return m_world; }

    [[nodiscard]] float worldOpacity() const { return m_world_opacity; }

    [[nodiscard]] const ClipContext* clipContext() const { return m_clip; }

    [[nodiscard]] LayerNode* layer() const { return m_layer; }
protected:
    explicit Node(Type type) : m_type(type) {}
private:
    friend struct UpdatePassAccess;
    friend class RootNode;

    Type m_type;
    Node* m_parent = nullptr;
    std::vector<std::unique_ptr<Node>> m_children;
    DirtyFlags m_dirty = DirtyFlags::None;

    Transform2D m_world;
    float m_world_opacity = 1.0f;
    const ClipContext* m_clip = nullptr;
    LayerNode* m_layer = nullptr;
};

class RootNode : public Node {
public:
    RootNode() : Node(Type::Root) {}
    ~RootNode() override;

    void setObserver(NodeObserver* observer) { m_observer = observer; }

    [[nodiscard]] NodeObserver* observer() const { return m_observer; }
private:
    friend class Node;
    NodeObserver* m_observer = nullptr;
};

} // namespace sg
