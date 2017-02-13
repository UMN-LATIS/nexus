#include "scene.h"

Scene::~Scene() {
    for(uint i = 0; i < nodes.size(); i++)
        delete nodes[i].nexus;
}

bool Scene::load(QStringList inputs, int instances) {
    for(int i = 0; i < inputs.size(); i++) {

        nx::Nexus *nexus = new nx::Nexus(&controller);

        if(!nexus->open(inputs[i].toLatin1())) {
            cerr << "Could not load file: " << qPrintable(inputs[i]) << endl;
            return false;
        }
        models.push_back(nexus);
    }

    //build scene
    int n_nexus = models.size();
    int n_nodes = instances;
    if(n_nodes < (int)models.size()) n_nodes = models.size();
    //int side = (int)sqrt(n_nodes);


    for(int i = 0; i < n_nodes; i++) {
        nx::Nexus *nexus = models[i%n_nexus];
        Scene::Node node(nexus);
        nodes.push_back(node);
    }
    return true;
}

void Scene::update() {
    int n_nodes = nodes.size();

    if(autopositioning) {
        int side = (int)sqrt((double)n_nodes);
        for(unsigned int i = 0; i < nodes.size(); i++) {
            Scene::Node &node = nodes[i];
            if(node.inited) continue;

            nx::Nexus *nexus = node.nexus;
            if(nexus->isReady()) {
                const int y  = i/side;
                const int x  = i - (y*side);

                //scale everything to 1
                vcg::Sphere3f sphere = nexus->boundingSphere();
                float r = sphere.Radius()*2;
                vcg::Point3f t = sphere.Center();

                node.inited = true;
                node.transform.SetTranslate(x, 0, y);
                node.transform *= vcg::Matrix44f().SetScale(1/r, 1/r, 1/r);
                node.transform *= vcg::Matrix44f().SetTranslate(-t);
            }
        }
    } else {
        vcg::Sphere3f sphere;
        for(unsigned int i = 0; i < nodes.size(); i++) {
            Scene::Node &node = nodes[i];

            nx::Nexus *nexus = node.nexus;
            if(nexus->isReady())
                sphere.Add(nexus->boundingSphere());
        }

        float r = sphere.Radius()*2;
        vcg::Point3f t = sphere.Center();
        for(unsigned int i = 0; i < nodes.size(); i++) {
            Scene::Node &node = nodes[i];
            node.transform.SetTranslate(0, 0, 0);
            node.transform *= vcg::Matrix44f().SetScale(1/r, 1/r, 1/r);
            node.transform *= vcg::Matrix44f().SetTranslate(-t);
        }
    }
}
