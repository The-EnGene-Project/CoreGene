#ifndef SCENE_H
#define SCENE_H
#pragma once

#include "gl_includes.h"
#include "polygon.h"
#include "geometry.h"
#include "shader.h"
#include "transform.h"
#include "error.h"
#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <memory>
#include <cmath>
#include <iostream>

#include "node.h"

// Encapsulamos toda a lógica de desenho em um namespace para organização.
namespace scene {

class SceneGraph;
using SceneGraphPtr = std::shared_ptr<SceneGraph>;

using namespace node;
using namespace shader;

class SceneGraph {
private:
    NodePtr root;
    ShaderPtr base_shader;
    std::map<std::string, NodePtr> name_map; // Mapa de nomes para nós
    std::map<int, NodePtr> node_map; // Mapa de IDs para nós
    NodePtr currentNode; // Nó atualmente selecionado
    transform::TransformPtr view_transform;

    
    SceneGraph(ShaderPtr base) {
        root = Node::Make("root", nullptr, base, transform::Transform::Make());
        base_shader = base;
        currentNode = root;
        name_map["root"] = root;
        node_map[root->getId()] = root;
        view_transform = transform::Transform::Make();
        view_transform->orthographic(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f); // Inicializa com ortográfica padrão
    }

    friend SceneGraphPtr graph();

    void registerNode(NodePtr node) {
        name_map[node->getName()] = node;
        node_map[node->getId()] = node;
        currentNode = node;
    }

protected:

    // BACALHAU
    NodePtr getRoot() const {
        return root;
    }

    NodePtr getCurrentNode() const {
        return currentNode;
    }

    NodePtr getNodeByName(const std::string& name) {
        auto it = name_map.find(name);
        if (it != name_map.end()) {
            currentNode = it->second;
            return currentNode;
        }
        return nullptr;
    }

    NodePtr getNodeById(int id) {
        auto it = node_map.find(id);
        if (it != node_map.end()) {
            currentNode = it->second;
            return currentNode;
        }
        return nullptr;
    }

public:

    void addNode(NodePtr node, NodePtr parent = nullptr) {
        if (!parent) {
            parent = root;
        }
        parent->addChild(node);
        registerNode(node);
    }

    void addNode(const std::string& name, GeometryPtr geometry = nullptr, ShaderPtr shader = nullptr, transform::TransformPtr transform = nullptr, NodePtr parent = nullptr) {
        if (name_map.find(name) != name_map.end()) {
            std::cerr << "Node with name " << name << " already exists!" << std::endl;
            return;
        }
        if (!transform) {
            transform = transform::Transform::Make();
        }
        NodePtr new_node = Node::Make(name, geometry, shader, transform);
        addNode(new_node, parent);
    }

    void addNodeToCurrent(const std::string& name, GeometryPtr geometry = nullptr, ShaderPtr shader = nullptr, transform::TransformPtr transform = nullptr) {
        addNode(name, geometry, shader, transform, currentNode);
    }

    void lookAtNode(const std::string& name) {
        NodePtr node = getNodeByName(name);
        if (node) {
            currentNode = node;
        } else {
            std::cerr << "Node with name " << name << " not found!" << std::endl;
        }
    }

    void lookAtNode(int id) {
        NodePtr node = getNodeById(id);
        if (node) {
            currentNode = node;
        } else {
            std::cerr << "Node with id " << id << " not found!" << std::endl;
        }
    }

    void setCurrentNodeShader(ShaderPtr shader) {
        currentNode->setShader(shader);
    }

    void setCurrentNodeGeometry(GeometryPtr geometry) {
        currentNode->setGeometry(geometry);
    }

    void setCurrentNodeTransform(transform::TransformPtr transform) {
        currentNode->setTransform(transform);
    }

    void moveCurrentNodeTo(NodePtr new_parent) {
        if (new_parent) {
            NodePtr old_parent = currentNode->getParent();
            if (old_parent) {
                old_parent->removeChild(currentNode);
            }
            new_parent->addChild(currentNode);
        } else {
            std::cerr << "New parent is null in moveCurrentNodeTo" << std::endl;
        }
    }

    void moveCurrentNodeTo(const std::string& new_parent_name) {
        NodePtr new_parent = getNodeByName(new_parent_name);
        if (new_parent) {
            NodePtr old_parent = currentNode->getParent();
            if (old_parent) {
                old_parent->removeChild(currentNode);
            }
            new_parent->addChild(currentNode);
        } else {
            std::cerr << "New parent with name " << new_parent_name << " not found in moveCurrentNodeTo" << std::endl;
        }
    }

    void moveToPositionUnderParent(const int position) {
        NodePtr parent = currentNode->getParent();
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
        currentNode->moveChild(from_idx, to_idx);
    }

    void swapChildren(const int idx1, const int idx2) {
        currentNode->swapChildren(idx1, idx2);
    }

    void renameCurrentNode(const std::string& new_name) {
        if (name_map.find(new_name) != name_map.end()) {
            std::cerr << "Node with name " << new_name << " already exists!" << std::endl;
            return;
        }
        name_map.erase(currentNode->getName());
        currentNode->setName(new_name);
        name_map[new_name] = currentNode;
    }

    void removeCurrentNode() {
        NodePtr parent = currentNode->getParent();
        if (parent) {
            parent->removeChild(currentNode);
        }
        name_map.erase(currentNode->getName());
        node_map.erase(currentNode->getId());
    }

    void duplicateNode(const std::string& name, const std::string& new_name) {
        NodePtr node = getNodeByName(name);
        if (node) {
            NodePtr new_node = Node::Make(
                new_name, 
                node->getGeometry(), 
                node->getShader(), 
                transform::Transform::Make(node->getTransform()->getMatrix()), 
                node->getParent()
            );
            node->getParent()->addChild(new_node);
            name_map[new_name] = new_node;
            node_map[new_node->getId()] = new_node;
            currentNode = new_node;
        } else {
            std::cerr << "Node with name " << name << " not found!" << std::endl;
        }
    }

    void addSibling(const std::string& name, GeometryPtr geometry = nullptr, ShaderPtr shader = nullptr, transform::TransformPtr transform = nullptr) {
        NodePtr parent = currentNode->getParent();
        if (parent) {
            addNode(name, geometry, shader, transform, parent);
        } else {
            std::cerr << "Current node has no parent!" << std::endl;
        }
    }

    void addSiblingAfter(NodePtr new_sibling, const std::string& node_to_add_after = "") {
        if (!node_to_add_after.empty()) {
            NodePtr after = getNodeByName(node_to_add_after);
            if (after) {
                NodePtr parent = after->getParent();
                if (parent) {
                    parent->addChildAfter(new_sibling, after);
                    registerNode(new_sibling);
                } else {
                    std::cerr << "Node to add after has no parent!" << std::endl;
                }
            } else {
                std::cerr << "Node to add after not found!" << std::endl;
            }
            return;
        }
        
        NodePtr parent = currentNode->getParent();
        if (parent) {
            NodePtr after = currentNode;
            parent->addChildAfter(new_sibling, after);
            registerNode(new_sibling);
        } else {
            std::cerr << "Current node has no parent!" << std::endl;
        }
    }

    void addSiblingAfter(const std::string& name, GeometryPtr geometry = nullptr, ShaderPtr shader = nullptr, transform::TransformPtr transform = nullptr, const std::string& node_to_add_after = "") {
        if (transform == nullptr) {
            transform = transform::Transform::Make();
        }
        addSiblingAfter(Node::Make(name, geometry, shader, transform), node_to_add_after);
    }

    // encapsula as funções do transform para o current node

    void translateCurrentNode(float delta_x, float delta_y, float delta_z) {
        currentNode->getTransform()->translate(delta_x, delta_y, delta_z);
    }

    void rotateCurrentNode(float angle, float axis_x, float axis_y, float axis_z) {
        currentNode->getTransform()->rotate(angle, axis_x, axis_y, axis_z);
    }

    void rotateCurrentNode(float angle) {
        rotateCurrentNode(angle, 0.0f, 0.0f, 1.0f);
    }

    void scaleCurrentNode(float x, float y, float z) {
        currentNode->getTransform()->scale(x, y, z);
    }

    void setTranslateCurrentNode(float x, float y, float z) {
        currentNode->getTransform()->setTranslate(x, y, z);
    }

    void setRotateCurrentNode(float angle, float axis_x, float axis_y, float axis_z) {
        currentNode->getTransform()->setRotate(angle, axis_x, axis_y, axis_z);
    }

    void setRotateCurrentNode(float angle) {
        setRotateCurrentNode(angle, 0.0f, 0.0f, 1.0f);
    }

    void setScaleCurrentNode(float x, float y, float z) {
        currentNode->getTransform()->setScale(x, y, z);
    }

    void resetTransformCurrentNode() {
        currentNode->getTransform()->reset();
    }

    void setTransformCurrentNode(glm::mat4 transform) {
        currentNode->getTransform()->setMatrix(transform);
    }

    void setTransformCurrentNode(transform::TransformPtr transform) {
        currentNode->setTransform(transform);
    }

    void newNodeAbove(const std::string& new_name) {
        NodePtr old_current = currentNode, old_parent = currentNode->getParent();
        addSiblingAfter(Node::Make(new_name));
        currentNode->addChild(old_current);
        old_parent->removeChild(old_current);
        old_current->setParent(currentNode);

        // alternative logic:
        // goes to parent finds the current node stores the current node 
        // then adds a new node where the current one was relative to the parent
        // and adds the previous current node as a child to the created node
        // NodePtr parent = currentNode->getParent();
        // if (parent) {
            
        //     int idx = -1;
        //     NodePtr new_node = Node::Make(name, nullptr, nullptr, transform::Transform::Make());
        //     idx = parent->getChildIndex(currentNode);
        //     if (idx != -1) {
        //         parent->removeChild(currentNode);
        //         parent->addChild(new_node, idx);
        //         new_node->addChild(currentNode);
        //         currentNode = new_node;
        //     } else {
        //         std::cerr << "Current node not found in parent's children!" << std::endl;
        //     }
        // } else {
        //     std::cerr << "Current node has no parent!" << std::endl;
        // }
    }


    void newNodeAbove() {
        std::string new_name = currentNode->getName() + "_parent";
        newNodeAbove(new_name);
    }
    






    void setView(float left, float right, float bottom, float top, float near, float far) {
        view_transform->orthographic(left, right, bottom, top, near, far);
    }

    void initializeBaseShader(const std::string& vertex_shader_file, const std::string& fragment_shader_file) {
        base_shader->AttachVertexShader(vertex_shader_file);
        base_shader->AttachFragmentShader(fragment_shader_file);
        base_shader->Link();
    }

    void clearGraph() {
        root = Node::Make("root", nullptr, base_shader, transform::Transform::Make());
        currentNode = root;
        name_map.clear();
        node_map.clear();
        name_map["root"] = root;
        node_map[root->getId()] = root;
    }

    

    void draw(bool print = false) {
        // Aplica a transformação de visão
        transform::stack()->push(view_transform->getMatrix());
        if (root) {
            root->draw(print);
        }
        transform::stack()->pop();
        if (print) printf("\n--------------------------------\n\n");
    }

    void drawSubtree(NodePtr node, bool print = false) {
        // Aplica a transformação de visão
        transform::stack()->push(view_transform->getMatrix());
        if (node) {
            node->draw(print);
        }
        transform::stack()->pop();
        if (print) printf("\n--------------------------------\n\n");
    }

    void drawSubtree(const std::string& node_name) {
        NodePtr node = getNodeByName(node_name);
        if (node) {
            drawSubtree(node);
        } else {
            std::cerr << "Node with name " << node_name << " not found!" << std::endl;
        }
    }

    void drawSubtree(const int node_id) {
        NodePtr node = getNodeById(node_id);
        if (node) {
            drawSubtree(node);
        } else {
            std::cerr << "Node with id " << node_id << " not found!" << std::endl;
        }
    }
};
    
inline SceneGraphPtr graph() {
    static ShaderPtr default_shader = Shader::Make();
    static SceneGraphPtr instance = SceneGraphPtr(new SceneGraph(default_shader));
    return instance;
}

}

#endif