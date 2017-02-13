#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <vector>
#include <stdint.h>

class BitStream {
public:
    BitStream(): size(0), buffer(0), allocated(0), pos(0), buff(0), bits(0) {}
    BitStream(int reserved); // in uint64_t units
    BitStream(int size, uint64_t *buffer); //for reading


    ~BitStream();
    void init(int size, uint64_t *buffer); //in uint64_t units for reading
    void reserve(int size); //for writing

    void write(uint64_t bits, int n);
    void read(int n, uint64_t &bits);

    void flush();
    int size; //in uint64
    uint64_t *buffer;
private:
    void push_back(uint64_t w);
    int allocated;

    uint64_t *pos;
    uint64_t buff;
    int bits;
};

class Obstream: public std::vector<uint64_t> {
 public:
  Obstream();

  void write(uint64_t bits, int n); //write last n bits
  void flush();
 private:
  uint64_t outbuff;
  int bitsToGo;
};


class Ibstream {
 public:
  Ibstream(int size, uint64_t *buffer);

  void read(int n, uint64_t &bits);       // # of bits to read
  void rewind();                   // rewind to beginning of input
 private:
  int size;
  uint64_t *buffer;                    // stream for reading bits
  uint64_t *pos; //keep track of current position

  uint64_t inbuff;                      // buffer bits for input
  int inbbits;                     // used for buffering
};

#endif // BITSTREAM_H
