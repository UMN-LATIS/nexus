#include <QTime>
#include "mesh_coder.h"
#include "bitstream.h"

using namespace nx;
using namespace vcg;
//side is the edge face.f[side] face.f[side+1]


void MeshCoder::encode(std::vector<uchar> &out) {
    FpuPrecision::store();
    FpuPrecision::setFloat();

    OutputStream output;
    RangeEncoder<OutputStream> encoder(output);

    QTime clock;

    clock.start();
    /* ENCODE GEOMETRY */
    quantize(node, data); //quantize vector position
    encodeCoordinates(encoder);
    coord_size = output.size();

    cout << "coords: " << clock.elapsed() << endl;
    encodeFaces(encoder);
    face_size = output.size() - coord_size;

    cout << "Faces: " << clock.elapsed() << endl;
    if(sig.vertex.hasNormals())
        encodeNormals(encoder);

    cout << "Normals: " << clock.elapsed() << endl;

    normal_size = output.size() - face_size - coord_size;

    if(sig.vertex.hasColors())
        encodeColors(encoder);

    cout << "colors: " << clock.elapsed() << endl;

    color_size = output.size() - face_size - coord_size - normal_size;


    encoder.flush();

    node.nvert = zpoints.size(); //needed because quantizatiojn might remove some vertices.
    //need to be done here: nvert used to read nodedata.


    cout << "Coord bpv: " << coord_size*8/(float)node.nvert << endl;
    cout << "Face bpv: " << face_size*8/(float)node.nvert << endl;
    cout << "Norm bpv: " << normal_size*8/(float)node.nvert << endl;
    cout << "Color bpv: " << color_size*8/(float)node.nvert << endl;

    swap(out, output);
    FpuPrecision::restore();
}

void MeshCoder::decode(unsigned char *input, int len) {
    FpuPrecision::store();
    FpuPrecision::setFloat();

    InputStream stream((unsigned char *)input, len);
    RangeDecoder<InputStream> decoder(stream);

    QTime time;
    time.start();
    decodeCoordinates(decoder);

    cout << "coords: " << time.elapsed() << endl;
    decodeFaces(decoder);
    cout << "faces: " << time.elapsed() << endl;

    /*    uint16_t *faces = data.faces(sig, node.nvert);
    int start = 0;
    for(int p = node.first_patch; p < node.last_patch(); p++) {
        int end = patches[p].triangle_offset;
        connectivity.decode(decoder, end - start, faces + start);
    } */

    if(sig.vertex.hasNormals())
        decodeNormals(decoder);

    cout << "normals: " << time.elapsed() << endl;
    if(sig.vertex.hasColors())
        decodeColors(decoder);
    cout << "colors: " << time.elapsed() << endl;
    FpuPrecision::restore();
}

void MeshCoder::quantize(Node &node, NodeData &patch) {

    //quantize vertex position and compute min.
    float side = pow(2.0f, (float)coord_q);

    //TODO use quadric to quantize for better rate?  probably not worth it, for so few bits.

    std::vector<Point3i> qpoints(node.nvert);
    for(unsigned int i = 0; i < node.nvert; i++) {
        Point3f &p = patch.coords()[i];
        Point3i &q = qpoints[i];
        for(int k = 0; k < 3; k++) {
            q[k] = (int)floor(p[k]/side + 0.5f);
            if(k == 1)
                q[k] *= 1.0;
            if(i == 0) {
                min[k] = q[k];
                max[k] = q[k]; }
            else {
                if(min[k] > q[k]) min[k] = q[k];
                if(max[k] < q[k]) max[k] = q[k];
            }
        }
    }
    Point3i d = max - min;
    coord_bits = 1+std::max(std::max(Math::log2(d[0]), Math::log2(d[1])), Math::log2(d[2]));
    assert(coord_bits > 2);

    //TODO use boundary information for better quantization on the inside.

    zpoints.resize(node.nvert);
    for(int i = 0; i < node.nvert; i++) {
        Point3i q = qpoints[i] - min;
        assert(q[0] >= 0);
        assert(q[1] >= 0);
        assert(q[2] >= 0);
        assert(coord_bits >= Math::log2(q[0]));
        assert(coord_bits >= Math::log2(q[1]));
        assert(coord_bits >= Math::log2(q[2]));

        zpoints[i] = ZPoint(q[0], q[1], q[2], coord_bits, i);
    }

    std::sort(zpoints.rbegin(), zpoints.rend());//, std::greater<ZPoint>());

    //we need to remove duplicated vertices and save back ordering
    //for each vertex order contains its new position in zpoints
    order.resize(node.nvert);
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

void MeshCoder::encodeCoordinates(RangeEncoder<OutputStream> &encoder) {

    int start = encoder.output.size();

//#define OPTIMAL
#ifdef OPTIMAL

    encoder.encodeInt(RangeCoder::toUint(min[0])); //can be negative
    encoder.encodeInt(RangeCoder::toUint(min[1]));
    encoder.encodeInt(RangeCoder::toUint(min[2]));
    encoder.encodeChar(RangeCoder::toUint(coord_q));
    encoder.encodeChar(coord_bits);

    cout << "COORDQ: " << coord_q << endl;

    int bits = coord_bits*3;
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

    AdaptiveModel model(3);
    vector<AdaptiveModel> models(bits+1);
    for(unsigned int i = 0; i< models.size(); i++)
        models[i].initAlphabet(3);



    for(int i = (int)output.size()-1; i >= 0; i--) {
        int v = output[i];
        encoder.encodeSymbol(v, models[levels[i]]);
        //encoder.encodeSymbol(v, model);
    }
#else    
    encoder.output.writeInt(min[0]);
    encoder.output.writeInt(min[1]);
    encoder.output.writeInt(min[2]);
    encoder.output.writeInt(coord_q);
    encoder.output.writeByte(coord_bits);

    cout << "COORD BITSA: " << coord_bits << endl;
    Obstream bitstream;
    bitstream.write(zpoints[0].bits, coord_bits);
    std::vector<int> diffs;
    for(int pos = 0; pos < zpoints.size()-1; pos++) {
        ZPoint &p = zpoints[pos];
        ZPoint &q = zpoints[pos+1];
        int d = p.difference(q);
        diffs.push_back(d);
        //we can get away with d diff bits (should be d+1) because the first bit will always be 1 (sorted q> p)
        bitstream.write(q.bits, d); //rmember to add a 1, since
    }
    bitstream.flush();

    cout << "SIZE: " << bitstream.size() << endl;
    encoder.output.writeInt(bitstream.size());
    int begin = encoder.output.size();
    encoder.output.resize(encoder.output.size() + bitstream.size()*sizeof(quint64));
    memcpy(&encoder.output[begin], &*bitstream.begin(), bitstream.size()*sizeof(quint64));

    cout << "outputsize: " << encoder.output.size() << endl;

    AdaptiveModel model(coord_bits*3);
    cout << "diffs size: " << diffs.size() << " node.nvert " << node.nvert << endl;
    assert(diffs.size() == zpoints.size()-1);
    for(int i = 0; i < diffs.size(); i++)
        encoder.encodeSymbol(diffs[i], model);
#endif

    cout << "Bytes: " << encoder.output.size() - start << endl;

    //entropy:
}

void MeshCoder::decodeCoordinates(RangeDecoder<InputStream> &decoder) {

    zpoints.resize(node.nvert);

#ifdef OPTIMAL

    min[0] = RangeCoder::toInt(decoder.decodeInt());
    min[1] = RangeCoder::toInt(decoder.decodeInt());
    min[2] = RangeCoder::toInt(decoder.decodeInt());
    coord_q = RangeCoder::toInt(decoder.decodeChar());
    coord_bits = decoder.decodeChar();

    cout << "COORDQ: " << coord_q << endl;
    cout << "MIN: " << min[0] << " " << min[1] << " " << min[2] << endl;

    int bits = coord_bits*3;
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
        assert(current_point < zpoints.size());
        zpoints[current_point].setBit(level, 1);
        level--;

    }
    delete []stack;
    float step = pow(2.0, coord_q);
    Point3f *coords = data.coords();
    for(unsigned int i = 0; i < zpoints.size(); i++) {
        coords[i] = zpoints[i].toPoint(min, step, coord_bits);
        Point3f &q = coords[i];
    }
#else

    cout << "POINT in stream : " << decoder.input.count << endl;
    min[0] = decoder.input.readInt();
    min[1] = decoder.input.readInt();
    min[2] = decoder.input.readInt();
    coord_q = decoder.input.readInt();
    coord_bits = decoder.input.readByte();

    cout << "COORD BITSA: " << coord_bits << endl;

    int size = decoder.input.readInt();
    cout << "SIZE: " << size << endl;

    Ibstream bitstream(size, (uint64_t *)(decoder.input.mem + decoder.input.count));
    decoder.input.count += size*sizeof(quint64);

    cout << "inputy size: " << decoder.input.count << endl;

    AdaptiveModel model(coord_bits*3);
    bitstream.read(coord_bits, zpoints[0].bits);
    for(int i = 1; i < zpoints.size(); i++) {
        ZPoint &p = zpoints[i];
        p = zpoints[i-1];
        int d = decoder.decodeSymbol(model);
        p.setBit(d, 1);
        bitstream.read(d, p.bits);
    }
#endif

}

Point3f norm(const Point3f &a, const Point3f &b, const Point3f &c) {
    return (b - a)^(c - a);
}

//compact in place faces in data, update patches information, compute topology and encode each patch.
void MeshCoder::encodeFaces(RangeEncoder<OutputStream> &encoder) {
    uint16_t *triangles = data.faces(sig, node.nvert);
    vector<McFace> faces;
    int start =  0;
    int count = 0;
    for(int p = node.first_patch; p < node.last_patch(); p++) {
        faces.clear();
        Patch &patch = patches[p];
        uint end = patch.triangle_offset;
        for(unsigned int i = start; i < end; i++) {
            uint16_t *face = triangles + i*3;

            McFace f(order[face[0]], order[face[1]], order[face[2]]);
            //keep degenerate faces (otherwise when compressin we would need 2 passes for the index.
            if(f.f[0] == f.f[1] || f.f[0] == f.f[2] || f.f[1] == f.f[2])
                continue;

            faces.push_back(f);
            count++;
        }
        patch.triangle_offset = count;
        start = end;

        connectivity.encode(encoder, faces);
    }


    node.nface = count;
}

void MeshCoder::decodeFaces(RangeDecoder<InputStream> &decoder) {
    uint16_t *faces = data.faces(sig, node.nvert);
    int start = 0;
    for(int p = node.first_patch; p < node.last_patch(); p++) {
        Patch &patch = patches[p];
        uint end = patch.triangle_offset;
        connectivity.decode(decoder, end - start, faces + start*3);
        start = end;
    }
}

void MeshCoder::encodeNormals(RangeEncoder<OutputStream> &encoder) {
    //1 compute normalf from faces and zpoints
    vector<vcg::Point3f> points(zpoints.size());
    vector<vcg::Point3f> normals(zpoints.size(), vcg::Point3f(0, 0, 0));
    //recover quantized positions
    float step = pow(2.0f, (float)coord_q);
    for(unsigned int i = 0; i < zpoints.size(); i++) {
        points[i] = zpoints[i].toPoint(min, step, coord_bits);
    }
    //accumulate normals on vertices
    for(unsigned int i = 0; i < node.nface; i++) {
        uint16_t *face = &(data.faces(sig, node.nvert)[i*3]);
        uint16_t f[3];
        f[0] = order[face[0]];
        f[1] = order[face[1]];
        f[2] = order[face[2]];
        if(f[0] == f[1] || f[0] == f[2] || f[1] == f[2])
            continue;

        vcg::Point3f &p0 = points[f[0]];
        vcg::Point3f &p1 = points[f[1]];
        vcg::Point3f &p2 = points[f[2]];
        vcg::Point3f n = (( p1 - p0) ^ (p2 - p0));
        normals[f[0]] += n;
        normals[f[1]] += n;
        normals[f[2]] += n;
    }
    //normalize
    for(unsigned int i = 0; i < normals.size(); i++)
        normals[i].Normalize();

    int max_value = (1<<(norm_q-1));


    encoder.encodeChar(norm_q);

    Point3s *norms = data.normals(sig, node.nvert);
    vector<Point3s> nor(zpoints.size());
    for(int i = 0; i < node.nvert; i++)
        nor[order[i]] = norms[i];

    //store difference between original and predicted
    //AdaptiveModel nmodel(Math::log2(2*max_value) + 2, (1<<16));
    AdaptiveLogModel nmodel(Math::log2(2*max_value)+1);
    AdaptiveModel signmodel(2, 20);
    for(unsigned int i = 0; i < zpoints.size(); i++) {
        for(int comp = 0; comp < 2; comp++) {
            float n = normals[i][comp];
            n *= max_value;
            if(n < -max_value+1) n = -max_value+1;
            if(n > max_value-1) n = max_value-1;
            int actual = nor[i][comp]*max_value/32767;
            int d = (int)(actual - n); //act1ual value - predicted

            d = RangeCoder::toUint(d);
            //if(d >= 2*max_value) d = 2*max_value-3; //accoount for +1 due to encoder.
            encoder.encodeSymbol(d, nmodel);
        }
        bool signbit = (normals[i][2]*nor[i][2] < 0);
        encoder.encodeSymbol(signbit, signmodel);
    }
}

void MeshCoder::decodeNormals(RangeDecoder<InputStream> &decoder) {
    //1 compute normalf from faces and points
    vector<vcg::Point3f> normals(zpoints.size(), vcg::Point3f(0, 0, 0));

    uint16_t *faces = data.faces(sig, node.nvert);
    Point3f *coords = data.coords();
    for(int i = 0; i < node.nface; i++) {
        uint16_t *face = faces + i*3;

        vcg::Point3f &p0 = coords[face[0]];
        vcg::Point3f &p1 = coords[face[1]];
        vcg::Point3f &p2 = coords[face[2]];
        vcg::Point3f n = (( p1 - p0) ^ (p2 - p0));
        normals[face[0]] += n;
        normals[face[1]] += n;
        normals[face[2]] += n;
    }

    //normalize
    for(unsigned int i = 0; i < normals.size(); i++)
        normals[i].Normalize();

    norm_q = decoder.decodeChar();
    int max_value = (1<<(norm_q-1));

    //AdaptiveModel nmodel(2*max_value, (1<<16));
    AdaptiveLogModel nmodel(Math::log2(2*max_value)+1);
    AdaptiveModel signmodel(2, 20);

    Point3s *norms = data.normals(sig, node.nvert);
    //get difference between original and predicted
    for(unsigned int i = 0; i < node.nvert; i++) {
        vcg::Point3s &N = norms[i];
        for(int comp = 0; comp < 2; comp++) {
            float n = normals[i][comp];
            n *= max_value;
            if(n < -max_value+1) n = -max_value+1;
            if(n > max_value-1) n = max_value-1;
            int diff = decoder.decodeSymbol(nmodel);
            diff = RangeCoder::toInt(diff);

            int actual = (int)((n + diff)*32767/max_value);
            N[comp] = actual;
        }
        float x = N[0];
        float y = N[1];
        float z = 32767.0f*32767.0f - x*x - y*y;

        if(z < 0) z = 0;
        z = sqrt(z);
        //sign
        if(z > 32767) z = 32767;
        bool signbit = decoder.decodeSymbol(signmodel);
        if(normals[i][2] < 0 != signbit)
            z = -z;
        N[2] = (int16_t)z;
    }
}

Color4b toYCC(Color4b s) {
    Color4b e;
    e[0] =              0.299000*s[0] + 0.587000*s[1] + 0.114000*s[2];
    e[1] = 128        - 0.168736*s[0] - 0.331264*s[1] + 0.500000*s[2];
    e[2] = 128        + 0.500000*s[0] - 0.428688*s[1] - 0.081312*s[2];
    e[3] = s[3];
    return e;
}
Color4b toRGB(Color4b e) {
    Color4b s;
    int r = e[0]                        + 1.40200*(e[2] - 128);
    int g = e[0] - 0.34414*(e[1] - 128) - 0.71414*(e[2] - 128);
    int b = e[0] + 1.77200*(e[1] - 128);
    s[3] = e[3];
    s[0] = std::max(0, std::min(255, r));
    s[1] = std::max(0, std::min(255, g));
    s[2] = std::max(0, std::min(255, b));

    return s;
}

void MeshCoder::encodeColors(RangeEncoder<OutputStream> &encoder) {

    Color4b *colors = data.colors(sig, node.nvert);
    vector<vcg::Color4b> values(zpoints.size());
    for(unsigned int i = 0; i < order.size(); i++)
        values[order[i]] = toYCC(colors[i]);

    for(int k = 0; k < 4; k++)
        encoder.encodeChar(color_q[k]);

    for(int k = 0; k < 4; k++) {
        int max_value = (1<<(color_q[k]));
        int step = (1<<(8 - color_q[k]));
        AdaptiveModel model(2*max_value, 1<<16);

        int on = 0; //old color
        for(unsigned int i = 0; i < values.size(); i++) {
            Color4b c = values[i];
            int n = c[k];

            n = (n/(float)step + 0.5);
            if(n >= max_value) n = max_value-1;
            if(n < 0) n = 0;
            int d = n - on;
            d = RangeCoder::toUint(d);
            encoder.encodeSymbol(d, model);
            on = n;
        }
    }
}

void MeshCoder::decodeColors(RangeDecoder<InputStream> &decoder) {
    Color4b *colors = data.colors(sig, node.nvert);

    for(int k = 0; k < 4; k++)
        color_q[k] = decoder.decodeChar();

    for(int k = 0; k < 4; k++) {
        int max_value = (1<<(color_q[k]));
        int shift = 8 - color_q[k];
        AdaptiveModel model(2*max_value, 1<<16);
        AdaptiveModel nmodel(9, 1<<16);
        int on = 0; //old normal;
        for(unsigned int i = 0; i < node.nvert; i++) {
            Color4b &c = colors[i];
            uchar &n = c[k];

            int d = decoder.decodeSymbol(model);
            d = RangeCoder::toInt(d);
            //      assert(d < 2*max_value);
            assert(on + d >= 0 && on + d < 256);
            n = on + d;
            on = n;
            n <<= shift;
        }
    }
    for(int i = 0; i < node.nvert; i++) {
        Color4b &c = colors[i];
        c = toRGB(c);
    }
}

void MeshCoder::encodeIndex(RangeEncoder<OutputStream> &encoder, int val, AdaptiveModel &model) {
    val = RangeCoder::toUint(val);

    int ret = Math::log2(val);
    if(ret + 1 >= model.size) {
        cerr << "Ret: " << ret << " Model.size: " << model.size << " val: " << val << endl;
    }
    assert(ret + 1 < model.size);
    encoder.encodeSymbol(ret+1, model);
    if(ret >= 1) {
        int range = 1<<(ret);
        val -= range;
        encoder.encodeRange(val, val+1, range);
    }
}

int MeshCoder::decodeIndex(RangeDecoder<InputStream> &decoder, AdaptiveModel &model) {

    int ret = decoder.decodeSymbol(model);
    int val;
    if(ret <= 1) {
        val = ret;
    } else {
        int range = 1<<(ret-1);
        uint32_t c = decoder.getCurrentCount(range);
        decoder.removeRange(c, c+1, range);
        val = range + c;
    }
    val = RangeCoder::toInt(val);
    return val;
}

