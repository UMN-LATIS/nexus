#ifndef MESHLOADER_H
#define MESHLOADER_H

#include <QString>
#include "trianglesoup.h"
#include <vcg/space/point3.h>
#include <vcg/space/box3.h>

class MeshLoader {
public:
    MeshLoader(): has_colors(false), has_normals(false), has_textures(false), quantization(0) {}
    virtual ~MeshLoader() {}
    void setVertexQuantization(float q) { quantization = q; }

    virtual void setMaxMemory(quint64 max_memory) = 0;
    //returns actual number of triangles to pointers, memory must be allocated //if null, ignore.
    virtual quint32 getTriangles(quint32 size, Triangle *triangle) = 0;
    virtual quint32 getVertices(quint32 size, Splat *vertex) = 0;
    virtual bool hasColors() { return has_colors; } //call this
    virtual bool hasNormals() { return has_normals; } //call this
    virtual bool hasTextures() { return has_textures; }
	std::vector<QString> texture_filenames;
	int texOffset; //when returning triangles add texOffset to refer to the correct texture in stream.

protected:
    bool has_colors;
    bool has_normals;
    bool has_textures;
    float quantization;

    void quantize(float &value);
};

#endif // MESHLOADER_H
