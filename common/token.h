#ifndef TOKEN_H
#define TOKEN_H

#include <wrap/gcache/token.h>

namespace nx {

class Nexus;

class Priority {
public:
    float error;
    uint32_t frame;
    Priority(float e = 0, uint32_t f = 0): error(e), frame(f) {}
    bool operator<(const Priority &p) const {
        if(frame == p.frame) return error < p.error;
        return frame < p.frame;
    }
    bool operator>(const Priority &p) const {
        if(frame == p.frame) return error > p.error;
        return frame > p.frame;
    }
};

class Token: public vcg::Token<Priority> {
public:
    Nexus *nexus;
    uint32_t node;

    Token() {}
    Token(Nexus *nx, uint32_t n):  nexus(nx), node(n) {}
};

} //namespace
#endif // TOKEN_H
