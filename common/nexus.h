#ifndef NEXUS_H
#define NEXUS_H

#include "nexusdata.h"

typedef void CURL;

namespace nx {

class Controller;
class Token;

class Nexus: public NexusData {
public:
    Token *tokens;

    Controller *controller;

    Nexus(Controller *controller = NULL);
    ~Nexus();
    bool open(const char *uri);
    void flush();
    bool isReady();
    bool isStreaming() { return http_stream; }

    uint64_t loadGpu(uint32_t node);
    uint64_t dropGpu(uint32_t node);

    void initIndex();
    void loadIndex();
    void loadIndex(char *buffer);

    bool loaded;
    bool http_stream;

	//tempoarary texture loading from file
	QString filename;
};

} //namespace
#endif // NEXUS_H
