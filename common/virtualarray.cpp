#include <assert.h>

#include "virtualarray.h"

#include <iostream>
using namespace std;


VirtualMemory::VirtualMemory(QString prefix):
    QTemporaryFile(prefix),
    used_memory(0),
    max_memory(1<<28) {

    setAutoRemove(true);
    if(!open())
        throw QString("Unable to open temporary file");

}

VirtualMemory::~VirtualMemory() {
    flush();
    for(uint i = 0; i < cache.size(); i++)
        assert(cache[i] == NULL);
}

void VirtualMemory::setMaxMemory(quint64 n) {
    max_memory = n;
}

uchar *VirtualMemory::getBlock(quint64 index, bool prevent_unload) {

    assert(index < cache.size());
    if(cache[index] == NULL) { //not mapped.
        if(!prevent_unload)
            makeRoom();
        mapBlock(index);
        if(!cache[index])
            throw QString("VirtualMemory error mapping block: " + this->errorString());

        mapped.push_front(index);
    }
    return cache[index];
}

void VirtualMemory::dropBlock(quint64 index) {
    unmapBlock(index);
}

void VirtualMemory::resize(quint64 n, quint64 n_blocks) {
#ifndef WIN32
    if(n < (quint64)size())
        flush();
#else
    flush();
#endif
    cache.resize(n_blocks, NULL);
    QTemporaryFile::resize(n);
#ifdef WIN32
	/*for (qint64 i = 0; i < cache.size(); i++)
		mapBlock(i);*/
#endif
}

quint64 VirtualMemory::addBlock(quint64 length) {
#ifdef WIN32
    flush();
#endif
    cache.push_back(NULL);
    QFile::resize(size() + length);
#ifdef WIN32
	/*for (qint64 i = 0; i < cache.size(); i++)
		mapBlock(i);*/
#endif
    return cache.size()-1;
}

void VirtualMemory::flush() {
    for(quint32 i = 0; i < cache.size(); i++) {
        if(cache[i])
            unmapBlock(i);
    }
    mapped.clear();
    used_memory = 0;
}

void VirtualMemory::makeRoom() {
    while(used_memory > max_memory) {
        assert(mapped.size());
        quint64 block = mapped.back();
        if(cache[block])
            unmapBlock(block);
        mapped.pop_back();
    }
}

uchar *VirtualMemory::mapBlock(quint64 block) {
    quint64 offset = blockOffset(block);
    quint64 length = blockSize(block);
    assert(offset + length <= (quint64)QFile::size());
    cache[block] = map(offset, length);
    used_memory += length;
    return cache[block];
}

void VirtualMemory::unmapBlock(quint64 block) {
    assert(block < cache.size());
    assert(cache[block]);
    unmap(cache[block]);
    cache[block] = NULL;
    used_memory -= blockSize(block);
}


