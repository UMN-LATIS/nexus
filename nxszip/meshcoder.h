#ifndef MESHCODER_H
#define MESHCODER_H

#include <vector>
#include <deque>
#include <algorithm>

#include "cstream.h"
#include "tunstall.h"
#include "connectivity.h"
#include "fpu_precision.h"
#include "zpoint.h"
#include "../common/nexusdata.h"

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
    void encode();
    void decode(int len, uchar *input);

    //compression stats
    int coord_size;
    int normal_size;
    int color_size;
    int face_size;

    CStream stream;
private:
    nx::Node &node;
    nx::NodeData &data;
    nx::Patch *patches;
    nx::Signature sig;

    vcg::Point3i min, max; //minimum position of vertices
    int coord_bits; //number of bits for coordinates.
#ifdef PARALLELOGRAM
	std::vector<vcg::Point3i> zpoints;
#else
	std::vector<ZPoint> zpoints;
#endif
    std::vector<int> order; //for every original face points to it's zpoint.
    std::vector<bool> boundary;
    int nface;

    Connectivity connectivity;
    void quantize();
    void encodeCoordinates();
    void decodeCoordinates();

    void encodeFaces();
    void decodeFaces();

    void encodeNormals();
    void decodeNormals();

    void encodeColors();
    void decodeColors();

    void computeNormals(vcg::Point3f *p, int nvert, uint16_t *f, int nface, vcg::Point3s *n);
    void markBoundary(uint16_t *faces, int nfaces, int nvert);

    void encodeIndex(std::vector<uchar> &diffs, BitStream &stream, int val);
    int decodeIndex(uchar diff, BitStream &stream);
//    static void encodeIndex(int val, std::vector<uchar> &nbits, BitStream &bitstream);
//    static int decodeIndex(std::vector<uchar> &nbits, Bitstream &bitstream);
};

#endif // MESHCODER_H
