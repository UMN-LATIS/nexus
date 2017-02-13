#ifndef MESH_H
#define MESH_H

#include <vcg/space/point3.h>
#include <vcg/space/color4.h>
#include "trianglesoup.h"
#include <QString>

// stuff to define the mesh
#include <vcg/complex/complex.h>
#include <wrap/io_trimesh/export.h> //DEBUG

#include <vcg/math/quadric.h>

// update
#include <vcg/complex/algorithms/clean.h>
#include <vcg/complex/algorithms/update/topology.h>
#include <vcg/complex/algorithms/update/normal.h>

// local optimization
#include <vcg/complex/algorithms/local_optimization.h>
#include <vcg/complex/algorithms/local_optimization/tri_edge_collapse_quadric.h>


#include "../common/nexus.h"
#include "../common/cone.h"

//Quadrics simplification structures.
class AVertex;
class AEdge;
class AFace;

struct AUsedTypes: public vcg::UsedTypes<vcg::Use<AVertex>::AsVertexType,vcg::Use<AEdge>::AsEdgeType,vcg::Use<AFace>::AsFaceType>{};

class AVertex  : public vcg::Vertex<
        AUsedTypes,
        vcg::vertex::VFAdj,
        vcg::vertex::Coord3f,
        vcg::vertex::Normal3f,
        vcg::vertex::TexCoord2f,
        vcg::vertex::Color4b,
        vcg::vertex::Mark,
        vcg::vertex::BitFlags> {
public:
    quint32 node;
    AVertex() { q.SetZero(); } //TODO remove this, it's only because of a stupid assert is valid on copy
    //int position;
    vcg::math::Quadric<double> &Qd() {return q;}

private:
    vcg::math::Quadric<double> q;

};

class VertexNodeCompare {
public:
    bool operator()(const AVertex &va, const AVertex &vb) const {
        return va.node < vb.node;
    }
};

class VertexCompare {
public:
    bool operator()(const AVertex &va, const AVertex &vb) const {
        const vcg::Point3f &a = va.cP();
        const vcg::Point3f &b = vb.cP();
        if(a[2] < b[2]) return true;
        if(a[2] > b[2]) return false;
        if(a[1] < b[1]) return true;
        if(a[1] > b[1]) return false;
        return a[0] < b[0];
    }
};

class AEdge : public vcg::Edge< AUsedTypes> {};

class AFace: public vcg::Face<
        AUsedTypes,
        vcg::face::VFAdj,
        vcg::face::VertexRef,
        vcg::face::BitFlags > {
public:
    quint32 node;
    bool operator<(const AFace &t) const {
        return node < t.node;
    }
};

class Mesh: public vcg::tri::TriMesh<std::vector<AVertex>, std::vector<AFace> > {
public:
    enum Simplification { QUADRICS, EDGE, CLUSTER, RANDOM };
    void load(Soup &soup);
    void load(Cloud &soup);
    void lock(std::vector<bool> &locked);
    void save(Soup &soup, quint32 node);
    void getTriangles(Triangle *triangles, quint32 node);
    void getVertices(Splat *vertices, quint32 node);

    float simplify(quint16 target_faces, Simplification method);
    std::vector<AVertex> simplifyCloud(quint16 target_vertices); //return removed vertices
    float averageDistance();

    void savePly(QString filename);
    nx::Node getNode();
    quint32 serializedSize(nx::Signature &sig);
    //appends nodes found in the mesh
    void serialize(uchar *buffer, nx::Signature &sig, std::vector<nx::Patch> &patches);

    vcg::Sphere3f boundingSphere();
    nx::Cone3s normalsCone();
protected:
    float randomSimplify(quint16 target_faces);
    float quadricSimplify(quint16 target_faces);

    float edgeLengthError();
};

typedef vcg::tri::BasicVertexPair<AVertex> VertexPair;

class TriEdgeCollapse:
        public vcg::tri::TriEdgeCollapseQuadric< Mesh, VertexPair, TriEdgeCollapse, vcg::tri::QInfoStandard<AVertex>  > {
public:
    typedef  vcg::tri::TriEdgeCollapseQuadric< Mesh,  VertexPair, TriEdgeCollapse, vcg::tri::QInfoStandard<AVertex>  > TECQ;
    typedef  Mesh::VertexType::EdgeType EdgeType;
    inline TriEdgeCollapse(  const VertexPair &p, int i, vcg::BaseParameterClass *pp) :TECQ(p,i,pp){}
};

#endif // MESH_H
