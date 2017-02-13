#ifndef CONNECTIVITY_H
#define CONNECTIVITY_H
/* same as connectivity_coder but using tunstall and CStream */

#include <vector>
#include <deque>
#include <algorithm>

#include "math_class.h"
#include "cstream.h"
#include "tunstall.h"
#include "zpoint.h"
#include <assert.h>

class McFace {
public:
    uint16_t f[3];
    uint16_t t[3]; //topology: opposite face
    uint16_t i[3]; //index in the opposite face of this face: faces[f.t[k]].t[f.i[k]] = f;
    McFace(uint16_t v0 = 0, uint16_t v1 = 0, uint16_t v2 = 0) {
        f[0] = v0; f[1] = v1; f[2] = v2;
        t[0] = t[1] = t[2] = 0xffff;
    }
    bool operator<(const McFace &face) const {
        if(f[0] < face.f[0]) return true;
        if(f[0] > face.f[0]) return false;
        if(f[1] < face.f[1]) return true;
        if(f[1] > face.f[1]) return false;
        return f[2] < face.f[2];
    }
    bool operator>(const McFace &face) const {
        if(f[0] > face.f[0]) return true;
        if(f[0] < face.f[0]) return false;
        if(f[1] > face.f[1]) return true;
        if(f[1] < face.f[1]) return false;
        return f[2] > face.f[2];
    }
};

class CEdge { //compression edges
public:
    uint16_t face;
	uint16_t side; //opposited to side vertex of face
    uint32_t prev, next;
    bool deleted;
    CEdge(uint16_t f = 0, uint16_t s = 0, uint32_t p = 0, uint32_t n = 0):
        face(f), side(s), prev(p), next(n), deleted(false) {}
};

class DEdge { //decompression edges
public:
    uint16_t v0;
	uint16_t v1; //ci potrei mettere v2
    uint32_t prev, next;
    bool deleted;
    DEdge(uint16_t a = 0, uint16_t b = 0, uint32_t p = 0, uint32_t n = 0):
        v0(a), v1(b), prev(p), next(n), deleted(false) {}
};

class McEdge { //topology
public:
    uint16_t face, side;
    uint16_t v0, v1;
    bool inverted;
    //McEdge(): inverted(false) {}
    McEdge(uint16_t _face, uint16_t _side, uint16_t _v0, uint16_t _v1): face(_face), side(_side), inverted(false) {

        if(_v0 < _v1) {
            v0 = _v0; v1 = _v1;
            inverted = false;
        } else {
            v1 = _v0; v0 = _v1;
            inverted = true;
        }
    }
    bool operator<(const McEdge &edge) const {
        if(v0 < edge.v0) return true;
        if(v0 > edge.v0) return false;
        return v1 < edge.v1;
    }
    bool match(const McEdge &edge) {
        if(inverted && edge.inverted) return false;
        if(!inverted && !edge.inverted) return false;
        //if(!(inverted ^ edge.inverted)) return false;
        return v0 == edge.v0 && v1 == edge.v1;
    }
};


class Connectivity {
public:

    enum Clers { VERTEX = 0, LEFT = 1, RIGHT = 2, END = 3, BOUNDARY = 4 };

    std::vector<uchar> clers;
    std::vector<uchar> diffs;
    BitStream bitstream;

    Connectivity() {}

	static int log2(uint64_t p) {
		int k = 0;
		while ( p>>=1 ) { ++k; }
		return k;
	}
    //returns nuber of bits.written

	//for each ENCODED vertex its original vertex
#ifdef PARALLELOGRAM
	int encode(CStream &stream, std::vector<McFace> &faces, std::vector<vcg::Point3i> &points, std::vector<int> &order) {
#else
	int encode(CStream &stream, std::vector<McFace> &faces) {
#endif
        buildTopology(faces);

        bitstream.reserve(2*faces.size());

        //clers.initAlphabet(5);
        //model.initAlphabet(18); //up until 1<<17k vertices

        unsigned int current = 0;          //keep track of connected component start

        std::vector<CEdge> front; // for fast mode we need to pop_front
        unsigned int current_edge = 0;     //keep track of position of edge in front

        std::vector<bool> visited(faces.size(), false);
        unsigned int totfaces = faces.size();

        while(totfaces > 0) {

            if(current_edge == front.size()) {   //empty front

                while(current != faces.size()) {   //find first triangle non visited
                    if(!visited[current]) break;
                    current++;
                }
                if(current == faces.size()) break; //no more faces to encode exiting

                //encode first face: 3 vertices indexes, and add edges
                McFace &face = faces[current];
#ifdef PARALLELOGRAM
				vcg::Point3i last(0, 0, 0);
#else
                int last = -1;
#endif

                for(int k = 0; k < 3; k++) {
#ifdef PARALLELOGRAM
					int index = face.f[k];
					order.push_back(index);
					vcg::Point3i &p = points[index];
					encodeVertex(p, last);
					last = p;
#else
                    encodeIndex(diffs, bitstream, face.f[k], last, last);
					last = face.f[k];
#endif
                    front.push_back(CEdge(current, k, current_edge + prev_(k), current_edge + next_(k)));
                }

                visited[current] = true;
                current++;
                totfaces--;
                continue;
            }

			CEdge &e = front[current_edge++];
            if(e.deleted) continue;
            e.deleted = true;

            //opposite face is the triangle we are encoding
            uint16_t opposite_face = faces[e.face].t[e.side];
            int opposite_side = faces[e.face].i[e.side];

            if(opposite_face == 0xffff || visited[opposite_face]) { //boundary edge or glue
                clers.push_back(BOUNDARY);
                //cout << (clers.size()-1) << " CLER: " << (int)clers.back() << endl;
                continue;
            }

            McFace &face = faces[opposite_face];

            int k2 = opposite_side;
            int k0 = next_(k2);
            int k1 = next_(k0);

            //check for closure on previous or next edge
            int eprev = e.prev;
            int enext = e.next;
            CEdge &previous_edge = front[eprev];
            CEdge &next_edge = front[enext];

            int first_edge = front.size();
            bool close_left = (faces[previous_edge.face].t[previous_edge.side] == opposite_face);
            bool close_right = (faces[next_edge.face].t[next_edge.side] == opposite_face);

            if(close_left && close_right) {
                clers.push_back(END);
                previous_edge.deleted = true;
                next_edge.deleted = true;
                front[previous_edge.prev].next = next_edge.next;
                front[next_edge.next].prev = previous_edge.prev;

            } else if(close_left) {
                clers.push_back(LEFT);
                previous_edge.deleted = true;
                front[previous_edge.prev].next = first_edge;
                front[enext].prev = first_edge;
                front.push_back(CEdge(opposite_face, k1, previous_edge.prev, enext));

            } else if(close_right) {
                clers.push_back(RIGHT);
                next_edge.deleted = true;
                front[next_edge.next].prev = first_edge;
                front[eprev].next = first_edge;
                front.push_back(CEdge(opposite_face, k0, eprev, next_edge.next));

            } else {
                clers.push_back(VERTEX);
                int v0 = face.f[k0];
                int v1 = face.f[k1];
                int opposite = face.f[k2];

                assert(v0 != v1);

#ifdef PARALLELOGRAM
				//compute how much would it take to save vertex information:
				//we need to estimate opposite direction using v0 + v1 -
				int v2 = faces[e.face].f[e.side];

				vcg::Point3i p0 = points[v0];
				vcg::Point3i p1 = points[v1];
				vcg::Point3i p2 = points[v2];

				vcg::Point3i predicted = p0 + p1 - p2;
				order.push_back(opposite);
				encodeVertex(points[opposite], predicted);
#else
				encodeIndex(diffs, bitstream, opposite, v0, v1);
#endif



                previous_edge.next = first_edge;
                next_edge.prev = first_edge + 1;
                front.push_back(CEdge(opposite_face, k0, eprev, first_edge+1));
                front.push_back(CEdge(opposite_face, k1, first_edge, enext));
            }
            //cout << (clers.size()-1) << " CLER: " << (int)clers.back() << endl;
            assert(!visited[opposite_face]);
            visited[opposite_face] = true;
            totfaces--;
        }
        Tunstall tunstall;
        tunstall.compress(stream, &*clers.begin(), clers.size());

        Tunstall tunstall1;
        tunstall1.compress(stream, &*diffs.begin(), diffs.size());
        bitstream.flush();

        stream.write(bitstream);
        return 0;
    }
    static void buildTopology(std::vector<McFace> &faces) {
        //create topology;
        std::vector<McEdge> edges;
        for(unsigned int i = 0; i < faces.size(); i++) {
            McFace &face = faces[i];
            for(int k = 0; k < 3; k++) {
                int kk = k+1;
                if(kk == 3) kk = 0;
                int kkk = kk+1;
                if(kkk == 3) kkk = 0;
                edges.push_back(McEdge(i, k, face.f[kk], face.f[kkk]));
            }
        }
        std::sort(edges.begin(), edges.end());

        McEdge previous(0xffff, 0xffff, 0xffff, 0xffff);
        for(unsigned int i = 0; i < edges.size(); i++) {
            McEdge &edge = edges[i];
            if(edge.match(previous)) {
                uint16_t &edge_side_face = faces[edge.face].t[edge.side];
                uint16_t &previous_side_face = faces[previous.face].t[previous.side];
                if(edge_side_face == 0xffff && previous_side_face == 0xffff) {
                    edge_side_face = previous.face;
                    faces[edge.face].i[edge.side] = previous.side;
                    previous_side_face = edge.face;
                    faces[previous.face].i[previous.side] = edge.side;
                }
            } else
                previous = edge;
        }
    }

#ifdef PARALLELOGRAM
	void decode(CStream &stream, int nface, uint16_t *faces, vcg::Point3i *vertices) {
#else
	void decode(CStream &stream, int nface, uint16_t *faces) {
#endif
		//std::vector<uchar> clers;
		//std::vector<uchar> diffs;
		//BitStream bitstream;

        Tunstall tunstall;
        tunstall.decompress(stream, clers);
        Tunstall tunstall1;
        tunstall1.decompress(stream, diffs);
        stream.read(bitstream);

        unsigned int current = 0;          //keep track of connected component start

        std::vector<DEdge> front; // for fast mode we need to pop_front
        unsigned int current_edge = 0;

        int count = 0;            //keep track of decoded triangles
		int vertex_count = 0;
        unsigned int totfaces = nface;

        int diff_count = 0;
        int cler_count = 0;

        while(totfaces > 0) {
            //testFront(front);
            if(current_edge == front.size()) {

                if(current == nface) break; //no more faces to encode exiting

#ifdef PARALLELOGRAM
				vcg::Point3i last(0, 0, 0);
#else
                int last = -1;
#endif
                int index[3];
                for(int k = 0; k < 3; k++) {
#ifdef PARALLELOGRAM

					decodeVertex(vertices[vertex_count], last, diffs[diff_count++]);
					index[k] = vertex_count++;
#else
                    index[k] = decodeIndex(diffs[diff_count++], bitstream, last, last);
                    last = index[k];
#endif
                    faces[count++] = index[k];

                }
                for(int k = 0; k < 3; k++)
                    front.push_back(DEdge(index[next_(k)], index[prev_(k)], current_edge + prev_(k), current_edge + next_(k)));

                current++;
                totfaces--;
                continue;
            }

            DEdge &e = front[current_edge++];
            if(e.deleted) continue;
            e.deleted = true;
            int prev = e.prev;
            int next = e.next;
            int v0 = e.v0;
            int v1 = e.v1;

            DEdge &previous_edge = front[prev];
            DEdge &next_edge = front[next];

            int c = clers[cler_count++];

            if(c == BOUNDARY) continue;


            int first_edge = front.size();
            int opposite = -1;
            if(c == END) {
                previous_edge.deleted = true;
                next_edge.deleted = true;
                front[previous_edge.prev].next = next_edge.next;
                front[next_edge.next].prev = previous_edge.prev;
                opposite = previous_edge.v0;

            } else if(c == LEFT) {
                previous_edge.deleted = true;
                front[previous_edge.prev].next = first_edge;
                front[next].prev = first_edge;
                opposite = previous_edge.v0;
                front.push_back(DEdge(opposite, v1, previous_edge.prev, next));

            } else if(c == RIGHT) {
                next_edge.deleted = true;
                front[next_edge.next].prev = first_edge;
                front[prev].next = first_edge;
                opposite = next_edge.v1;
                front.push_back(DEdge(v0, opposite, prev, next_edge.next));

            } else {
                assert(c == VERTEX);
                opposite = decodeIndex(diffs[diff_count++], bitstream, e.v1, e.v0);
                previous_edge.next = first_edge;
                next_edge.prev = first_edge + 1;
                front.push_back(DEdge(e.v0, opposite, prev, first_edge + 1));
                front.push_back(DEdge(opposite, v1, first_edge, next));
            }
            //if(v1 < 0 || v0 < 0 || opposite < 0)
            //    cout << "OOPPS" << endl;
            //assert(opposite < nvert);
            faces[count++] = v1;
            faces[count++] = v0;
            faces[count++] = opposite;
            totfaces--;
        }
        //model->dump();
        //clers.dump();
    }
    vector<int> counting;
    int totcounting;
    void addEntropy(int val) {
        if(counting.size() == 0)
            totcounting = 0;
        if(val >= counting.size()) {
            counting.resize(val+1, 0);
        }
        counting[val]++;
        totcounting++;
    }
    void entropy() {
        double e = 0.0;
        for(int i = 0; i < counting.size(); i++) {
            int n = counting[i];
            if(n == 0) continue;
            double p = n/(double)totcounting;
            e -= p*log2(p);
        }
        cout << "Entropy: " << e << endl;
    }

	void encodeVertex(const vcg::Point3i &target, const vcg::Point3i &predicted) {
		//compute how much would it take to save vertex information:
		//we need to estimate opposite direction using v0 + v1 -
		vcg::Point3i d = target - predicted; //this is the estimated point.

		for(int k = 0; k < 3; k++)
			d[k] = Tunstall::toUint(d[k]);
		int diff = std::max(log2(d[0]), std::max(log2(d[1]), log2(d[2])));

		diffs.push_back(diff);
		bitstream.write(d[0], diff);
		bitstream.write(d[1], diff);
		bitstream.write(d[2], diff);
	}

	void decodeVertex(vcg::Point3i &target, const vcg::Point3i &predicted, int diff) {
		target = predicted;
		if(diff == 0) return;
		for(int k = 0; k < 3; k++) {
			uint64_t bits = 0;
			bitstream.read(diff, bits);
			bits = Tunstall::toInt(bits);
			target[k] += diff;
		}
	}

    void encodeIndex(std::vector<uchar> &diffs, BitStream &stream, int val, int v0, int v1) {

        int last = v0;
        if(v1 < v0) last = v1;
        val = val - last;
        assert(val != 0);
        val = Tunstall::toUint(val);

        addEntropy(val);

        int ret = Math::log2(val);
        diffs.push_back(ret);
        if(ret > 0)
            bitstream.write(val, ret);
    }

    int decodeIndex(uchar diff, BitStream &stream, int v0, int v1) {
        int ret = diff;
        int val;
        if(ret == 0) {
            val = 1;
        } else {
            uint64_t c = 1<<(ret);
            stream.read(ret, c);
            val = (int)c;
        }

        int last = v0;
        if(v1 < v0) last = v1;
        val = Tunstall::toInt(val);
        val = val + last;
        return val;
    }

    static int next_(int t) {
        t++;
        if(t == 3) t = 0;
        return t;
    }
    static int prev_(int t) {
        t--;
        if(t == -1) t = 2;
        return t;
    }

    template<class EDGE> void testFront(std::vector<EDGE> &front) {
        if(front.size() > 40) return;

        for(unsigned int i = 0; i < front.size(); i++) {
            EDGE &edge = front[i];

            if(edge.deleted) continue;

            EDGE &next = front[edge.next];
            EDGE &prev = front[edge.prev];
            assert(next.prev == i);
            assert(prev.next == i);
        }
    }
};

#endif // CONNECTIVITY_H

