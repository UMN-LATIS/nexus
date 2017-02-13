#ifndef MESH_ENCODER_H
#define MESH_ENCODER_H

#include <vector>
#include <deque>
#include <algorithm>

#include <vcg/space/box3.h>

#include "connectivity_coder.h"
#include "fpu_precision.h"
#include "bytestream.h"
#include "range.h"
#include "zpoint.h"
#include "model.h"
#include "../common/nexusdata.h"

#ifdef MECO_TEST
#include "watch.h"
#endif



/* come deve funzionare sto attrezzo:
 *
 *1) quantizza vertici con errore fissato (potenza di 2) e usando le quadriche per quantizzare l'interno(da fare in seguito)
 *   mi servono le facce per farlo a modo. salva i vertici.
 *3 salva le facce.
 *2) salva  normal (stima + diff)
  4) colori: at the moment just difference encoded. Not that nice. could use some smarter move.

 */

class MeshCoder {
public:
    float error; //quadric error on internal vertices, 0 on boundary vertices
    int coord_q; //global power of 2 quantization.
    int norm_q;  //normal bits
    int color_q[4]; //color bits
    int tex_q;   //texture bits

public:
    MeshCoder(nx::Node &_node, nx::NodeData &_data, nx::Patch *_patches, nx::Signature &_sig):
        node(_node), data(_data), patches(_patches), sig(_sig) {}
    void encode(std::vector<uchar> &output);
    void decode(unsigned char *buffer, int size);

    //compression stats
    int coord_size;
    int normal_size;
    int color_size;
    int face_size;

private:
    nx::Node &node;
    nx::NodeData &data;
    nx::Patch *patches;
    nx::Signature sig;

    vcg::Point3i min, max; //minimum position of vertices
    int coord_bits; //number of bits for coordinates.
    std::vector<ZPoint> zpoints;
    std::vector<int> order; //for every original face points to it's zpoint.
    std::vector<bool> boundary;

    Connectivity connectivity;
    void quantize(nx::Node &node, nx::NodeData &patch);
    void encodeCoordinates(RangeEncoder<OutputStream> &encoder);
    void decodeCoordinates(RangeDecoder<InputStream> &decoder);

    void encodeFaces(RangeEncoder<OutputStream> &encoder);
    void decodeFaces(RangeDecoder<InputStream> &decoder);

    void encodeNormals(RangeEncoder<OutputStream> &encoder);
    void decodeNormals(RangeDecoder<InputStream> &decoder);

    void encodeColors(RangeEncoder<OutputStream> &encoder);
    void decodeColors(RangeDecoder<InputStream> &decoder);

    static void encodeIndex(RangeEncoder<OutputStream> &encoder, int val, AdaptiveModel &model);
    static int decodeIndex(RangeDecoder<InputStream> &decoder, AdaptiveModel &model);
};

#endif // MESH_ENCODER_H
