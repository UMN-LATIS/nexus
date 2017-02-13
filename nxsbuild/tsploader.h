#ifndef TSPLOADER_H
#define TSPLOADER_H

#include "meshloader.h"

class TspLoader: public MeshLoader {
public:
    TspLoader(QString file);
    void setMaxMemory(quint64 /*m*/) {  }
    quint32 getTriangles(quint32 size, Triangle *buffer);
	quint32 getVertices(quint32 size, Splat *vertex) { assert(0); return 0; }

private:
    QFile file;
    quint64 current_triangle;

};

#endif
