#ifndef NEXUSDATA_H
#define NEXUSDATA_H

#include <stdio.h>

#include <string>

#include "signature.h"
#include "dag.h"

#include <vcg/space/color4.h>
#include <QFile>

namespace nx {

class NodeData {
public:
    NodeData(): memory(NULL), vbo(0), fbo(0) {}
    char *memory;
    uint32_t vbo;
    uint32_t fbo;
    vcg::Point3f *coords() { return (vcg::Point3f *)memory; }
    vcg::Point2f *texCoords(Signature &/*sig*/, uint32_t nvert) {
        char *m = memory;
        m += nvert * sizeof(vcg::Point3f);
        return (vcg::Point2f *)m;
    }
    vcg::Point3s *normals(Signature &sig, uint32_t nvert) {
		char *m = memory + nvert * sizeof(vcg::Point3f);
        if(sig.vertex.hasTextures())
            m += nvert * sizeof(vcg::Point2f);
        return (vcg::Point3s *)m;
    }
    vcg::Color4b *colors(Signature &sig, uint32_t nvert) {
        char *m = memory;
        m += nvert * sizeof(vcg::Point3f);
        if(sig.vertex.hasTextures())
            m += nvert * sizeof(vcg::Point2f);
        if(sig.vertex.hasNormals())
            m += nvert * sizeof(vcg::Point3s);
        return (vcg::Color4b *)m;
    }

    uint16_t *faces(Signature &sig, uint32_t nvert) {
        return faces(sig, nvert, memory);
    }

    static uint16_t *faces(Signature &sig, uint32_t nvert, char *mem);
};

class TextureData {
public:
    TextureData(): memory(NULL), tex(0), count_ram(0), count_gpu(0) {}
    char *memory;             //some form of raw data (probably compressed jpeg)
    uint32_t tex;             //opengl identifier
    uint32_t count_ram;           //number of nodes using the texture
    uint32_t count_gpu;           //number of nodes using the texture
};


class NexusData {
public:
    Header header;
    Node *nodes;
    Patch *patches;
    Texture *textures;

    NodeData *nodedata;
    TextureData *texturedata;

    std::string url;


    NexusData();
    virtual ~NexusData();
    bool open(const char *uri);
    void close();
    virtual void flush();

    vcg::Sphere3f &boundingSphere() { return header.sphere; }
    bool intersects(vcg::Ray3f &r, float &distance);

    uint32_t size(uint32_t node);
    uint64_t loadRam(uint32_t node);
    uint64_t dropRam(uint32_t node, bool write = false);

    void loadHeader();
    void loadHeader(char *buffer);
    uint64_t indexSize();
    virtual void initIndex();
    virtual void loadIndex();
    virtual void loadIndex(char *buffer);

    //FILE *file;

    QFile file;
};

} //namespace

#endif // NEXUSDATA_H
