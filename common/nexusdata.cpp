#define _FILE_OFFSET_BITS 64

#include <QTime>
#include "nexusdata.h"

#include <vcg/space/line3.h>
#include <vcg/space/intersection3.h>

#ifdef USE_OPENCTM
#include <openctm.h>
#endif

#include "../nxszip/meshdecoder2.h"

#if _MSC_VER >= 1800
#include <random>
#endif

#include <iostream>
using namespace std;
using namespace vcg;
using namespace nx;

uint16_t *NodeData::faces(Signature &sig, uint32_t nvert, char *mem) {
	int a = sig.vertex.size();
	return (uint16_t  *)(mem + nvert*sig.vertex.size());
}


NexusData::NexusData(): nodes(0), patches(0), textures(0), nodedata(0), texturedata(0) {
}

NexusData::~NexusData() {
	close();
}

bool NexusData::open(const char *_uri) {

	file.setFileName(_uri);
	if(!file.open(QIODevice::ReadOnly))
		//file = fopen(_uri, "rb+");
		//if(!file)
		return false;
	loadHeader();
	loadIndex();
	return true;
}

void NexusData::close() {
	flush();
}

void NexusData::flush() {
	//flush
	for(uint i = 0; i < header.n_nodes; i++)
		delete nodedata[i].memory;

	delete []nodes;
	delete []patches;
	delete []textures;
	delete []nodedata;
	delete []texturedata;
}

void NexusData::loadHeader() {
	//fread(&header, sizeof(Header), 1, file);
	int readed = file.read((char *)&header, sizeof(Header));
	if(readed != sizeof(Header))
		throw "Could not read header, file too short";
	if(header.magic != 0x4E787320)
		throw "Could not read header.. probably not a nxs file";
}

void NexusData::loadHeader(char *buffer) {
	header = *(Header *)buffer;
	if(header.magic != 0x4E787320)
		throw "Could not read header.. probably not a nxs file";
}

uint64_t NexusData::indexSize() {
	return header.n_nodes * sizeof(Node) +
			header.n_patches * sizeof(Patch) +
			header.n_textures * sizeof(Texture);
}

void NexusData::initIndex() {
	nodes = new Node[header.n_nodes];
	patches = new Patch[header.n_patches];
	textures = new Texture[header.n_textures];

	nodedata = new NodeData[header.n_nodes];
	texturedata = new TextureData[header.n_textures];
}

void NexusData::loadIndex() {
	initIndex();

	//fread(nodes, sizeof(Node), header.n_nodes, file);
	//fread(patches, sizeof(Patch), header.n_patches, file);
	//fread(textures, sizeof(Texture), header.n_textures, file);
	file.read((char *)nodes, sizeof(Node)*header.n_nodes);
	file.read((char *)patches, sizeof(Patch)*header.n_patches);
	file.read((char *)textures, sizeof(Texture)*header.n_textures);
}

void NexusData::loadIndex(char *buffer) {
	initIndex();

	uint32_t size = sizeof(Node)*header.n_nodes;
	memcpy(nodes, buffer, size);
	buffer += size;

	size = sizeof(Patch)*header.n_patches;
	memcpy(patches, buffer, size);
	buffer += size;

	size = sizeof(Texture)*header.n_textures;
	memcpy(textures, buffer, size);

}

uint32_t NexusData::size(uint32_t node) {
	return nodes[node].getSize();
}

#ifdef USE_OPENCTM

uint ctmreader(void *buf, CTMuint count, void *data) {
	QFile *file = (QFile *)data;
	return file->read((char *)buf, count);
	//return fread(buf, 1, count, file);
}
#endif


uint64_t NexusData::loadRam(uint32_t n) {
	Node &node = nodes[n];
	quint64 offset = node.getBeginOffset();

	NodeData &d = nodedata[n];
	quint64 compressed_size = node.getEndOffset() - offset;

	quint64 size = -1;

	if(header.signature.flags & Signature::MECO) {

		QTime time;
		time.start();

		char *buffer = new char[compressed_size];
		file.seek(offset);
		int r = file.read(buffer, compressed_size);

		assert(r == compressed_size);
		size = node.nvert * header.signature.vertex.size() +
				node.nface * header.signature.face.size();
		d.memory = new char[size];
		//memort data is
		for(int i = 0; i < 1; i++) {
			MeshDecoder coder(node, d, patches, header.signature);
			coder.decode(compressed_size, (unsigned char *)buffer);

			if(0 && !header.signature.face.hasIndex()) {
				auto &sig = header.signature;
				std::vector<int> order(node.nvert);
				for(int i = 0; i < node.nvert; i++)
					order[i] = i;

				unsigned seed = 0;
				std::shuffle (order.begin(), order.end(), std::default_random_engine(seed));
				std::vector<vcg::Point3f> coords(node.nvert);
				for(int i =0; i < node.nvert; i++)
					coords[i] = d.coords()[order[i]];
				memcpy(d.coords(), &*coords.begin(), sizeof(Point3f)*node.nvert);

				if(sig.vertex.hasNormals()) {
					Point3s *n = d.normals(sig, node.nvert);
					std::vector<Point3s> normals(node.nvert);
					for(int i =0; i < node.nvert; i++)
						normals[i] = n[order[i]];
					memcpy(n, &*normals.begin(), sizeof(Point3s)*node.nvert);
				}

				if(sig.vertex.hasColors()) {
					Color4b *c = d.colors(sig, node.nvert);
					std::vector<Color4b> colors(node.nvert);
					for(int i =0; i < node.nvert; i++)
						colors[i] = c[order[i]];
					memcpy(c, &*colors.begin(), sizeof(Color4b)*node.nvert);
				}
			}
		}
		double elapsed = time.elapsed();
		//cout << "Z Elapsed: " << elapsed << " M/s " << 1000*(node.nface/elapsed)/(1<<20) << " Faces: " << node.nface << endl;


	} else if(header.signature.flags & Signature::CTM1 || header.signature.flags & Signature::CTM2) {
#ifdef USE_OPENCTM

		QTime time;
		time.start();
		quint64 size = node.nvert * header.signature.vertex.size() +
				node.nface * header.signature.face.size();
		d.memory = new char[size];

		CTMuint    vertCount, triCount;
		const CTMuint   *indices;
		const CTMfloat  *vertices;
		const CTMfloat  *normalf;
		// Create a new context
		CTMcontext context = ctmNewContext(CTM_IMPORT);

		file.seek(offset);
		ctmLoadCustom(context, ctmreader, (void *)&file);
		if(ctmGetError(context) != CTM_NONE)
			return -1;

		// Access the mesh data
		vertCount = ctmGetInteger(context, CTM_VERTEX_COUNT);
		triCount = ctmGetInteger(context, CTM_TRIANGLE_COUNT);
		vertices = ctmGetFloatArray(context, CTM_VERTICES);
		indices = ctmGetIntegerArray(context, CTM_INDICES);
		normalf = ctmGetFloatArray(context, CTM_NORMALS);
		assert(vertCount == node.nvert);
		assert(triCount == node.nface);

		memcpy(d.coords(), vertices, node.nvert * sizeof(vcg::Point3f));
		vcg::Point3s *normals = d.normals(header.signature, node.nvert);
		for(uint i = 0; i < node.nvert; i++) {
			for(int k = 0; k < 3; k++)
				normals[i][k] = (short)(normalf[i*3 + k]*32766);
		}
		uint16_t *faces = d.faces(header.signature, node.nvert);
		for(uint i = 0; i < node.nface*3; i++) {
			faces[i] = indices[i];
			assert(faces[i] < node.nvert);
		}

		// Free the context
		ctmFreeContext(context);

		double elapsed = time.elapsed();
		cout << "CTM Elapsed: " << elapsed << " M/s " << 1000*(triCount/elapsed)/(1<<20) << endl;
		return size;
#else
		cout << "CTM compression library not enabled." << endl;
		exit(0);
		return -1;
#endif
	} else { //not compressed
		size = node.getSize();
		d.memory = (char *)file.map(offset, size);
		vcg::Point2f *tex = (vcg::Point2f *)(d.memory + node.nvert*12);
	}

	assert(d.memory);
	if(header.n_textures) {
		//be sure to load images
		for(uint32_t p = node.first_patch; p < node.last_patch(); p++) {
			Patch &patch = patches[p];
			uint32_t t = patch.texture;
			if(t == 0xffffffff) continue;
			TextureData &data = texturedata[t];
			data.count_ram++;
			if(texturedata[t].memory) continue;
			assert(data.count_ram == 1);
//			texturedata[t].memory = "a";
			Texture &texture = textures[t];
			quint64 o = texture.getBeginOffset();
			quint64 s = texture.getSize();
//TEMPORARILY LOADING DATA FROM FILES
			texturedata[t].memory = (char *)file.map(texture.getBeginOffset(), texture.getSize());
			//assert(texture.getSize());
			//assert(texturedata[t].memory);
			//size += texture.getSize();
		}
	}
	return size;
}

uint64_t NexusData::dropRam(uint32_t n, bool write) {

	Node &node = nodes[n];
	NodeData &data = nodedata[n];

	assert(!write);
	assert(!data.vbo);
	assert(data.memory);

	if(!(header.signature.flags & Signature::MECO)) //not compressed.
		file.unmap((uchar *)data.memory);
	else
		delete []data.memory;

	data.memory = NULL;

	//report RAM size for compressed meshes.
	uint32_t size = 0;
	if(header.signature.flags & Signature::MECO)
		size = node.nvert * header.signature.vertex.size() +
				node.nface * header.signature.face.size();
	else
		size = node.getSize();

	if(header.n_textures) {
		for(uint32_t p = node.first_patch; p < node.last_patch(); p++) {
			Patch &patch = patches[p];
			uint32_t t = patch.texture;
			if(t == 0xffffffff) continue;
			TextureData &tdata = texturedata[t];
			tdata.count_ram--;
			if(tdata.count_ram != 0) continue;
			file.unmap((uchar *)tdata.memory);
			tdata.memory = NULL;
			//Texture &texture = textures[t];
			//size += tdata.getSize(); //careful with cache... might create problems to return different sizes in get drop and size
		}
	}
	return size;
}


//local structure for keeping track of nodes and their distance
class DNode {
public:
	uint32_t node;
	float dist;
	DNode(uint32_t n = 0, float d = 0.0f): node(n), dist(d) {}
	bool operator<(const DNode &n) const {
		return dist < n.dist;
	}
};

bool closest(vcg::Sphere3f &sphere, vcg::Ray3f &ray, float &distance) {
	vcg::Point3f dir = ray.Direction();
	vcg::Line3f line(ray.Origin(), dir.Normalize());

	vcg::Point3f p, q;
	bool success = vcg::IntersectionLineSphere(sphere, line, p, q);
	if(!success) return false;
	p -= ray.Origin();
	q -= ray.Origin();
	float a = p*ray.Direction();
	float b = q*ray.Direction();
	if(a > b) a = b;
	if(b < 0) return false; //behind observer
	if(a < 0) {   //we are inside
		distance = 0;
		return true;
	}
	distance = a;
	return true;
}

bool NexusData::intersects(vcg::Ray3f &ray, float &distance) {
	uint32_t sink = header.n_nodes -1;

	vcg::Point3f dir = ray.Direction();
	ray.SetDirection(dir.Normalize());

	if(!closest(header.sphere, ray, distance))
		return false;

	distance = 1e20f;

	//keep track of visited nodes (it is a DAG not a tree!
	std::vector<bool> visited(header.n_nodes, false);
	std::vector<DNode> candidates;      //heap of nodes
	candidates.push_back(DNode(0, distance));

	bool hit = false;

	while(candidates.size()) {
		pop_heap(candidates.begin(), candidates.end());
		DNode candidate = candidates.back();
		candidates.pop_back();

		if(candidate.dist > distance) break;

		Node &node = nodes[candidate.node];

		if(node.first_patch  == sink) {
			if(candidate.dist < distance) {
				distance = candidate.dist;
				hit = true;
			}
			continue;
		}

		//not a leaf, insert children (if they intersect
		for(uint32_t i = node.first_patch; i < node.last_patch(); i++) {
			uint32_t child = patches[i].node;
			if(child == sink) continue;
			if(visited[child]) continue;

			float d;
			if(closest(nodes[child].sphere, ray, d)) {
				candidates.push_back(DNode(child, d));
				push_heap(candidates.begin(), candidates.end());
				visited[child] = true;
			}
		}
	}
	distance = sqrt(distance);
	return hit;
}

