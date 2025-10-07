#ifndef SCENE_VISITOR_H
#define SCENE_VISITOR_H
#pragma once

#include "scene.h"

namespace scene {

class SceneGraphVisitor {
    inline static SceneGraphPtr sg = graph();
    
};

}

#endif