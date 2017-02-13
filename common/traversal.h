#ifndef NX_TRAVERSAL_H
#define NX_TRAVERSAL_H

#include "nexusdata.h"
#include "metric.h"

/* a generic DAG traversal algorithm:
   add(0) //needs virtual compute error
   while(heap) {
     if(process()) //virtual
       addchildren()
     else
       break;
   }
 */

namespace nx {

class Traversal {
public:
    enum Action { STOP,           //stop traversal
                  EXPAND,         //expand node
                  BLOCK };        //do not expand node and block all his children

    struct HeapNode {
        uint32_t node;
        float error;
        bool visible;

        HeapNode(uint32_t _node, float _error, bool _visible):
            node(_node), error(_error), visible(_visible) {}
        bool operator<(const HeapNode &n) const {
            if(error == n.error) return node > n.node;
            return error < n.error;
        }
        bool operator>(const HeapNode &n) const {
            if(error == n.error) return node < n.node;
            return error > n.error;
        }
    };

    NexusData *nexus;
    std::vector<bool> selected;

    Traversal();
    virtual ~Traversal() {}
    void traverse(NexusData *nx);

    virtual float nodeError(uint32_t node, bool &visible);
    virtual Action expand(HeapNode /*h*/) = 0;

protected:
    uint32_t sink;
    std::vector<HeapNode> heap;
    std::vector<bool> visited;
    std::vector<bool> blocked; //nodes blocked by some parent which could not be expanded
    int32_t non_blocked;
    int32_t prefetch;

    bool skipNode(quint32 node);

private:
    bool add(uint32_t node);                //returns true if added
    void addChildren(uint32_t node);
    void blockChildren(uint32_t node);

};
/*
class MetricTraversal: public Traversal {
public:
    FrustumMetric metric;
    float target_error;

    MetricTraversal();
    virtual ~MetricTraversal();
    virtual float nodeError(uint32_t node, bool &visible);
    virtual Action expand(HeapNode h);

    void setTargetError(float error) { target_error = error; }
    void getView(const float *proj = NULL, const float *modelview = NULL, const int *viewport = NULL) {
        metric.GetView(proj, modelview, viewport);
    }
};*/

}//namespace
#endif // TRAVERSAL_H
