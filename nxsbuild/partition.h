#ifndef PARTITION_H
#define PARTITION_H

#include <vcg/space/point3.h>
#include "trianglesoup.h"
#include <vector>

class StreamSoup;

class Partition: public VirtualTriangleSoup {
 public:
  Partition(QString prefix): VirtualTriangleSoup(prefix) {}
  virtual ~Partition() {}
  virtual void load(StreamSoup &stream) = 0;
  virtual bool isInNode(quint32 node, vcg::Point3f &p) = 0;
};

#endif // PARTITION_H
