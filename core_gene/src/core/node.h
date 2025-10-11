#ifndef NODE_H
#define NODE_H
#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <utility>
#include <algorithm>
#include <iostream>

namespace node {

/**
 * @class Node
 * @brief A generic, templated class for representing a node in a tree structure.
 *
 * This class manages the hierarchical relationships (parent/child) and contains
 * a user-defined 'PayloadType'. It is responsible for traversal logic but delegates
 * the specific actions performed during traversal to the caller via the visit() method.
 *
 * @tparam PayloadType The type of data or behavior object this node will contain.
 */
template <typename PayloadType>
class Node : public std::enable_shared_from_this<Node<PayloadType>> {
public:
    // Type aliases for cleaner code
    using NodePtr = std::shared_ptr<Node<PayloadType>>;
    using ParentPtr = std::weak_ptr<Node<PayloadType>>;

private:
    int id;
    inline static int next_id = 0;
    std::string name;
    std::vector<NodePtr> children;
    ParentPtr parent;
    bool applicability = true;

    // The node now stores its own behavior (its "strategy")
    std::function<void(Node<PayloadType>&)> pre_visit_action_;
    std::function<void(Node<PayloadType>&)> post_visit_action_;
    PayloadType payload_;

    // Private constructor to enforce creation via the static Make() function.
    explicit Node(std::string name) : name(std::move(name)) {
        id = next_id++;
    }

    // Private setter for parent to be controlled by child management methods.
    void setParent(ParentPtr new_parent) {
        parent = new_parent;
    }

public:
    /**
     * @brief Factory function to create a new Node.
     * @param name The name of the node.
     * @return A shared pointer to the newly created Node.
     */
    static NodePtr Make(std::string name) {
        return NodePtr(new Node<PayloadType>(std::move(name)));
    }

    // --- Payload Access ---
    /**
     * @brief Gets a mutable reference to the node's payload.
     */
    PayloadType& payload() { return payload_; }

    /**
     * @brief Gets a constant reference to the node's payload.
     */
    const PayloadType& payload() const { return payload_; }

    // --- Core Getters ---
    int getId() const { return id; }
    const std::string& getName() const { return name; }
    NodePtr getParent() const { return parent.lock(); }
    bool getApplicability() const { return applicability; }

    // --- Core Setters ---
    void setName(const std::string& new_name) { name = new_name; }
    void setApplicability(bool new_applicability) { applicability = new_applicability; }

    // --- Hierarchy Management ---
    
    int getChildCount() const {
        return children.size();
    }

    NodePtr getChild(int index) const {
        if (index < 0 || index >= children.size()) {
            std::cerr << "Index out of bounds in getChild" << std::endl;
            return nullptr;
        }
        return children[index];
    }

    NodePtr getChildByName(const std::string& child_name) const {
        for (const NodePtr& child : children) {
            if (child->getName() == child_name) {
                return child;
            }
        }
        // This is a common lookup, returning nullptr is sufficient.
        // A loud error can be added by the caller if needed.
        return nullptr;
    }

    int getChildIndex(const NodePtr& child) const {
        for (int i = 0; i < children.size(); i++) {
            if (children[i] == child) {
                return i;
            }
        }
        return -1; // Not found
    }

    void addChild(const NodePtr& child) {
        if (child) {
            children.push_back(child);
            child->setParent(this->shared_from_this());
        }
    }

    void addChild(const NodePtr& child, int index) {
        if (!child) return;
        if (index < 0 || index > children.size()) {
            std::cerr << "Index out of bounds in addChild" << std::endl;
            return;
        }
        children.insert(children.begin() + index, child);
        child->setParent(this->shared_from_this());
    }

    void addChildFront(const NodePtr& child) {
        if (child) {
            children.insert(children.begin(), child);
            child->setParent(this->shared_from_this());
        }
    }

    void addChildAfter(const NodePtr& child, const NodePtr& after) {
        if (!child || !after) return;
        auto it = std::find(children.begin(), children.end(), after);
        if (it != children.end()) {
            children.insert(it + 1, child);
            child->setParent(this->shared_from_this());
        } else {
            std::cerr << "Reference child not found in addChildAfter" << std::endl;
        }
    }

    void moveChild(int from_idx, int to_idx) {
        if (from_idx < 0 || from_idx >= children.size() || to_idx < 0 || to_idx >= children.size()) {
            std::cerr << "Index out of bounds in moveChild" << std::endl;
            return;
        }
        NodePtr child_to_move = children[from_idx];
        children.erase(children.begin() + from_idx);
        children.insert(children.begin() + to_idx, child_to_move);
    }

    void swapChildren(int idx1, int idx2) {
        if (idx1 < 0 || idx1 >= children.size() || idx2 < 0 || idx2 >= children.size()) {
            std::cerr << "Index out of bounds in swapChildren" << std::endl;
            return;
        }
        std::swap(children[idx1], children[idx2]);
    }

    void removeChild(const NodePtr& child) {
        if (!child) return;
        auto it = std::find(children.begin(), children.end(), child);
        if (it != children.end()) {
            (*it)->setParent(nullptr);
            children.erase(it);
        } else {
            std::cerr << "Child not found in removeChild" << std::endl;
        }
    }



    // --- Behavior Management API ---

    /**
     * @brief Sets the action to be performed on this node BEFORE visiting children.
     * @param action The function to execute.
     */
    void onPreVisit(const std::function<void(Node<PayloadType>&)>& action) {
        pre_visit_action_ = action;
    }

    /**
     * @brief Sets the action to be performed on this node AFTER visiting children.
     * @param action The function to execute.
     */
    void onPostVisit(const std::function<void(Node<PayloadType>&)>& action) {
        post_visit_action_ = action;
    }

    /**
     * @brief Detaches all actions from this node.
     */
    void clearActions() {
        pre_visit_action_ = nullptr;
        post_visit_action_ = nullptr;
    }

    // --- Generic Traversal Method ---
    /**
     * @brief Traverses this node and its children, executing their stored actions.
     *
     * This method defines the traversal order:
     * 1. Perform pre-visit action on the current node.
     * 2. Recursively visit all children.
     * 3. Perform post-visit action on the current node.
     *
     */
    void visit() {
        if (!applicability) return;

        // 1. Execute the stored pre-order action, if it exists
        if (pre_visit_action_) {
            pre_visit_action_(*this);
        }

        // 2. Recursively visit children
        for (const auto& child : children) {
            if (child) {
                child->visit();
            }
        }

        // 3. Execute the stored post-order action, if it exists
        if (post_visit_action_) {
            post_visit_action_(*this);
        }
    }
};

} // namespace node

#endif // NODE_H