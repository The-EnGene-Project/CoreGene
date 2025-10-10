#ifndef GEOMETRY_COMPONENT_H
#define GEOMETRY_COMPONENT_H
#pragma once

#include <memory>
#include "component.h"
#include "../gl_base/geometry.h"
#include "../gl_base/shader.h"

namespace component {

class GeometryComponent;
using GeometryComponentPtr = std::shared_ptr<GeometryComponent>;

class GeometryComponent : virtual public Component {
private:
    geometry::GeometryPtr m_geometry;
protected:

    GeometryComponent(geometry::GeometryPtr g) :
        Component(ComponentPriority::GEOMETRY),
        m_geometry(g)
        {}

public:

    static GeometryComponentPtr Make(geometry::GeometryPtr g) {
        return GeometryComponentPtr(new GeometryComponent(g));
    }
    
    virtual void apply() override {
        shader::stack()->top();
        m_geometry->Draw();
    }

    virtual void unapply() override {
        // nothing to do here
    }

    
    virtual const char* getTypeName() const override {
        return "GeometryComponent";
    }

    geometry::GeometryPtr getGeometry() {
        return m_geometry;
    }

    void setGeometry(geometry::GeometryPtr g) {
        m_geometry = g;
    }
};


}

#endif