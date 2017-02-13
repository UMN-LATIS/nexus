#include "controller.h"

using namespace nx;

//TODO how to auto-guess ram and gpu?
Controller::Controller(): max_tokens(300), max_ram(512*1000*1000), max_gpu(256*1000*1000) {
    ram_cache.setCapacity(max_ram);
    gpu_cache.setCapacity(max_gpu);

    addCache(&ram_cache);
    addCache(&gpu_cache);
    //setMaxTokens(max_tokens);
}

void Controller::setWidget(QGLWidget *widget) {
    //TODO check destruction of widget
    gpu_cache.shared = widget; //new QGLWidget(NULL, widget);
}

void Controller::load(Nexus *nexus) {
    ram_cache.add(nexus);
    ram_cache.input->check_queue.open();
}

void Controller::flush(Nexus *nexus) {
    TokenRemover remover(nexus);
    removeTokens(remover);
}

bool Controller::hasCacheChanged() {
    return newData();
}

void Controller::endFrame() {
#ifndef SHARED_CONTEXT
    gpu_cache.dropGpu();
#endif
    updatePriorities();
}
