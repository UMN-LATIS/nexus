#include <QTextStream>
#include "extractor.h"
#ifdef USE_OPENCTM
#include <openctmpp.h>
#endif

#include "../nxszip/meshcoder2.h"
//typedef MeshCoder MeshEncoder;

using namespace std;
using namespace nx;

Extractor::Extractor(NexusData *nx):
	transform(false),
	max_size(0), current_size(0),
	min_error(0),current_error(0),
	max_triangles(0), current_triangles(0) {

	nexus = nx;
	selected.resize(nexus->header.n_nodes, true);
	selected.back() = false;
}

void Extractor::setMatrix(vcg::Matrix44f m) {
	transform = true;
	matrix = m;
}

void Extractor::selectBySize(quint64 size) {
	max_size = size;
	traverse(nexus);
}

void Extractor::selectByError(float error) {
	min_error = error;
	traverse(nexus);
}

void Extractor::selectByTriangles(quint64 triangles) {
	max_triangles = triangles;
	traverse(nexus);
}

void Extractor::dropLevel() {
	selected.resize(nexus->header.n_nodes, true);
	quint32 n_nodes = nexus->header.n_nodes;
	quint32 sink = n_nodes-1;
	//unselect nodes which output is the sink (definition of last level :)
	for(uint i = 0; i < n_nodes-1; i++) {
		nx::Node &node = nexus->nodes[i];
		if(nexus->patches[node.first_patch].node == sink)
			selected[i] = false;

	}
	selected.back() = false; //sink unselection purely for coherence
}

void Extractor::save(QString output, nx::Signature &signature) {
	QFile file;
	file.setFileName(output);
	if(!file.open(QIODevice::WriteOnly | QFile::Truncate))
		throw "Could not open file: " + output + " for writing";

	nx::Header header = nexus->header;
	header.signature = signature;
	header.nvert = 0;
	header.nface = 0;

	if(transform)
		header.sphere.Center() = matrix * header.sphere.Center();

	vector<nx::Node> nodes;
	vector<nx::Patch> patches;
	vector<nx::Texture> textures;

	uint n_nodes = nexus->header.n_nodes;
	vector<int> node_remap(n_nodes, -1);


	for(uint i = 0; i < n_nodes-1; i++) {
		if(!selected[i]) continue;
		nx::Node node = nexus->nodes[i];
		if(transform)
			node.sphere.Center() = matrix * node.sphere.Center();
		header.nvert += node.nvert;
		header.nface += node.nface;

		node_remap[i] = nodes.size();

		node.first_patch = patches.size();
		for(uint k = nexus->nodes[i].first_patch; k < nexus->nodes[i].last_patch(); k++) {
			nx::Patch patch = nexus->patches[k];
			patches.push_back(patch);
		}

		nodes.push_back(node);
	}

	nx::Node sink = nexus->nodes[n_nodes -1];
	sink.first_patch = patches.size();
	nodes.push_back(sink);

	for(uint i = 0; i < patches.size(); i++) {
		nx::Patch &patch = patches[i];
		int remapped = node_remap[patch.node];
		if(remapped == -1)
			patch.node = nodes.size()-1;
		else
			patch.node = remapped;
		assert(patch.node < nodes.size());
	}

	cout << "n textures: " << nexus->header.n_textures << endl;
	textures.resize(nexus->header.n_textures);

	header.n_nodes = nodes.size();
	header.n_patches = patches.size();
	header.n_textures = textures.size();


	quint64 size = sizeof(nx::Header)  +
			nodes.size()*sizeof(nx::Node) +
			patches.size()*sizeof(nx::Patch) +
			textures.size()*sizeof(nx::Texture);
	size = pad(size);

	for(uint i = 0; i < nodes.size(); i++) {
		nodes[i].offset += size/NEXUS_PADDING;
	}

	//TODO should actually remove textures not used anymore.

	file.write((char *)&header, sizeof(header));
	file.write((char *)&*nodes.begin(), sizeof(nx::Node)*nodes.size());
	file.write((char *)&*patches.begin(), sizeof(nx::Patch)*patches.size());
	file.write((char *)&*textures.begin(), sizeof(nx::Texture)*textures.size());
	file.seek(size);

	for(uint i = 0; i < node_remap.size()-1; i++) {
		int n = node_remap[i];
		if(n == -1) continue;
		nx::Node &node = nodes[n];
		node.offset = file.pos()/NEXUS_PADDING;

		nexus->loadRam(i);

		NodeData &data = nexus->nodedata[i];
		char *memory = data.memory;
		int data_size = nexus->nodes[i].getSize();
		if(transform) {
			memory = new char[node.getSize()];
			memcpy(memory, data.memory, data_size);

			for(int k = 0; k < node.nvert; k++) {
				vcg::Point3f &v = ((vcg::Point3f *)memory)[k];
				v = matrix * v;
			}
		}
		if(signature.flags & nx::Signature::MECO ||
				signature.flags & nx::Signature::CTM1 ||
				signature.flags & nx::Signature::CTM2) {
			compress(file, signature, node, nexus->nodedata[i], &*patches.begin());
		} else {
			file.write(memory, data_size);
		}
		if(transform)
			delete []memory;

		nexus->dropRam(i);
	}
	nodes.back().offset = file.pos()/NEXUS_PADDING;

	//save textures
	if(textures.size()) {
		for(uint i = 0; i < textures.size()-1; i++) {
			Texture &in = nexus->textures[i];
			Texture &out = textures[i] = in;

			quint64 start = in.getBeginOffset();
			quint64 size = in.getSize();
			char *memory = (char *)nexus->file.map(start, size);

			out.offset = file.pos()/NEXUS_PADDING;
			file.write(memory, size);
		}
		textures.back().offset = file.pos()/NEXUS_PADDING;
	}

	file.seek(sizeof(nx::Header));
	file.write((char *)&*nodes.begin(), sizeof(nx::Node)*nodes.size());
	file.write((char *)&*patches.begin(), sizeof(nx::Patch)*patches.size());
	file.write((char *)&*textures.begin(), sizeof(nx::Texture)*textures.size());
	file.close();
}


nx::Traversal::Action Extractor::expand(nx::Traversal::HeapNode h) {
	nx::Node &node = nexus->nodes[h.node];
	current_size += node.getEndOffset() - node.getBeginOffset();
	current_triangles += node.nface;

	cout << "max size: " << max_size << " CUrrent siuze: " << current_size << endl;
	if(max_triangles && current_triangles > max_triangles)
		return STOP;
	if(max_size && current_size > max_size)
		return STOP;
	if(min_error && node.error < min_error)
		return STOP;

	return EXPAND;
}

quint32 Extractor::pad(quint32 s) {
	const quint32 padding = NEXUS_PADDING;
	quint64 m = (s-1) & ~(padding -1);
	return m + padding;
}

uint ctmwriter(const void *buffer, uint count, void *data) {
	QFile *file = (QFile *)data;
	return file->write((const char *)buffer, count);
}

void Extractor::compress(QFile &file, nx::Signature &signature, nx::Node &node, nx::NodeData &data, Patch *patches) {

	float error = node.error;
	//detect first node error which is out of boundary.
	if(signature.flags & Signature::MECO) {

		MeshEncoder coder(node, data, patches, signature);
		coder.coord_q = coord_q;
		coder.error = error_factor*node.error;
		coder.norm_q = norm_bits;
		for(int k = 0; k < 4; k++)
			coder.color_q[k] = color_bits[k];
		//HER if signaure has texture was set to (int)log2(tex_step * pow(2, -12)); //here set to -14
		//here we shold set to -log2(max_side/tex_step) where max_side width or height of the associated texture.
		//unfortunately we have no really fast to provide this info.....
		//so assume
		coder.tex_q = -(int)log2(512/tex_step);

		coder.encode();
		file.write((char *)&*coder.stream.buffer, coder.stream.size());
		//padding
		quint64 size = pad(file.pos()) - file.pos();
		char tmp[NEXUS_PADDING];
		file.write(tmp, size);

		//DUMPING nodes and svg info
		/*
		static int count = 0;
		QString filename("test_%1");
		filename = filename.arg(count);
		QFile file1(filename);
		file1.open(QFile::WriteOnly);
		file1.write((char *)&*coder.stream.buffer, coder.stream.size());

		QString jname("json_%1");
		jname = jname.arg(count++);
		QFile json(jname);
		json.open(QFile::WriteOnly);
		QTextStream stream(&json);
		stream << "{ \"nvert\": " << node.nvert
			   << "\n, \"nface\": " << node.nface
			   << "\n, \"normals\": " << (signature.vertex.hasNormals()?1:0)
			   << "\n, \"colors\": " << (signature.vertex.hasColors()?1:0)
			   << "\n, \"indices\": " << (signature.face.hasIndex()?1:0)
			   << "\n, \"patches\": [";
		for(int i = node.first_patch; i < node.last_patch(); i++) {
			stream << patches[i].triangle_offset;
			if(i != node.last_patch()-1)
				stream << ", ";
		}
		stream << "]\n}\n";
*/

	} else if(signature.flags & Signature::CTM1 || signature.flags & Signature::CTM2) {

#ifdef USE_OPENCTM
		cout << "CTM compression" << endl;
		float *vertices = (float *)data.coords();
		vcg::Point3s *normals = data.normals(signature, node.nvert);

		float *normalf = new float[node.nvert * 3];
		for(int i = 0; i < node.nvert; i++) {
			for(int k = 0; k < 3; k++)
				normalf[3*i + k] = normals[i][k]/32766.0f;
		}

		uint16_t *faces = data.faces(signature, node.nvert);

		uint *indices = new uint[node.nface * 3];
		for(int i = 0; i < node.nface *3; i++)
			indices[i] = faces[i];


		CTMcontext  context = ctmNewContext(CTM_EXPORT);
		if(signature.flags & Signature::CTM1)
			ctmCompressionMethod(context, CTM_METHOD_MG1);
		else
			ctmCompressionMethod(context, CTM_METHOD_MG2);

		ctmCompressionLevel(context, 9);
		ctmVertexPrecision(context, error_factor*node.error);
		ctmDefineMesh(context, vertices, node.nvert, indices, node.nface, normalf);

		float colors[node.nvert*4];
		vcg::Color4b *bcolors = data.colors(signature, node.nvert);
		for(int i = 0; i < node.nvert; i++) {
			for(int k =0 ; k < 3; k++) {
				colors[i*4 + k] = bcolors[i][k] / 256.0f;
			}
		}
		CTMenum map = ctmAddAttribMap(context, colors, "Color");
		ctmAttribPrecision(context, map, 1.0f/256.0f);

		quint64 start = file.pos();
		ctmSaveCustom(context, ctmwriter, (void *)&file);

		//padding!
		quint64 size = pad(file.pos());
		file.seek(size);

		ctmFreeContext(context);

		delete []normalf;
		delete []indices;
#else

		cerr << "CTM Compression was not enabled during compilation" << endl;
		exit(0);
#endif
	}
}



void Extractor::savePly(QString filename) {

	uint32_t n_nodes = nexus->header.n_nodes;
	Node *nodes = nexus->nodes;
	Patch *patches = nexus->patches;

	if(!selected.size())
		selected.resize(n_nodes, true);

	selected.back() = false;
	QFile ply(filename);
	if(!ply.open(QFile::ReadWrite)) {
		cerr << "Could not open file: " << qPrintable(filename) << endl;
		exit(-1);
	}
	//extracted patches
	quint64 n_vertices = 0;
	quint64 n_faces = 0;
	std::vector<quint64> offsets(n_nodes, 0);


	for(quint32 i = 0; i < n_nodes-1; i++) {
		offsets[i] = n_vertices;

		if(skipNode(i)) continue;

		Node &node = nodes[i];
		n_vertices += node.nvert;

		uint start = 0;
		for(uint p = node.first_patch; p < node.last_patch(); p++) {
			Patch &patch = patches[p];
			if(!selected[patch.node]) {
				n_faces += patch.triangle_offset - start;
			}
			start = patch.triangle_offset;
		}

	}

	bool has_colors = nexus->header.signature.vertex.hasColors();

	cout << "n vertices: " << n_vertices << endl;
	cout << "n faces: " << n_faces << endl;
	{ //stram flushes on destruction
		QTextStream stream(&ply);
		stream << "ply\n"
			   << "format binary_little_endian 1.0\n"
				  //<< "format ascii 1.0\n"
			   << "comment generated from nexus\n"
			   << "element vertex " << n_vertices << "\n"
			   << "property float x\n"
			   << "property float y\n"
			   << "property float z\n";
		if(has_colors) {
			stream << "property uchar red\n"
				   << "property uchar green\n"
				   << "property uchar blue\n"
				   << "property uchar alpha\n";
		}
		stream << "element face " << n_faces << "\n"
			   << "property list uchar int vertex_index\n"
			   << "end_header\n";        //qtextstrem adds a \n when closed. stupid.
	}

	//writing vertices
	quint32 bytes_per_vertex = 12; //3 floats
	if(has_colors) bytes_per_vertex += 4;

	quint64 verify_vertices = 0;
	for(uint n = 0; n < n_nodes-1; n++) {

		if(skipNode(n)) continue;

		Node &node = nodes[n];
		nexus->loadRam(n);
		NodeData &data = nexus->nodedata[n];

		char *buffer = new char[bytes_per_vertex * node.nvert];
		char *pos = buffer;
		vcg::Point3f *coords = data.coords();
		vcg::Color4b *colors = data.colors(nexus->header.signature, node.nvert);
		for(int k = 0; k < node.nvert; k++) {
			vcg::Point3f *p = (vcg::Point3f *)pos;
			*p = coords[k];
			p++;
			pos = (char *)p;
			if(has_colors) {
				vcg::Color4b *c  = (vcg::Color4b *)pos;
				*c = colors[k];
				c++;
				pos = (char *)c;
			}
		}
		ply.write((char *)buffer, bytes_per_vertex * node.nvert);
		verify_vertices += node.nvert;
		delete []buffer;
		nexus->dropRam(n);
	}
	assert(verify_vertices == n_vertices);


	//writing faces
	quint32 bytes_per_face = 1 + 3*sizeof(quint32);


	char *buffer = new char[bytes_per_face * (1<<16)];

	for(uint n = 0; n < n_nodes-1; n++) {

		if(skipNode(n)) continue;

		Node &node = nodes[n];


		uint32_t offset = offsets[n];

		nexus->loadRam(n);
		NodeData &data = nexus->nodedata[n];
		uint start = 0;
		for(uint p = node.first_patch; p < node.last_patch(); p++) {
			Patch &patch = patches[p];
			if(selected[patch.node]) {
				start = patch.triangle_offset;
				continue;
			}

			uint face_no = patch.triangle_offset - start;
			uint16_t *triangles = data.faces(nexus->header.signature, node.nvert);

			char *pos = buffer;
			for(uint k = start; k < patch.triangle_offset; k++) {
				*pos = 3;
				pos++;
				int *f = (int *)pos;
				for(int j = 0; j < 3; j++) {
					*f = offset + (int)triangles[3*k + j];
					f++;
				}
				pos = (char *)f;
			}

			ply.write((char *)buffer, bytes_per_face * face_no);

			start = patch.triangle_offset;
		}
		nexus->dropRam(n);
	}
	delete []buffer;


	ply.close();
}

struct PlyVertex {
	vcg::Point3f p;
	PlyVertex() {}
	PlyVertex(const vcg::Point3f &_p): p(_p){}
};

struct PlyColorVertex {
	vcg::Point3f v;
	vcg::Color4b c;
	PlyColorVertex(const vcg::Point3f &_v, const vcg::Color4b &_c): v(_v), c(_c) {}
};

struct PlyFace {
	char t[13];
	PlyFace() { t[0] = 3;}
	int &v(int k) {
		return *(int *)(t + 1 + k*4);
	}
};

void Extractor::saveUnifiedPly(QString filename) {

	uint32_t n_nodes = nexus->header.n_nodes;
	Node *nodes = nexus->nodes;
	Patch *patches = nexus->patches;

	bool has_colors = nexus->header.signature.vertex.hasColors();

	if(!selected.size())
		selected.resize(n_nodes, true);

	selected.back() = false;
	QFile ply(filename);
	if(!ply.open(QFile::ReadWrite)) {
		cerr << "Could not open file: " << qPrintable(filename) << endl;
		exit(-1);
	}
	//extracted patches
	quint64 n_vertices = 0;
	quint64 n_faces = 0;

	vector<PlyColorVertex> color_vertices;
	vector<PlyVertex> vertices;
	vector<PlyFace> faces;

	for(quint32 i = 0; i < n_nodes-1; i++) {
		if(skipNode(i)) continue;

		Node &node = nodes[i];
		NodeData &data = nexus->nodedata[i];
		nexus->loadRam(i);

		vcg::Point3f *coords = data.coords();
		vcg::Color4b *colors = data.colors(nexus->header.signature, node.nvert);
		uint16_t *triangles = data.faces(nexus->header.signature, node.nvert);


		vector<int> remap(node.nvert, -1);
		uint start = 0;

		for(uint p = node.first_patch; p < node.last_patch(); p++) {
			Patch &patch = patches[p];

			if(!selected[patch.node]) {

				for(uint k = start; k < patch.triangle_offset; k++) {
					PlyFace f;
					for(int j = 0; j < 3; j++) {
						int v = (int)triangles[3*k + j];
						if(remap[v] == -1) {

							if(has_colors) {
								remap[v] = color_vertices.size();
								color_vertices.push_back(PlyColorVertex(coords[v], colors[v]));
							} else {
								remap[v] = vertices.size();
								vertices.push_back(PlyVertex(coords[v]));
							}
						}
						f.v(j) = remap[v];
					}
					faces.push_back(f);
				}
			}
			start = patch.triangle_offset;
		}
		nexus->dropRam(i);

	}
	n_vertices = vertices.size()? vertices.size() : color_vertices.size();
	n_faces = faces.size();



	cout << "n vertices: " << n_vertices << endl;
	cout << "n faces: " << n_faces << endl;
	{
		QTextStream stream(&ply);
		stream << "ply\n"
			   << "format binary_little_endian 1.0\n"
				  //<< "format ascii 1.0\n"
			   << "comment generated from nexus\n"
			   << "element vertex " << n_vertices << "\n"
			   << "property float x\n"
			   << "property float y\n"
			   << "property float z\n";
		if(has_colors) {
			stream << "property uchar red\n"
				   << "property uchar green\n"
				   << "property uchar blue\n"
				   << "property uchar alpha\n";
		}
		stream << "element face " << n_faces << "\n"
			   << "property list uchar int vertex_index\n"
			   << "end_header\n";        //qtextstrem adds a \n when closed. stupid.
	}

	assert(sizeof(PlyVertex) == 12);
	assert(sizeof(PlyColorVertex) == 16);
	assert(sizeof(PlyFace) == 13);

	if(has_colors)
		ply.write((char *)&*color_vertices.begin(),sizeof(PlyColorVertex)*color_vertices.size());
	else
		ply.write((char *)&*vertices.begin(),sizeof(PlyVertex)*vertices.size());


	ply.write((char *)&*faces.begin(), sizeof(PlyFace)*faces.size());

	ply.close();
}

