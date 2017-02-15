#ifndef PLYLOADER_H
#define PLYLOADER_H

#include "meshloader.h"
#include "../common/virtualarray.h"


#include <wrap/ply/plylib.h>


class PlyLoader: public MeshLoader {
public:
    PlyLoader(QString file);
    ~PlyLoader();

    void setMaxMemory(quint64 max_memory);
	quint32 getTriangles(quint32 size, Triangle *buffer);
    quint32 getVertices(quint32 size, Splat *vertex);

    quint32 nVertices() { return n_vertices; }
    quint32 nTriangles() { return n_triangles; }
private:
	vcg::ply::PlyFile pf;
    qint64 vertices_element;
    qint64 faces_element;

    VirtualArray<Vertex> vertices;
    quint64 n_vertices;
    quint64 n_triangles;
    quint64 current_triangle;
    quint64 current_vertex;

    void init();
    void cacheVertices();
};

#endif // PLYLOADER_H

