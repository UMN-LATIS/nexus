#ifndef NEXUSBUILDER_H
#define NEXUSBUILDER_H

#include "../common/signature.h"
#include "../common/dag.h"

#include <QString>
#include <QFile>
#include <QMutex>
#include <QImage>
#include "../common/virtualarray.h"

#include <vcg/space/box3.h>

#include <vector>

class KDTree;
class KDTreeSoup;
class KDTreeCloud;
class Stream;
class StreamSoup;
class StreamCloud;

namespace nx {
    class Nexus;
}

//Structures used for normalization
struct NodeBox {
    vcg::Point3f axes[3];
    vcg::Box3f box;

    NodeBox() {}
    NodeBox(KDTree *tree, uint32_t block);
    bool isIn(vcg::Point3f &p);
    std::vector<bool> markBorders(nx::Node &node, vcg::Point3f *p, uint16_t *f);
};

class NVertex {
public:
    NVertex(uint32_t b, uint32_t i, vcg::Point3f p, vcg::Point3s *n):
        node(b), index(i), point(p), normal(n) {}
    uint32_t node;
    uint32_t index;
    vcg::Point3f point;
    vcg::Point3s *normal;

    bool operator<(const NVertex &v) const {
        if(point == v.point)
            return node > v.node;  //ascending orde: the first will be the highest node order (the leafest node)
        return point < v.point;
    }
};


class NexusBuilder {
public:
    enum Components { FACES = 1, NORMALS = 2, COLORS = 4, TEXTURES = 8 };
    NexusBuilder(quint32 components);
    NexusBuilder(nx::Signature &sig);

    bool hasNormals() { return header.signature.vertex.hasNormals(); }
    bool hasColors() { return header.signature.vertex.hasColors(); }
    bool hasTextures() { return header.signature.vertex.hasTextures(); }

    void create(KDTree *input, Stream *output, uint top_node_size);
    void createLevel(KDTree *input, Stream *output, int level);

    void setMaxMemory(quint64 m) {
        max_memory = m;
        chunks.setMaxMemory(m);
    }
    void setScaling(float s) { scaling = s; }

    void saturate();

    void reverseDag();
    void save(QString filename);

    //void addNode(mesh)
    //    void process(Nexus &input, std::vector<bool> &selection);
    QMutex m_input;
    QMutex m_output;
    QMutex m_builder;
    QMutex m_chunks;

    QFile file;

    VirtualChunks chunks;
    std::vector<NodeBox> boxes; //a box for each node

    nx::Header header;
    std::vector<nx::Node> nodes;
    std::vector<nx::Patch> patches;
    std::vector<nx::Texture> textures;
    std::vector<QString> images;

    QTemporaryFile tmp; //used during construction
    quint64 max_memory;

    float scaling;



    void invertNodes(); //
    void saturateNode(quint32 n);
    void optimizeNode(quint32 node, uchar *chunk);
    void uniformNormals();
    void appendBorderVertices(uint32_t origin, uint32_t destination, std::vector<NVertex> &vertices);
    quint32 pad(quint32 s);

    void testSaturation();
};

#endif // NEXUSBUILDER_H
