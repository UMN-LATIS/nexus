#ifndef GPU_CACHE_H
#define GPU_CACHE_H

#include "globalgl.h"
#include "nexus.h"
#include "token.h"
#include <wrap/gcache/cache.h>

#include <QThread>
//#include <QWindow>
#include <QGLContext>

#include <iostream>
using namespace std;

namespace nx {

//#define SHARED_CONTEXT

class GpuCache: public vcg::Cache<nx::Token> {
public:
#ifndef NO_QT
    QGLWidget *widget;
    QGLWidget *shared;
#endif

#ifndef SHARED_CONTEXT
    std::vector<unsigned int> to_drop;
    mt::mutex droplock;
#endif

    GpuCache(): widget(NULL), shared(NULL) {}

#ifndef SHARED_CONTEXT
    void dropGpu() {
        mt::mutexlocker locker(&droplock);
        for(uint i = 0; i < to_drop.size(); i++)
            glDeleteBuffers(1, (GLuint *)&(to_drop[i]));
        to_drop.clear();
    }
#endif

#ifdef SHARED_CONTEXT
     void begin() {
        widget = new QGLWidget(NULL, shared);
        widget->show();
        widget->makeCurrent();
    }
    void end() {
        widget->doneCurrent();
//        /context.doneCurrent();
    }
#endif

    int get(nx::Token *in) {
#ifdef SHARED_CONTEXT
        in->nexus->loadGpu(in->node);
        glFinish();
        mt::sleep_ms(1);
#endif
        //do nothing, transfer will be done in renderer
        return in->nexus->size(in->node);
    }
    int drop(nx::Token *in) {


#ifndef SHARED_CONTEXT
        //mark GPU to be dropped, it will be done in the renderer
        NodeData &data= in->nexus->nodedata[in->node];
        if(data.vbo == 0)  //can happen if a patch is scheduled for GPU but never actually rendererd
            return in->nexus->size(in->node);
        {
            mt::mutexlocker locker(&droplock);
            if(data.vbo) {
                to_drop.push_back(data.vbo);
                data.vbo = 0;
            }
            if(data.fbo) {
                to_drop.push_back(data.fbo);
                data.fbo = 0;
            }
        }
#else
        in->nexus->dropGpu(in->node);
#endif
        return in->nexus->size(in->node);
    }

    int size(nx::Token *in) {
        return in->nexus->size(in->node);
    }
    int size() { return Cache<nx::Token>::size(); }
};

} //namespace

#endif // GPU_CACHE_H
