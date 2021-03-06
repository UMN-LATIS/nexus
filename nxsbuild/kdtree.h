#ifndef KDTREE_H
#define KDTREE_H

#include <vcg/space/box3.h>
#include "partition.h"

#include <vector>

class Stream;
class StreamSoup;
class StreamCloud;
class Mesh;
class TMesh;

class KDCell {
public:
    vcg::Box3f box;
    int split;           //splitting axis: 0, 1,
    float middle;
    int children[2];     //first and second child
    int block;             //bin where triangle are stored.

    KDCell(): split(-1), middle(0), block(-1) {
        children[0] = children[1] = -1;
    }
    bool isLeaf() { return children[0] < 0; }
};

class KDTree {
public:    
    vcg::Point3f axes[3];
    float ratio;
    std::vector<KDCell> cells;
    std::vector<vcg::Box3f> block_boxes;      //bounding box associated to the blocks (leaf nodes)
    std::vector<QString> textures;

    KDTree(float adapt = 0.333);
    virtual ~KDTree() {}

    void load(Stream *stream);
    virtual void setMaxMemory(quint64 m) = 0;
    virtual void clear() = 0;

    void setAxes(vcg::Point3f &x, vcg::Point3f &y, vcg::Point3f &z);
    void setAxesDiagonal();
    void setAxesOrthogonal();

    int nLeaves() { return block_boxes.size(); }
    void lock(Mesh &mesh, int block);
    void lock(TMesh &mesh, int block);
    bool isInNode(quint32 node, vcg::Point3f &p);
    static bool isIn(vcg::Point3f *axes, vcg::Box3f &box, vcg::Point3f &p); //check if point is in the box (using axes) semiopen box.

protected:
    float adaptive; //from 0 (not adaptive, to 1) fully adaptive.

    virtual quint64 addBlock() = 0;
    virtual void loadElements(Stream *stream) = 0;
    virtual void findRealMiddle(KDCell &node)  = 0;
    virtual void splitNode(KDCell &node, KDCell &child0, KDCell &child1) = 0;

    void split(int n);
    float findMiddle(KDCell &node);
    bool isIn(vcg::Box3f &box, vcg::Point3f &p); //check if point is in the box (using axes) semiopen box.
    vcg::Box3f computeBox(vcg::Box3f &b);
};

class KDTreeSoup: public VirtualTriangleSoup, public KDTree {
public:
    KDTreeSoup(QString prefix, float adapt = 0.333): VirtualTriangleSoup(prefix), KDTree(adapt) {}    
    void setMaxMemory(quint64 m) { return VirtualTriangleSoup::setMaxMemory(m); }
    void clear();
    void pushTriangle(Triangle &t);


protected:
    quint64 addBlock() { return VirtualTriangleSoup::addBlock(); }
    void loadElements(Stream *stream);
    void findRealMiddle(KDCell &node);
    void splitNode(KDCell &node, KDCell &child0, KDCell &child1);
    static int assign(Triangle &t, quint32 &mask, vcg::Point3f axis, float middle);
};

class KDTreeCloud: public VirtualVertexCloud, public KDTree {
public:
    KDTreeCloud(QString prefix, float adapt = 0.333): VirtualVertexCloud(prefix), KDTree(adapt) {}
    void setMaxMemory(quint64 m) { return VirtualVertexCloud::setMaxMemory(m); }
    void clear();
    void pushVertex(Splat &v);

protected:
    quint64 addBlock() { return VirtualVertexCloud::addBlock(); }
    void load(StreamSoup &stream);
    void loadElements(Stream *stream);
    void findRealMiddle(KDCell &node);
    void splitNode(KDCell &node, KDCell &child0, KDCell &child1);
};


#endif // KDTREE_H
