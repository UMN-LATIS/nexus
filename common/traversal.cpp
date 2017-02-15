#include <algorithm>
#include "globalgl.h"
#include "traversal.h"

using namespace std;
using namespace nx;

Traversal::Traversal(): nexus(NULL), prefetch(200) {}

void Traversal::traverse(NexusData *nx) {
    nexus = nx;
    uint32_t n_nodes = nexus->header.n_nodes;
    sink =  n_nodes - 1;

    heap.clear();
    visited.clear();
    visited.resize(n_nodes, false);
    selected.clear();
    selected.resize(n_nodes, false);
    blocked.clear();
    blocked.resize(n_nodes, false);

	//This is ok only if the top level has only 1 node.
	//find first non parent node.
	Node &root = nexus->nodes[0];
	uint32_t second_level_start = nexus->patches[root.first_patch].node;
	for(uint32_t i = 0; i < second_level_start; i++) {
		add(i);
	}

    int node_visited = 0;
    non_blocked = 1;
    while(heap.size() && non_blocked > -prefetch) {
        pop_heap(heap.begin(), heap.end());
        HeapNode h = heap.back();
        heap.pop_back();

        if(blocked[h.node])
            blockChildren(h.node);
        else {
            non_blocked--;
            Action action = expand(h);

            if(action == STOP) break;
            else if(action == EXPAND)
                addChildren(h.node);
            else //BLOCK
                blockChildren(h.node);
        }
        node_visited++;
    }
}

float Traversal::nodeError(uint32_t n, bool &visible) {
    visible = true;
    Node &node = nexus->nodes[n];
    return node.error;
}

bool Traversal::add(uint32_t n) {
    if(n == sink) return false;

    //do not add duplicates of nodes
    if(visited[n]) return false;

    bool visible = true;
    float error = nodeError(n, visible);
    //if(!visible) return false;
    HeapNode h(n, error, visible);

    heap.push_back(h);
    push_heap(heap.begin(), heap.end());
    visited[n] = true;
    return true;
}

void Traversal::addChildren(uint32_t n) {
    selected[n] = true;
    Node &node = nexus->nodes[n];
    for(uint32_t i = node.first_patch; i < node.last_patch(); i++)
        if(add(nexus->patches[i].node))
            non_blocked++;
}

void Traversal::blockChildren(uint32_t n) {
    Node &node = nexus->nodes[n];
    for(uint32_t i = node.first_patch; i < node.last_patch(); i++) {
        uint32_t child = nexus->patches[i].node;
        blocked[child] = true;
        if(!add(child))
            non_blocked--;
    }
}

bool Traversal::skipNode(quint32 n) {
    if(!selected[n]) return true;

    Node &node = nexus->nodes[n];

    for(uint p = node.first_patch; p < node.last_patch(); p++)
        if(!selected[nexus->patches[p].node])
            return false;
    return true;
}


/* METRIC TRAVERSAL */
/*
MetricTraversal::MetricTraversal():  target_error(4.0f) {}

MetricTraversal::~MetricTraversal() {}


float MetricTraversal::nodeError(uint32_t n, bool &visible) {
    Node &node = nexus->nodes[n];
    return metric.GetError(node.sphere, node.error, visible);
}

Traversal::Action MetricTraversal::expand(HeapNode h) {
    if(h.error < target_error) return BLOCK;
    return EXPAND;
}
*/
