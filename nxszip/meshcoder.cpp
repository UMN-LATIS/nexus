#include <QTime>
#include "meshcoder.h"

using namespace nx;
using namespace vcg;
//side is the edge face.f[side] face.f[side+1]


void MeshCoder::encode() {
    FpuPrecision::store();
    FpuPrecision::setFloat();

    stream.reserve(node.nvert);
    QTime clock;

    clock.start();
    /* ENCODE GEOMETRY */
    quantize(); //quantize vector position
#ifndef PARALLELOGRAM
    encodeCoordinates();
#endif
    coord_size = stream.size();

    if(sig.face.hasIndex())
        encodeFaces();
    face_size = stream.size() - coord_size;

	if(sig.vertex.hasNormals())
		encodeNormals();
    normal_size = stream.size() - face_size - coord_size;

	if(sig.vertex.hasColors())
		encodeColors();
    color_size = stream.size() - face_size - coord_size - normal_size;

    node.nvert = zpoints.size(); //needed because quantizatiojn might remove some vertices.
    node.nface = nface;
/*
    cout << "Coord bpv: " << coord_size*8/(float)node.nvert << endl;
    cout << "Face bpv: " << face_size*8/(float)node.nvert << endl;
    cout << "Norm bpv: " << normal_size*8/(float)node.nvert << endl;
    cout << "Color bpv: " << color_size*8/(float)node.nvert << endl;
*/

    FpuPrecision::restore();
}

void MeshCoder::decode(int len, uchar *input) {
    FpuPrecision::store();
    FpuPrecision::setFloat();

    stream.init(len, input);

    QTime time;
    time.start();
#ifndef PARALLELOGRAM
    decodeCoordinates();
#endif


    if(sig.face.hasIndex())
        decodeFaces();

	if(sig.vertex.hasNormals())
		decodeNormals();

	if(sig.vertex.hasColors())
		decodeColors();

    FpuPrecision::restore();
}

void MeshCoder::quantize() {

    //quantize vertex position and compute min.
    float side = pow(2.0f, (float)coord_q);

    //TODO use quadric to quantize for better rate?  probably not worth it, for so few bits.
#ifdef PARALLELOGRAM
	zpoints.resize(node.nvert);
	std::vector<Point3i> &qpoints = zpoints;
#else
    std::vector<Point3i> qpoints(node.nvert);
#endif
    for(unsigned int i = 0; i < node.nvert; i++) {
        Point3f &p = data.coords()[i];
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
    //assert(coord_bits > 2);

    //TODO use boundary information for better quantization on the inside.
#ifdef PARALLELOGRAM
	for(int i = 0; i < node.nvert; i++) {
		qpoints[i] -= min;
	}
#else
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
#endif
}

void MeshCoder::encodeCoordinates() {
#ifndef PARALLELOGRAM
    stream.write<int>(min[0]);
    stream.write<int>(min[1]);
    stream.write<int>(min[2]);
    stream.write<char>(coord_q);
    stream.write<char>(coord_bits);

    BitStream bitstream(node.nvert/2);
    bitstream.write(zpoints[0].bits, coord_bits*3);

    std::vector<uchar> diffs;
    for(int pos = 0; pos < zpoints.size()-1; pos++) {
        ZPoint &p = zpoints[pos];
        ZPoint &q = zpoints[pos+1];
        uchar d = p.difference(q);
        diffs.push_back(d);
        //we can get away with d diff bits (should be d+1) because the first bit will always be 1 (sorted q> p)
        bitstream.write(q.bits, d); //rmember to add a 1, since
    }

    bitstream.flush();

    stream.write(bitstream);

    Tunstall tunstall;
    tunstall.compress(stream, &*diffs.begin(), diffs.size());
#endif
}

void MeshCoder::decodeCoordinates() {
#ifndef PARALLELOGRAM
    min[0] = stream.read<int>();
    min[1] = stream.read<int>();
    min[2] = stream.read<int>();
    coord_q = stream.read<char>();
    coord_bits = stream.read<char>();

    BitStream bitstream;
    stream.read(bitstream);

    std::vector<uchar> diffs;
    Tunstall tunstall;
    tunstall.decompress(stream, diffs);

    zpoints.resize(node.nvert);
    bitstream.read(coord_bits*3, zpoints[0].bits);
    for(int i = 1; i < zpoints.size(); i++) {
        ZPoint &p = zpoints[i];
        p = zpoints[i-1];
        uchar d = diffs[i-1];
        p.setBit(d, 1);
        uint64_t e = 0;
        bitstream.read(d, e);
        p.bits &= ~((1LL<<d) -1);
        p.bits |= e;
    }

    float step = pow(2.0f, (float)coord_q);
    vcg::Point3f *coords = data.coords();
    for(int i = 0; i < zpoints.size(); i++)
        coords[i] = zpoints[i].toPoint(min, step);
#endif
}

Point3f norm(const Point3f &a, const Point3f &b, const Point3f &c) {
    return (b - a)^(c - a);
}

//compact in place faces in data, update patches information, compute topology and encode each patch.
void MeshCoder::encodeFaces() {

    if(!node.nface) return;
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
#ifdef PARALLELOGRAM
			McFace f(face[0], face[1], face[2]);
#else
            McFace f(order[face[0]], order[face[1]], order[face[2]]);
#endif
            //keep degenerate faces (otherwise when compressin we would need 2 passes for the index.
            if(f.f[0] == f.f[1] || f.f[0] == f.f[2] || f.f[1] == f.f[2])
                continue;

            faces.push_back(f);
            count++;
        }
        patch.triangle_offset = count;
        start = end;


        Connectivity c;
#ifdef PARALLELOGRAM
		c.encode(stream, faces, zpoints, order); //TEST
#else
		c.encode(stream, faces); //TEST
#endif
    }
    nface = count;
}

void MeshCoder::decodeFaces() {
    if(!node.nface) return;
    uint16_t *faces = data.faces(sig, node.nvert);
    int start = 0;
    for(int p = node.first_patch; p < node.last_patch(); p++) {
        Patch &patch = patches[p];
        uint end = patch.triangle_offset;
        Connectivity c;
#ifdef PARALLELOGRAM
		c.decode(stream, end - start, faces + start*3, );
#else
		c.decode(stream, end - start, faces + start*3);
#endif
        start = end;
    }
}

void MeshCoder::encodeNormals() {

    int side = 1<<(16 - norm_q);

    //these are the original normals, reordered
    Point3s *norms = data.normals(sig, node.nvert);
#ifdef PARALLELOGRAM
	Point3s *original_normals = norms;
#else
    vector<Point3s> original_normals(zpoints.size()); //original normals
	for(int i = 0; i < node.nvert; i++) {
		original_normals[order[i]] = norms[i];
	}
#endif

    std::vector<uchar> diffs;
    std::vector<uchar> signs;
    BitStream bitstream(node.nvert/64);

    //we have faces: estimate normals and save difference from originals on boundary
    if(sig.face.hasIndex()) {
        //1 recover quantized vertices
        vector<vcg::Point3f> points(zpoints.size());
        //recover quantized positions
        float step = pow(2.0f, (float)coord_q);
#ifdef PARALLELOGRAM
        for(unsigned int i = 0; i < zpoints.size(); i++)
			for(int k =0; k < 3; k++)
				points[i][k] = zpoints[i][k];
#else
		for(unsigned int i = 0; i < zpoints.size(); i++)
			points[i] = zpoints[i].toPoint(min, step);
#endif

        //get faces with reordered indices
		uint16_t *original_faces = data.faces(sig, node.nvert);
#ifndef PARALLELOGRAM
        vector<uint16_t> faces;
        for(int i = 0; i < node.nface; i++) { ///aaarrgghhhhh
            uint16_t *face = original_faces + i*3;
            uint16_t f[3];
            for(int k = 0; k < 3; k++)
                f[k] = order[face[k]];
            if(f[0] == f[1] || f[0] == f[2] || f[1] == f[2])
                continue;
            for(int k = 0; k < 3; k++)
                faces.push_back(f[k]);
        }
#endif


		vector<Point3s> normals(points.size());
#ifdef PARALLELOGRAM

		markBoundary(original_faces, node.nface, points.size());
		computeNormals(&*points.begin(), points.size(), original_faces, node.nface, &*normals.begin());
#else
		markBoundary(&*faces.begin(), faces.size()/3, points.size());
		computeNormals(&*points.begin(), points.size(), &*faces.begin(), faces.size()/3, &*normals.begin());
#endif

        for(unsigned int i = 0; i < zpoints.size(); i++) {
            if(!boundary[i]) continue;
            Point3s &computed = normals[i];
            Point3s &original = original_normals[i];
            for(int comp = 0; comp < 2; comp++) {
                int d = (int)(original[comp]/side - computed[comp]/side); //act1ual value - predicted
                encodeIndex(diffs, bitstream, d);
            }
            bool signbit = (computed[2]*original[2] < 0);
            signs.push_back(signbit);
        }
    } else {
        int max_value = (1<<(norm_q));
        int step = (1<<(16 - norm_q));

        //point cloud: compute differences from previous normal
        for(int k = 0; k < 2; k++) {
            int on = 0; //old color
			for(unsigned int i = 0; i < zpoints.size(); i++) {
                Point3s c = original_normals[i];
                int n = c[k];

                n = (n/(float)step + 0.5);
                if(n >= max_value) n = max_value-1;
                if(n <= -max_value) n = -max_value+1;
                int d = n - on;
                encodeIndex(diffs, bitstream, d);
                on = n;
            }
        }
		for(unsigned int i = 0; i < zpoints.size(); i++) {
            Point3s c = original_normals[i];
            bool signbit = c[2] > 0;
            signs.push_back(signbit);
        }
    }

    stream.write<char>(norm_q);
    Tunstall tunstall;
    tunstall.compress(stream, &*diffs.begin(), diffs.size());
    Tunstall tunstall1;
    tunstall1.compress(stream, &*signs.begin(), signs.size());
    bitstream.flush();
    stream.write(bitstream);
}

void MeshCoder::decodeNormals() {

    uint16_t *faces = data.faces(sig, node.nvert);
    Point3f *coords = data.coords();
    Point3s *norms = data.normals(sig, node.nvert);

    norm_q = stream.read<char>();

    std::vector<uchar> diffs;
    Tunstall tunstall;
	tunstall.decompress(stream, diffs);

    std::vector<uchar> signs;
    Tunstall tunstall1;
	tunstall1.decompress(stream, signs);

    BitStream bitstream;
    stream.read(bitstream);


    int side = 1<<(16 - norm_q);

    if(sig.face.hasIndex()) {
        markBoundary(faces, node.nface, node.nvert);
        computeNormals(coords, node.nvert, faces, node.nface, norms);

        int diffcount = 0;
        int signcount = 0;
        //get difference between original and predicted
        for(unsigned int i = 0; i < node.nvert; i++) {
            vcg::Point3s &N = norms[i];
            if(!boundary[i]) continue;
            for(int comp = 0; comp < 2; comp++) {
                int n = N[comp]/side;
                int diff = decodeIndex(diffs[diffcount++], bitstream);
                N[comp] = (n + diff)*side;
            }
            float x = N[0];
            float y = N[1];
            float z = 32767.0f*32767.0f - x*x - y*y;

            if(z < 0) z = 0;
            z = sqrt(z);
            //sign
            if(z > 32767.0f) z = 32767.0f;
            bool signbit = signs[signcount++];
            if(N[2] < 0 != signbit)
                z = -z;
            N[2] = (int16_t)z;
        }

    } else {
        int step = (1<<(16 - norm_q));

        int count = 0;
        for(int k = 0; k < 2; k++) {

            int on = 0; //old normal;
            for(unsigned int i = 0; i < node.nvert; i++) {
                Point3s &c = norms[i];
                int16_t &n = c[k];

                int d = decodeIndex(diffs[count++], bitstream);
                n = on + d;
                on = n;
                n *= step;
            }
        }
        for(unsigned int i = 0; i < node.nvert; i++) {
            Point3s &c = norms[i];
            c[2] = (uint16_t)sqrt(32767.0 * 32767.0 - c[0]*c[0] - c[1]*c[1]);
            if(!signs[i])
                c[2] = -c[2];
        }
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

void MeshCoder::encodeColors() {
    Color4b *colors = data.colors(sig, node.nvert);
    vector<vcg::Color4b> values(zpoints.size());
    for(unsigned int i = 0; i < order.size(); i++)
        values[order[i]] = toYCC(colors[i]);

    for(int k = 0; k < 4; k++)
        stream.write<char>(color_q[k]);

    BitStream bitstream(node.nvert/2);
    for(int k = 0; k < 4; k++) {
        int max_value = (1<<(color_q[k]));
        int step = (1<<(8 - color_q[k]));
        std::vector<uchar> diffs;

        int on = 0; //old color
        for(unsigned int i = 0; i < values.size(); i++) {
            Color4b c = values[i];
            int n = c[k];

            n = (n/(float)step + 0.5);
            if(n >= max_value) n = max_value-1;
            if(n < 0) n = 0;
            int d = n - on;
            encodeIndex(diffs, bitstream, d);
            on = n;
        }
        Tunstall tunstall;
        tunstall.compress(stream, &*diffs.begin(), diffs.size());
    }
    bitstream.flush();
    stream.write(bitstream);
}

void MeshCoder::decodeColors() {

    Color4b *colors = data.colors(sig, node.nvert);

    for(int k = 0; k < 4; k++)
        color_q[k] = stream.read<char>();

    std::vector<uchar> diffs[4];
    for(int k = 0; k < 4; k++) {
        Tunstall tunstall;
        tunstall.decompress(stream, diffs[k]);
    }
    BitStream bitstream;
    stream.read(bitstream);

    for(int k = 0; k < 4; k++) {
        int shift = 8 - color_q[k];

        int count = 0;
        int on = 0; //old normal;
        vector<uchar> &cdiffs = diffs[k];

        for(unsigned int i = 0; i < node.nvert; i++) {
            Color4b &c = colors[i];
            uchar &n = c[k];

            int d = decodeIndex(cdiffs[count++], bitstream);
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

//how to determine if a vertex is a boundary without topology:
//for each edge a vertex is in, add or subtract the id of the other vertex depending on order
//for internal vertices sum is zero.
//unless we have strange configurations and a lot of sfiga, zero wont happen. //TODO think about this
void MeshCoder::markBoundary(uint16_t *faces, int nfaces, int nvert) {
    if(!sig.face.hasIndex()) {
        boundary.resize(nvert, true);
        return;
    }
	boundary.resize(nvert, false);

    std::vector<int> count(nvert, 0);
    for(int i = 0; i < nfaces; i++) {
        uint16_t *f = faces + i*3;
        count[f[0]] += (int)f[1] - (int)f[2];
        count[f[1]] += (int)f[2] - (int)f[0];
        count[f[2]] += (int)f[0] - (int)f[1];
    }
    for(int i = 0; i < nvert; i++)
        if(count[i] != 0)
            boundary[i] = true;
}

void MeshCoder::computeNormals(vcg::Point3f *coords, int nvert, uint16_t *faces, int nfaces, vcg::Point3s *n) {
    vector<Point3f> normals(nvert, Point3f(0, 0, 0));
    for(int i = 0; i < nfaces; i++) {
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
    for(unsigned int i = 0; i < normals.size(); i++) {
        Point3f &normal = normals[i];
        normal.Normalize();
        for(int k = 0; k < 3; k++)
            n[i][k] = normal[k]*32767;
    }
}
//val cam be zero.
void MeshCoder::encodeIndex(std::vector<uchar> &diffs, BitStream &stream, int val) {
    val = Tunstall::toUint(val)+1;
    int ret = Math::log2(val);
    diffs.push_back(ret);
    if(ret > 0)
        stream.write(val, ret);
}

int MeshCoder::decodeIndex(uchar diff, BitStream &stream) {
    int ret = diff;
    int val;
    if(ret == 0) {
        val = 1;
    } else {
        uint64_t c = 1<<(ret);
        stream.read(ret, c);
        val = (int)c;
    }
    return Tunstall::toInt(val-1);
}
