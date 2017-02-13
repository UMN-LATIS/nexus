#ifndef CONTROLLER_H
#define CONTROLLER_H


#include <wrap/gcache/controller.h>

#include "token.h"
#include "ram_cache.h"
#include "gpu_cache.h"


class QGLWidget;

namespace nx {

class Nexus;

class TokenRemover {
 public:
  Nexus *nexus;
  TokenRemover(Nexus *n): nexus(n) {}
  bool operator()(vcg::Token<Priority> *t) {
    nx::Token *n = (nx::Token *)t;
    return n->nexus == nexus;
  }
};

class Controller: public vcg::Controller<Token> {
public:
    RamCache ram_cache;
    GpuCache gpu_cache;

    Controller();
    ~Controller() { finish(); }


    void setWidget(QGLWidget *widget);

    void setGpu(uint64_t size) { max_gpu = size; gpu_cache.setCapacity(size); }
    uint64_t maxGpu() const { return max_gpu; }

    void setRam(uint64_t size) { max_ram = size; ram_cache.setCapacity(size); }
    uint64_t maxRam() const { return max_ram; }

    void load(Nexus *nexus);
    void flush(Nexus *nexus);
    bool hasCacheChanged();
    void endFrame();

    void dropGpu();
protected:
    uint64_t max_tokens, max_ram, max_gpu;    //sizes for cache and (max gpu actually limits max tri per frame?)
};


}

#endif // NEXUSCONTROLLER_H
