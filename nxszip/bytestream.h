#ifndef BYTESTREAM_H
#define BYTESTREAM_H

#include <assert.h>

#include <vector>
#include <iostream>
using namespace std;

//input and output byte: you must know when to stop reading.

class OutputStream: public std::vector<unsigned char> {
 public:
  OutputStream(): pos(0) {}
  void writeByte(unsigned char byte) { push_back(byte); }
  template<class T> void write(T &t) {
      int s = size();
      resize(s + sizeof(T));
      *(T *)(&*begin() + s) = t;
  }
  unsigned char readByte() { return operator[](pos++); }
  void restart() { pos = 0; }

  int pos;
 private:

};

//read and write into memory directly, ensure there is enough space

class InputStream {
 public:
  unsigned char *mem; //current memory location
  int count;
  int max_length;

  InputStream(unsigned char *m, int maxlen): mem(m), count(0), max_length(maxlen) {}
  unsigned char readByte() { assert(count < max_length); return mem[count++]; }
  template<class T> T read() {
      T &t = *(T *)(mem + count);
      count += sizeof(T);
      return t;
  }
  void restart() { count = 0; }
};

#endif // BYTESTREAM_H
