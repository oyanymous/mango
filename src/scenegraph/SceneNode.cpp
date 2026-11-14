/**
 * @file
 * @brief sg::Node and sg::RootNode: tree operations and dirty propagation.
 */

#include "SceneNode.h"

#include <algorithm>
#include <cassert>

namespace sg {

Node::~Node() = default;

RootNode* Node::root() {
    Node* node = this;
    while (node->m_parent != nullptr) {
        node = node->m_parent;
    }
    return node->m_type == Type::Root ? static_cast<RootNode*>(node) : nullptr;
}

Node* Node::appendChild(std::unique_ptr<Node> child) {
    return insertChildBefore(std::move(child), nullptr);
}

Node* Node::insertChildBefore(std::unique_ptr<Node> child, Node* before) {
    assert(child && child->m_parent == nullptr && child->m_type != Type::Root);
    auto position = m_children.end();
    if (before != nullptr) {
        position = std::ranges::find(m_children, before, &std::unique_ptr<Node>::get);
        assert(position != m_children.end());
    }
    Node* raw = child.get();
    raw->m_parent = this;
    m_children.insert(position, std::move(child));
    // The new subtree's world state is unknown: NodeAdded makes the update pass recompute all of it.
    raw->markDirty(DirtyFlags::NodeAdded);
    return raw;
}

std::unique_ptr<Node> Node::removeChild(Node* child) {
    auto position = std::ranges::find(m_children, child, &std::unique_ptr<Node>::get);
    if (position == m_children.end()) {
        return nullptr;
    }
    // Tell the renderer while the subtree is still attached, so it can find what it drew.
    if (RootNode* top = root(); top != nullptr && top->m_observer != nullptr) {
        top->m_observer->nodeRemoved(*child);
    }
    std::unique_ptr<Node> owned = std::move(*position);
    m_children.erase(position);
    owned->m_parent = nullptr;
    markDirty(DirtyFlags::NodeRemoved);
    return owned;
}

void Node::markDirty(DirtyFlags flags) {
    m_dirty |= flags;
    Node* node = this;
    while (node->m_parent != nullptr) {
        node = node->m_parent;
        if (hasAny(node->m_dirty, DirtyFlags::Subtree)) {
            return; // an earlier change already marked the rest of the path, and told the observer
        }
        node->m_dirty |= DirtyFlags::Subtree;
    }
    if (node->m_type == Type::Root) {
        if (NodeObserver* observer = static_cast<RootNode*>(node)->m_observer) {
            observer->nodeDirty();
        }
    }
}

RootNode::~RootNode() {
    if (m_observer != nullptr) {
        m_observer->rootDestroyed();
    }
}

} // namespace sg
