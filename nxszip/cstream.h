#ifndef CSTREAM_H
#define CSTREAM_H

#include <string.h>
#include <iostream>

#include "bitstream.h"

using namespace std;

typedef unsigned char uchar;

class CStream {
public:
    uchar *buffer;
    uchar *pos; //for reading.
    int allocated;

    CStream(): buffer(NULL), pos(NULL), allocated(0) {}
    CStream(int reserved) {
        reserve(reserved);
    }
    CStream(int _size, unsigned char *_buffer) {
        init(_size, _buffer);
    }
    ~CStream() {
        if(allocated)
            delete []buffer;
    }
    void reserve(int reserved) {
        allocated = reserved;
        pos = buffer = new uchar[allocated];
    }
    void init(int /*_size*/, unsigned char *_buffer) {
        buffer = _buffer;
        pos = buffer;
        allocated = 0;
    }
    void rewind() { pos = buffer; }
    int size() { return pos - buffer; }
    void grow(int s) {
        int size = pos - buffer;
        if(size + s > allocated) { //needs more spac
            int new_size = allocated*2;
            while(new_size < size + s)
                new_size *= 2;
            uchar *b = new uchar[new_size];
            memcpy(b, buffer, allocated);
            delete []buffer;
            buffer = b;
            pos = buffer + size;
            allocated = new_size;
        }
    }

    void push(void *b, int s) {
        grow(s);
        memcpy(pos, b, s);
        pos += s;
    }
    template<class T> void write(T c) { grow(sizeof(T)); *(T *)pos = c; pos += sizeof(T); }
    //usually we know how long c is for some other reason
    template<class T> void writeArray(int s, T*c) {
        int bytes = s*sizeof(T);
        push(c, bytes);
    }
    void write(BitStream &stream) {
        //need to pad to 32bits
        write<int>((int)stream.size);
        //padding is needed for javascript reading.
        int pad = (pos - buffer) & 0x3;
        if(pad != 0)
            pad = 4 - pad;
        grow(pad);
        pos += pad;
        push(stream.buffer, stream.size*sizeof(uint64_t));
    }


    template<class T> T read() {
        T c;
        c = *(T *)pos;
        pos += sizeof(T);
        return c;
    }
    template<class T> T *readArray(int s) {
        int bytes = s*sizeof(T);
        T *buffer = (T *)pos;
        pos += bytes;
        return buffer;
    }
    void read(BitStream &stream) {
        int s = read<int>();
        int pad = (pos - buffer) & 0x3;
        if(pad != 0)
            pos += 4 - pad;
        stream.init(s, (uint64_t *)pos);
        pos += s*sizeof(uint64_t);
    }


};

#endif // CSTREAM_H
