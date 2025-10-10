#ifndef SCENE_VISITOR_H
#define SCENE_VISITOR_H
#pragma once

#include "scene.h"

namespace scene {

class SceneGraphVisitor {
private:
    inline static SceneGraphPtr sg = graph();
    node::NodePtr currentNode;
    
public:
    SceneGraphVisitor() {
        currentNode = sg->getRoot();
    }

    node::NodePtr getCurrentNode() const {
        return currentNode;
    }

    void goToRoot() {
        currentNode = sg->getRoot();
    }

    void lookAtNode(const std::string& name) {
        node::NodePtr node = sg->getNodeByName(name);
        if (node) {
            currentNode = node;
        } else {
            std::cerr << "Node with name " << name << " not found!" << std::endl;
        }
    }

    void lookAtNode(int id) {
        node::NodePtr node = sg->getNodeById(id);
        if (node) {
            currentNode = node;
        } else {
            std::cerr << "Node with id " << id << " not found!" << std::endl;
        }
    }

    void addNode(node::NodePtr new_node, node::NodePtr parent = nullptr) {
        if (!parent) {
            parent = currentNode;
        }
        sg->addNode(new_node, parent);
        currentNode = new_node;
    }

    void addNode(const std::string& name, node::NodePtr parent = nullptr) {
        if (!parent) {
            parent = currentNode;
        }
        node::NodePtr new_node = sg->addNode(name, parent);
        if (new_node) {
            currentNode = new_node;
        }
    }
    
    void addNodeToCurrent(const std::string& name) {
        if (!currentNode) {
            std::cerr << "Current node is null!" << std::endl;
            return;
        }
        addNode(name, currentNode);
    }
    
    void addComponentToCurrentNode(component::ComponentPtr new_component) {
        if (!currentNode) {
            std::cerr << "Current node is null!" << std::endl;
            return;
        }
        currentNode->addComponent(new_component);
    }

    void moveCurrentNodeTo(node::NodePtr new_parent) {
        if (!currentNode || !new_parent) {
            std::cerr << "Current node or new parent is null in moveCurrentNodeTo" << std::endl;
            return;
        }
        node::NodePtr old_parent = currentNode->getParent();
        if (old_parent) {
            old_parent->removeChild(currentNode);
        }
        new_parent->addChild(currentNode);
    }

    void moveCurrentNodeTo(const std::string& new_parent_name) {
        node::NodePtr new_parent = sg->getNodeByName(new_parent_name);
        if (new_parent) {
            moveCurrentNodeTo(new_parent);
        } else {
            std::cerr << "New parent with name " << new_parent_name << " not found in moveCurrentNodeTo" << std::endl;
        }
    }

    void moveToPositionUnderParent(const int position) {
        if (!currentNode) {
            std::cerr << "Current node is null!" << std::endl;
            return;
        }
        node::NodePtr parent = currentNode->getParent();
        if (parent) {
            if (position < 0 || position >= parent->getChildCount()) {
                std::cerr << "Position out of bounds in moveToPositionUnderParent" << std::endl;
                return;
            }
            parent->moveChild(parent->getChildIndex(currentNode), position);
        } else {
            std::cerr << "Current node has no parent in moveToPositionUnderParent" << std::endl;
        }
    }

    void moveChild(const int from_idx, const int to_idx) {
        if (!currentNode) {
            std::cerr << "Current node is null!" << std::endl;
            return;
        }
        currentNode->moveChild(from_idx, to_idx);
    }

    void swapChildren(const int idx1, const int idx2) {
        if (!currentNode) {
            std::cerr << "Current node is null!" << std::endl;
            return;
        }
        currentNode->swapChildren(idx1, idx2);
    }

    void renameCurrentNode(const std::string& new_name) {
        sg->renameNode(currentNode, new_name);
    }

    void removeCurrentNode() {
        if (!currentNode || currentNode == sg->getRoot()) {
            std::cerr << "Cannot remove current node (it's null or root)!" << std::endl;
            return;
        }
        node::NodePtr parent = currentNode->getParent();
        sg->removeNode(currentNode);
        currentNode = parent; // Após a exclusão, o nó atual se torna o pai
    }

    void duplicateCurrentNode(const std::string& new_name) {
        if (!currentNode) {
            std::cerr << "Current node is null!" << std::endl;
            return;
        }
        node::NodePtr new_node = sg->duplicateNode(currentNode->getName(), new_name);
        if (new_node) {
            currentNode = new_node;
        }
    }
    
    void addSibling(const std::string& name) {
        if (!currentNode) {
            std::cerr << "Current node is null!" << std::endl;
            return;
        }
        node::NodePtr parent = currentNode->getParent();
        if (parent) {
            addNode(name, parent);
        } else {
            std::cerr << "Current node has no parent!" << std::endl;
        }
    }

    void addSiblingAfter(node::NodePtr new_sibling, node::NodePtr after_node) {
        if (!new_sibling || !after_node) {
            std::cerr << "Sibling or after_node is null" << std::endl;
            return;
        }
        node::NodePtr parent = after_node->getParent();
        if (parent) {
            parent->addChildAfter(new_sibling, after_node);
            sg->registerNode(new_sibling); // Deve registrar manualmente, pois não passa por sg->addNode
            currentNode = new_sibling;
        } else {
            std::cerr << "Node to add after has no parent!" << std::endl;
        }
    }

    void addSiblingAfterCurrent(node::NodePtr new_sibling) {
        if (!currentNode) {
            std::cerr << "Current node is null!" << std::endl;
            return;
        }
        addSiblingAfter(new_sibling, currentNode);
    }
    
    void addSiblingAfterCurrent(const std::string& new_sibling_name) {
        if (!currentNode) {
            std::cerr << "Current node is null!" << std::endl;
            return;
        }
        if (sg->getNodeByName(new_sibling_name)) {
            std::cerr << "Node with name " << new_sibling_name << " already exists!" << std::endl;
            return;
        }
        addSiblingAfter(node::Node::Make(new_sibling_name), currentNode);
    }
    
    void newNodeAbove(const std::string& new_name) {
        if (!currentNode || currentNode == sg->getRoot()) {
            std::cerr << "Cannot create a node above the current node (null or root)." << std::endl;
            return;
        }
        
        node::NodePtr old_parent = currentNode->getParent();
        if (!old_parent) return;

        int original_idx = old_parent->getChildIndex(currentNode);
        if (original_idx == -1) return;

        // Adiciona o novo nó ao pai, ele será adicionado no final
        node::NodePtr new_node = sg->addNode(new_name, old_parent);
        if (!new_node) return;

        // Move o novo nó para a posição original do currentNode
        old_parent->moveChild(old_parent->getChildIndex(new_node), original_idx);
        
        // Move o currentNode para ser filho do novo nó
        old_parent->removeChild(currentNode);
        new_node->addChild(currentNode);

        currentNode = new_node;
    }

    void newNodeAbove() {
        if (!currentNode) return;
        std::string new_name = currentNode->getName() + "_parent";
        while (sg->getNodeByName(new_name)) {
            new_name += "_new";
        }
        newNodeAbove(new_name);
    }
};

}

#endif
