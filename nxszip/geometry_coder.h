#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <vector>

#include "math_class.h"
#include "bytestream.h"
#include "range.h"
#include "model.h"
#include "zpoint.h"
#include "../common/nexusdata.h"
//#include "connectivity_coder.h"


#include <iostream>

/* binari octree to encode a pack of vertices:
   1) gli passi il bounding box e lui calcola la potenza di due che lo contiene
   2) gli specifichi la quantizzazione (relativa o assoluta)
   3) gli passi i vertici in float e lui:
      a) li quantizza
      b) mescola i bit di x, y, z
      c) li ordina (e ti ritorna il riordinamento!)
      d) encoda l'albero in una sequenza di bit (con informazioni di contorno
         come il bounding box appunto)
      il contesto adattivo e' per livello che si limano un paio di bit (e costa zero)
   decompressione:
   gli passi uno stream di bit e riproduce un vettore di vertici.

   //il formato del bitstream e':
   side, x, y, z (in side units?), number of levels octree stream.
*/

class GeometryCoder {

 private:
  vcg::Box3f qbox; // powerof 2 quantized bounding box
  int origin[3];   //used to build qbox
  int logside;     //log2 of qbox side

 public:
  class Options {
   public:
    int coord_q;  //number of levels of quantization per component
    int normal_q; //number of levels of quantization per component
    int red_q, green_q, blue_q;  //color quantization
    float precision;
    Options(): coord_q(12), normal_q(10), red_q(5), green_q(6), blue_q(5), precision(0) {}
  };

  Options options;
  std::vector<ZPoint> zpoints;

  GeometryCoder() {}
  void setOptions(Options &opt) {
    options = opt;
  }

  /*void setPatch(Patch &patch) {
    //compute bounding box
    vcg::Box3f box;
    for(int i = 0; i < patch.nvert; i++) {
      box.Add(patch.coords[i]);
    }
    findLogBox(box);
    if(options.precision != 0)
      setCoordQSide(options.precision);
  } */
  void setCoordQ(int levels) { options.coord_q = levels; }
  void setNormalQ(int levels) { options.normal_q = levels; }
  void setRGBQ(int r, int g, int b) { options.red_q = r; options.green_q = g, options.blue_q = b; }

  void setCoordQSide(float precision) {  //set side of the quantization voxel
    int lside = Math::ilogbf(qbox.DimX());
    int lstep = Math::ilogbf(precision);
    options.coord_q = lside - lstep;
    if(options.coord_q < 0) options.coord_q = 0;
  }

  void quantize(Patch &patch, std::vector<int> &order) {
    setPatch(patch);
    if(options.coord_q > 21) {
      cout << "Warning: maximum quantization exceeded: " << options.coord_q << "/21" << endl;
      options.coord_q = 21;
    }

    //create zpoints
    zpoints.resize(patch.nvert);
    for(unsigned int i = 0; i < patch.nvert; i++)
      zpoints[i] = ZPoint(patch.coords[i], qbox, options.coord_q, i);


    std::sort(zpoints.rbegin(), zpoints.rend());//, std::greater<ZPoint>());

    //we need to remove duplicated vertices and save back ordering
    //for each vertex order contains its new position in zpoints
    order.resize(zpoints.size());
    int count = 0;

    order[zpoints[0].pos] = count;
    for(unsigned int i = 1; i < zpoints.size(); i++) {
      if(zpoints[i] != zpoints[count]) {
        count++;
        zpoints[count] = zpoints[i];
      }
      order[zpoints[i].pos] = count;
    }
    count++;
    zpoints.resize(count);
  }

  void encode(RangeEncoder<OutputStream> &encoder, nx::Node &node, nx::NodeData &data,
              nx::Signature &sig, std::vector<int> &order) {
    //encoder header
    encoder.encodeChar(options.coord_q);
    encoder.encodeChar(options.normal_q);
    encoder.encodeChar(options.red_q);
    encoder.encodeChar(options.green_q);
    encoder.encodeChar(options.blue_q);
    encoder.encodeArray(sizeof(uint)*3, (unsigned char *)(origin));
    encoder.encodeShort(RangeCoder::toUint(logside));

    encodeCoordinates(encoder, zpoints);

    if(sig.vertex.hasColors()) {
      //reorder original normals accordingly to new order.
      vector<vcg::Color4b> colors(zpoints.size());
      for(unsigned int i = 0; i < order.size(); i++) {
        colors[order[i]] = patch.colors[i];
      }
      int max_q[3] = { options.red_q, options.green_q, options.blue_q };
      std::vector<int> attr;
      attr.resize(zpoints.size());
      for(int comp = 0; comp < 3; comp++) {
        int max_value = (1<<max_q[comp]);
        for(unsigned int i = 0; i < zpoints.size(); i++) {
          int value = colors[i][comp];//mesh->vert[zpoints[i].pos].C()[comp];
          value *= max_value;
          value /= 256;
          if(value > max_value-1) value = max_value-1;
          attr[i] = value;
        }
        encodeAttributes(encoder, 2*max_value, attr);
      }
    }
  }

  void encodeCoordinates(RangeEncoder<OutputStream> &encoder, std::vector<ZPoint> &zpoints) {
    int bits = options.coord_q*3;
    std::vector<int> stack(bits+1);
    std::vector<char> output; //temporary vector: we need to invert it

    std::vector<int> levels;

    ZPoint &f = zpoints.back();
    for(int i = 0; i < bits; i++)
      stack[i] = f.testBit(i);


    for(int pos = zpoints.size()-2; pos >= 0; pos--) {
      ZPoint &p = zpoints[pos];
      ZPoint &q = zpoints[pos+1];
      int d = p.difference(q);
      assert(d < bits);
      assert(d >= 0);

      for(int i = 0; i < d; i++) {
        output.push_back(stack[i]);
        levels.push_back(i);
        stack[i] = p.testBit(i);
      }
      stack[d] = 2;
    }

    for(int i = 0; i < bits; i++) {
      output.push_back(stack[i]);
      levels.push_back(i);
    }


    vector<AdaptiveModel> models(bits+1);
    for(unsigned int i = 0; i< models.size(); i++)
      models[i].initAlphabet(3);


    vector<char> encoded(output.size()*2);
    int space = Lzjb::compress(&*output.begin(), &*encoded.begin(), output.size());
    cout << "lzjb initial: " << output.size() << " compression: " << space << " bpv: " << space*8/zpoints.size() << endl;
    cout << "basic bpv: " << output.size()*2/zpoints.size() << endl;
    //for better performance use 2 qs models (1 edge markov chain)
    int count[3];
    count[0] = count[1] = count[2] = 0;
    for(int i = (int)output.size()-1; i >= 0; i--) {
      int v = output[i];
      count[v]++;
      encoder.encodeSymbol(v, models[levels[i]]);

//      encoder.encodeSymbol(v, model);
    }
    double n = output.size();
    double p[3];
    p[0] = count[0]/n;
    p[1] = count[1]/n;
    p[2] = count[2]/n;
    cout << "p: " << p[0] << " " << p[1] << " " << p[2] << endl;
    double entropy = -(p[0]*log2(p[0]) + p[1]*log2(p[1]) + p[2]*log2(p[2]));
    cout << "Entropy: " << entropy << endl;
    cout << "BPV: " << output.size()*entropy/zpoints.size() << endl;

  }

  void decode(RangeDecoder<InputStream> &decoder, Patch &patch, Signature &sig) {
    options.coord_q = decoder.decodeChar();
    options.normal_q = decoder.decodeChar();
    options.red_q = decoder.decodeChar();
    options.green_q = decoder.decodeChar();
    options.blue_q = decoder.decodeChar();
    decoder.decodeArray(sizeof(uint)*3, (unsigned char *)origin);
    logside = decoder.decodeShort();
    logside = RangeCoder::toInt(logside);
    buildQBox();

    zpoints.resize(patch.nvert);
    decodeCoordinates(decoder, zpoints, patch);

    //decoder attributes
    if(sig.vertex.hasColors()) {
      int max_q[3] = { options.red_q, options.green_q, options.blue_q };
      std::vector<int> attr;
      attr.resize(zpoints.size());
      for(int comp = 0; comp < 3; comp++) {
        int max_value = (1<<max_q[comp]);
        decodeAttributes(decoder, 2*max_value, attr);
        for(unsigned int i = 0; i < attr.size(); i++)
          patch.colors[i][comp] = attr[i]*256/max_value;
      }
      for(int i = 0; i < patch.nvert; i++)
        patch.colors[i][3] = 255;
    }
  }

  void decodeCoordinates(RangeDecoder<InputStream> &decoder, std::vector<ZPoint> &zpoints, Patch &patch) {
    //AdaptiveModel model(3);

    int bits = options.coord_q*3;
    int level = bits-1;

    int *stack = new int[level+1];
    int stacksize = 0;

    vector<AdaptiveModel> models(bits+1);
    for(unsigned int i = 0; i< models.size(); i++)
      models[i].initAlphabet(3);

    int current_point = 0;
    /* A small optimization on the order of 10%:
       try to set only the 1 and not the zeroes
       but the problem is we are working ona BIT level.... */
    while(1) {
      for(; level >= 0; level--) {
        ZPoint &current = zpoints[current_point];//zpoints.back();
        //int symbol = decoder.decodeSymbol(model);
        uint32_t symbol = decoder.decodeSymbol(models[level]);

        if(symbol == 2) {
          stack[stacksize++] = level;
          symbol = 0;
        }
        current.setBit(level, symbol);
      }
      if(stacksize == 0) break;
      level = stack[--stacksize];
      zpoints[current_point+1] = zpoints[current_point];

      current_point++;
      zpoints[current_point].setBit(level, 1);
      level--;
    }
    for(unsigned int i = 0; i < zpoints.size(); i++)
      patch.coords[i] = zpoints[i].toPoint(qbox, options.coord_q);
    delete []stack;
  }


  void encodeNormals(RangeEncoder<OutputStream> &encoder, Patch &patch, std::vector<McFace> &faces, std::vector<int> &order) {
    //1 compute normalf from faces and zpoints
   vector<vcg::Point3f> points(zpoints.size());
   vector<vcg::Point3f> normals(zpoints.size(), vcg::Point3f(0, 0, 0));
   //recover quantized positions
   for(unsigned int i = 0; i < zpoints.size(); i++) {
      points[i] = zpoints[i].toPoint(qbox, options.coord_q);
    }
   //accumulate normals on vertices
   for(unsigned int i = 0; i < faces.size(); i++) {
     McFace &face = faces[i];

     vcg::Point3f &p0 = points[face.f[0]];
     vcg::Point3f &p1 = points[face.f[1]];
     vcg::Point3f &p2 = points[face.f[2]];
     vcg::Point3f n = (( p1 - p0) ^ (p2 - p0));
     normals[face.f[0]] += n;
     normals[face.f[1]] += n;
     normals[face.f[2]] += n;
   }
   //normalize
   for(unsigned int i = 0; i < normals.size(); i++)
     normals[i].Normalize();

    int max_value = (1<<(options.normal_q-1));

    AdaptiveModel nmodel(2*max_value, (1<<16));
    AdaptiveModel signmodel(2, 20);
    /*StaticModel nmodel(2*max_value); //for speed...
    nmodel.setProbability(0, 8000);
    for(int i = 1; i < 10; i++) {
      assert(2*max_value > 2*i);
      int prob = 4096>>i;
      nmodel.setProbability(2*i-1, prob);
      nmodel.setProbability(2*i, prob);
    }*/

    //reorder original normals accordingly to new order.
    vector<vcg::Point3s> reordered(zpoints.size());
    for(unsigned int i = 0; i < order.size(); i++) {
      reordered[order[i]] = patch.normals[i];
    }
    //store difference between original and predicted
    for(int comp = 0; comp < 2; comp++) {
      for(unsigned int i = 0; i < zpoints.size(); i++) {
        float n = normals[i][comp];
        n *= max_value;
        if(n < -max_value+1) n = -max_value+1;
        if(n > max_value-1) n = max_value-1;
        int actual = reordered[i][comp]*max_value/32767;
        int d = (int)(actual - n); //actual value - predicted
        d = RangeCoder::toUint(d);
        if(d >= 2*max_value) d = 2*max_value-1;

        encoder.encodeSymbol(d, nmodel);
      }
    }
    for(unsigned int i = 0; i < zpoints.size(); i++) {
      bool signbit = (normals[i][2] > 0);
      encoder.encodeSymbol(signbit, signmodel);
    }
  }

  void decodeNormals(RangeDecoder<InputStream> &decoder, Patch &patch) {

    //1 compute normalf from faces and points
    vector<vcg::Point3f> normals(zpoints.size(), vcg::Point3f(0, 0, 0));

    for(int i = 0; i < patch.nface; i++) {
      uint16_t *face = &(patch.triangles[i*3]);

      vcg::Point3f &p0 = patch.coords[face[0]];
      vcg::Point3f &p1 = patch.coords[face[1]];
      vcg::Point3f &p2 = patch.coords[face[2]];
      vcg::Point3f n = (( p1 - p0) ^ (p2 - p0));
      normals[face[0]] += n;
      normals[face[1]] += n;
      normals[face[2]] += n;
    }

    //normalize
    for(unsigned int i = 0; i < normals.size(); i++)
      normals[i].Normalize();

    int max_value = (1<<(options.normal_q-1));

    AdaptiveModel nmodel(2*max_value, (1<<16));
    AdaptiveModel signmodel(2, 20);
    /*StaticModel nmodel(2*max_value); //for speed...
    nmodel.setProbability(0, 8000);
    for(int i = 1; i < 10; i++) {
      assert(2*max_value > 2*i);
      int prob = 4096>>i;
      nmodel.setProbability(2*i-1, prob);
      nmodel.setProbability(2*i, prob);
    }*/


    //get difference between original and predicted
    for(int comp = 0; comp < 2; comp++) {
      for(unsigned int i = 0; i < patch.nvert; i++) {
        float n = normals[i][comp];
        //cout << "N: " << n << endl;
        n *= max_value;
        if(n < -max_value+1) n = -max_value+1;
        if(n > max_value-1) n = max_value-1;
        int diff = decoder.decodeSymbol(nmodel);
        diff = RangeCoder::toInt(diff);
        int actual = (int)((n + diff)*32767/max_value);
        patch.normals[i][comp] = actual;
      }
    }

    for(int i = 0; i < patch.nvert; i++) {
      vcg::Point3s &n = patch.normals[i];
      float x = n[0];
      float y = n[1];
      float z = 32767.0f*32767.0f - x*x - y*y;

      if(z < 0) z = 0;
      z = sqrt(z);
      //sign
      if(z > 32767) z = 32767;
      bool signbit = decoder.decodeSymbol(signmodel);
      if(!signbit) z = -z;
      n[2] = (int16_t)z;
    }

/*    int max_value = (1<<(options.normal_q-1));
    BasicModel nmodel(2*max_value, (1<<16));
    vcg::tri::UpdateNormals<MESH>::PerVertexNormalized(*mesh);
    for(int comp = 0; comp < 3; comp++) {
      vector<int> c(mesh->vert.size());
      for(unsigned int i = 0; i < mesh->vert.size(); i++) {
        float n = mesh->vert[i].N()[comp];
        n += 1;
        n *= max_value/2;
        if(n < 0) n = 0;
        if(n > max_value-1) n = max_value-1;
        int predicted = (int)n;
        int diff = decoder.decodeSymbol(nmodel);
        diff = RangeCoder::toInt(diff);
        n = predicted + diff;
        n /= max_value/2;
        n -= 1;
        mesh->vert[i].N()[comp] = n;
      }
    }*/
  }

  void encodeAttributes(RangeEncoder<OutputStream> &encoder, int max_value, std::vector<int> &values) {
    AdaptiveModel nmodel(2*max_value, 20);

    int on = 0; //old normal;
    for(unsigned int i = 0; i < values.size(); i++) {

      int &n = values[i];
      int d = n - on;
      d = RangeCoder::toUint(d);
//      assert(d < 2*max_value);
      encoder.encodeSymbol(d, nmodel);
      on = n;
    }
  }

  void decodeAttributes(RangeDecoder<InputStream> &decoder, int max_value, std::vector<int> &values) {
    AdaptiveModel nmodel(2*max_value, 20);

    int on = 0; //old normal;
    for(unsigned int i = 0; i < values.size(); i++) {
      int &n = values[i];
      int d = decoder.decodeSymbol(nmodel);
      d = RangeCoder::toInt(d);
//      assert(d < 2*max_value);
      n = on + d;
      on = n;
    }
  }

 private:

  //builds qbox from origin and logside
  void buildQBox() {
    float side = pow(2.0f, logside);
    for(int i = 0; i < 3; i++)
      qbox.min[i] = origin[i]*side;
    qbox.max = qbox.min + vcg::Point3f(2*side, 2*side, 2*side);
  }

  void findLogBox(vcg::Box3f &box) {
    vcg::Point3f d = box.Dim();

    float side = d[0];
    if(d[1] > side) side = d[1];
    if(d[2] > side) side = d[2];
    logside = Math::log2(side); //exponent of the side
    float new_side = pow(2.0f, logside);
    if(new_side < side) {
      logside++;
      new_side = pow(2.0f, logside);
    }
    for(int i = 0; i < 3; i++)
      origin[i] = floor(box.min[i]/new_side);
    buildQBox();
  }
};


#endif // GEOMETRY_H
