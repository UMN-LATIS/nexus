#ifndef OBJLOADER_H
#define OBJLOADER_H

#include "meshloader.h"
#include "../common/virtualarray.h"

#include <QFile>

class ObjLoader: public MeshLoader {
public:
    ObjLoader(QString file);
    ~ObjLoader();

    void setMaxMemory(quint64 max_memory);
    quint32 getTriangles(quint32 size, Triangle *buffer);
    quint32 getVertices(quint32 size, Splat *vertex);

private:
    QFile file;
    VirtualArray<Vertex> vertices;
    quint64 n_vertices;
    quint64 n_triangles;    
    quint64 current_triangle;
    quint64 current_vertex;

    void cacheVertices();
};
#endif // OBJLOADER_H
