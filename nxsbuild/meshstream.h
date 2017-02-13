#ifndef MESHSTREAM_H
#define MESHSTREAM_H

#include <QStringList>
#include <vcg/space/box3.h>

#include "trianglesoup.h"
#include "meshloader.h"

/* triangles are organized in levels, each one with half the triangle of the lower one
   we have a buffer for each level, when filled it gets written to disk.
   I need a vector keeping track of how much full each level is
   and a  vector of vectors holding which blocks belongs to which level
   */

class Stream {
    public:
    vcg::Box3f box;
    bool has_colors;
    bool has_normals;
    bool has_textures;
    std::vector<QString> textures;

    Stream();
    virtual ~Stream() {}
    void setVertexQuantization(double q);
    void load(QStringList paths);

    //return a block of triangles. The buffer is valid until next call to getTriangles. Return null when finished
    void clear();
    virtual quint64 size() = 0;
    virtual void setMaxMemory(quint64 m) = 0;
    virtual bool hasColors() { return has_colors; }
    virtual bool hasNormals() { return has_normals; }
    virtual bool hasTextures() { return has_textures; }

protected:
    std::vector<std::vector<quint64> > levels; //for each level the list of blocks
    std::vector<quint64> order;          //order of the block for streaming

    double vertex_quantization; //a power of 2.
    quint64 current_triangle;   //used both for loading and streaming
    quint64 current_block;

    virtual void flush() = 0;
    virtual void loadMesh(MeshLoader *loader) = 0;
    virtual void clearVirtual() = 0; //clear the virtualtrianglesoup or virtualtrianglebin
    virtual quint64 addBlock(quint64 level) = 0; //return index of block added

    quint64 getLevel(qint64 index);
    void computeOrder();
};


class StreamSoup: public Stream, public VirtualTriangleSoup {
public:
    StreamSoup(QString prefix);

    void pushTriangle(Triangle &triangle);
    //return a block of triangles. The buffer is valid until next call to getTriangles. Return null when finished
    Soup streamTriangles();
    quint64 size() { return VirtualTriangleSoup::size(); }
    void setMaxMemory(quint64 m) { return VirtualTriangleSoup::setMaxMemory(m); }

protected:    
    void flush() { VirtualTriangleSoup::flush(); }
    void loadMesh(MeshLoader *loader);
    void clearVirtual();
    quint64 addBlock(quint64 level); //return index of block added

};


class StreamCloud: public Stream, public VirtualVertexCloud {
public:
    StreamCloud(QString prefix);

    void pushVertex(Splat &ertex);
    //return a block of triangles. The buffer is valid until next call to getTriangles. Return null when finished
    Cloud streamVertices();
    quint64 size() { return VirtualVertexCloud::size(); }
    void setMaxMemory(quint64 m) { return VirtualVertexCloud::setMaxMemory(m); }

protected:
    void flush() { VirtualVertexCloud::flush(); }
    void loadMesh(MeshLoader *loader);
    void clearVirtual();
    quint64 addBlock(quint64 level); //return index of block added

};

#endif // MESHSTREAM_H
