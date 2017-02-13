#ifndef SCENE_H
#define SCENE_H

#include "../common/controller.h"
#include <QStringList>
#include <vcg/math/matrix44.h>

namespace nx {
    class Nexus;
    class Controller;
}

class Scene {
public:
    class Node {
    public:
        nx::Nexus *nexus;
        vcg::Matrix44f transform;
        bool inited;

        Node(nx::Nexus *in): nexus(in), inited(false) {
            transform.SetIdentity();
        }
    };
    std::vector<nx::Nexus *> models;
    std::vector<Node> nodes;
    nx::Controller controller;
    bool autopositioning;

    Scene(): autopositioning(false) {}
    ~Scene();
    bool load(QStringList input, int instances);
    void update();
};

#endif // SCENE_H
