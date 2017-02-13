#ifndef NX_SIGNATURE_H
#define NX_SIGNATURE_H

#ifdef _MSC_VER

typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;

#else
#include <stdint.h>
#endif

#include <assert.h>

namespace nx {

class Attribute {
 public:
  enum Type { NONE = 0, BYTE, UNSIGNED_BYTE, SHORT, UNSIGNED_SHORT, INT, UNSIGNED_INT, FLOAT, DOUBLE };
  unsigned char type; //0, GL_BYTE=1,GL_UNSIGNED_BYTE, GL_SHORT, GL_UNSIGNED_SHORT,GL_INT, GL_UNSIGNED_INT, GL_FLOAT, and GL_DOUBLE etc
  unsigned char number;

  Attribute(): type(0), number(0) {}

  Attribute(Type t, unsigned char n): type(t), number(n) {}
  int size() {
    static unsigned char typesize[] = { 0, 1, 1, 2, 2, 4, 4, 4, 8 };
    return number*typesize[type];
  }
  bool isNull() { return type == 0; }
};

class Element {
 public:
  Attribute attributes[8];
  void setComponent(unsigned int c, Attribute attr) {
    assert(c < 8);
    attributes[c] = attr;
  }
  int size() {
    int s = 0;
    for(unsigned int i = 0; i < 8; i++)
      s += attributes[i].size();
    return s;
  }
};

class VertexElement: public Element {
 public:
  enum Component { COORD = 0, NORM =1, COLOR = 2, TEX = 3, DATA0 = 4 };

  bool hasNormals() { return !attributes[NORM].isNull(); }
  bool hasColors() { return !attributes[COLOR].isNull(); }
  bool hasTextures() { return !attributes[TEX].isNull(); }
  bool hasData(int i) { return !attributes[DATA0 + i].isNull(); }
};

class FaceElement: public Element {
 public:
  enum Component { INDEX = 0, NORM =1, COLOR = 2, TEX = 3, DATA0 = 4 };

  bool hasIndex() { return !attributes[INDEX].isNull(); }
  bool hasNormals() { return !attributes[NORM].isNull(); }
  bool hasColors() { return !attributes[COLOR].isNull(); }
  bool hasTextures() { return !attributes[TEX].isNull(); }
  bool hasData(int i) { return !attributes[DATA0 + i].isNull(); }
};

class Signature {
 public:
  VertexElement vertex;
  FaceElement face;

  enum Flags { PTEXTURE = 0x1, MECO = 0x2, CTM1 = 0x4, CTM2 = 0x8 };
  uint32_t flags;
  void setFlag(Flags f) { flags |= f; }
  void unsetFlag(Flags f) { flags &= ~f; }

  bool hasPTextures() { return (bool)(flags | PTEXTURE); }
  bool isMecoCompressed() { return (bool)(flags | MECO); }

  Signature(): flags(0) {}
};

}//namespace

#endif // SIGNATURE_H
