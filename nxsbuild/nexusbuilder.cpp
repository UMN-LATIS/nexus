#include <QDebug>
#include <QThread>
#include <QFileInfo>
#include "vertex_cache_optimizer.h"

#include "nexusbuilder.h"
#include "kdtree.h"
#include "meshstream.h"
#include "mesh.h"
#include "tmesh.h"
#include "../common/nexus.h"

#include <iostream>
using namespace std;

using namespace nx;

NodeBox::NodeBox(KDTree *tree, uint32_t block) {
	for(int k = 0; k < 3; k++)
		axes[k] = tree->axes[k];
	box = tree->block_boxes[block];
}

bool NodeBox::isIn(vcg::Point3f &p) {
	return KDTree::isIn(axes, box, p);
}

vector<bool> NodeBox::markBorders(Node &node, vcg::Point3f *p, uint16_t *f) {
	vector<bool> border(node.nvert, false);
	for(int i = 0; i < node.nface; i++) {
		bool outside = false;
		for(int k = 0; k < 3; k++) {
			uint16_t index = f[i*3 + k];
			outside |= !isIn(p[index]);
		}
		if(outside)
			for(int k = 0; k < 3; k++) {
				uint16_t index = f[i*3 + k];
				border[index] = true;
			}
	}
	return border;
}

NexusBuilder::NexusBuilder(quint32 components): chunks("cache_chunks"), scaling(0.5) {

	Signature &signature = header.signature;
	signature.vertex.setComponent(VertexElement::COORD, Attribute(Attribute::FLOAT, 3));
	if(components & FACES)     //ignore normals for meshes
		signature.face.setComponent(FaceElement::INDEX, Attribute(Attribute::UNSIGNED_SHORT, 3));
	if(components & NORMALS)
		signature.vertex.setComponent(VertexElement::NORM, Attribute(Attribute::SHORT, 3));
	if(components & COLORS)
		signature.vertex.setComponent(VertexElement::COLOR, Attribute(Attribute::BYTE, 4));
	if(components & TEXTURES)
		signature.vertex.setComponent(FaceElement::TEX, Attribute(Attribute::FLOAT, 2));

	header.version = 2;
	header.signature = signature;
	header.nvert = header.nface = header.n_nodes = header.n_patches = header.n_textures = 0;
}

NexusBuilder::NexusBuilder(Signature &signature): chunks("cache_chunks"), scaling(0.5) {
	header.version = 2;
	header.signature = signature;
	header.nvert = header.nface = header.n_nodes = header.n_patches = header.n_textures = 0;
}

void NexusBuilder::create(KDTree *tree, Stream *stream, uint top_node_size) {
	Node sink;
	sink.first_patch = 0;
	nodes.push_back(sink);

	int level = 0;
	do {
		cout << "Creating level " << level << endl;
		tree->clear();
		if(level % 2) tree->setAxesDiagonal();
		else tree->setAxesOrthogonal();

		tree->load(stream);
		stream->clear();

		createLevel(tree, stream, level);
		level++;
	} while(tree->nLeaves() > 1 || stream->size() > top_node_size);

	reverseDag();
	saturate();
}

/*
  Commented because the gain is negligible ( and the code is not correct either, there is
  some problem in saving the trianglws...


class Worker: public QThread {
public:
	uint block;
	KDTree &input;
	StreamSoup &output;
	NexusBuilder &builder;

	Worker(uint n, KDTree &in, StreamSoup &out, NexusBuilder &parent):
		block(n), input(in), output(out), builder(parent) {}

protected:
	void run() {
		Mesh mesh;
		Soup soup;
		{
			QMutexLocker locker(&builder.m_input);
			soup = input->getSoup(block, true);
		}
		mesh.load(soup);
		input->lock(mesh, block);

		{
			//QMutexLocker locker(&builder.m_input);
			input->dropSoup(block);
		}

		quint32 patch_offset = 0;
		quint32 chunk = 0;
		{
			QMutexLocker output(&builder.m_chunks);

			//save node in nexus temporary structure
			quint32 mesh_size = mesh.serializedSize(builder.header.signature);
			mesh_size = builder.pad(mesh_size);
			chunk = builder.chunks.addChunk(mesh_size);
			uchar *buffer = builder.chunks.getChunk(chunk);
			patch_offset = builder.patches.size();
			std::vector<Patch> node_patches;
			mesh.serialize(buffer, builder.header.signature, node_patches);

			//patches will be reverted later, but the local order is important because of triangle_offset
			std::reverse(node_patches.begin(), node_patches.end());
			builder.patches.insert(builder.patches.end(), node_patches.begin(), node_patches.end());
		}

		float error = mesh.simplify(mesh.fn/2, Mesh::QUADRICS);

		quint32 current_node = 0;
		{
			QMutexLocker locker(&builder.m_builder);

			current_node = builder.nodes.size();
			nx::Node node = mesh.getNode();
			node.offset = chunk; //temporaryle remember which chunk belongs to which node
			node.error = error;
			node.first_patch = patch_offset;
			builder.nodes.push_back(node);
		}

		{
			QMutexLocker locker(&builder.m_output);
			Triangle *triangles = new Triangle[mesh.fn];
			//streaming the output TODO be sure not use to much memory on the chunks: we are writing sequentially
			mesh.getTriangles(triangles, current_node);
			for(int i = 0; i < mesh.fn; i++)
				output.pushTriangle(triangles[i]);

			delete []triangles;
		}

	}
}; */

void NexusBuilder::createLevel(KDTree *in, Stream *out, int level) {
	KDTreeSoup *test = dynamic_cast<KDTreeSoup *>(in);
	if(!test){
		KDTreeCloud *input = dynamic_cast<KDTreeCloud *>(in);
		StreamCloud *output = dynamic_cast<StreamCloud *>(out);

		for(uint block = 0; block < input->nBlocks(); block++) {
			Cloud cloud = input->get(block);
			assert(cloud.size() < (1<<16));
			if(cloud.size() == 0) continue;

			Mesh mesh;
			mesh.load(cloud);

			int target_points = cloud.size()*scaling;
			std::vector<AVertex> deleted = mesh.simplifyCloud(target_points);

			//save node in nexus temporary structure
			quint32 mesh_size = mesh.serializedSize(header.signature);
			mesh_size = pad(mesh_size);
			quint32 chunk = chunks.addChunk(mesh_size);
			uchar *buffer = chunks.getChunk(chunk);
			quint32 patch_offset = patches.size();

			std::vector<Patch> node_patches;
			mesh.serialize(buffer, header.signature, node_patches);

			//patches will be reverted later, but the local order is important because of triangle_offset
			std::reverse(node_patches.begin(), node_patches.end());
			patches.insert(patches.end(), node_patches.begin(), node_patches.end());

			quint32 current_node = nodes.size();
			nx::Node node = mesh.getNode();
			node.offset = chunk; //temporaryle remember which chunk belongs to which node
			node.error = mesh.averageDistance();
			node.first_patch = patch_offset;
			nodes.push_back(node);
			boxes.push_back(NodeBox(input, block));


			//we pick the deleted vertices from simplification and reprocess them.

			swap(mesh.vert, deleted);
			mesh.vn = mesh.vert.size();

			Splat *vertices = new Splat[mesh.vn];
			mesh.getVertices(vertices, current_node);

			for(int i = 0; i < mesh.vn; i++) {
				Splat &s = vertices[i];
				output->pushVertex(s);
			}

			delete []vertices;
		}


	} else {
		KDTreeSoup *input = dynamic_cast<KDTreeSoup *>(in);
		StreamSoup *output = dynamic_cast<StreamSoup *>(out);


		int current_texture = textures.size();
		if(hasTextures()) {
			//keep texture information

			QString filename = input->textures.front();
			QImage img;
			img.load(filename);

			cout << "Level: " << level << endl;
			cout << "TEXSIZE: " << img.width() << endl;

			if(level % 2 == 0 && img.width() > 32) {
				images.push_back(filename);
				QFileInfo info(filename);
				if(!info.exists())
					throw QString("Missing texture '%1'!").arg(filename);

				Texture t;
				t.offset = info.size();
				textures.push_back(t);
				//create 1.4 size texture and store it.

				img = img.scaled(img.width()*0.5, img.height()*0.5, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

				QString texture_filename = QString("nexus_tmp_tex%1.png").arg(current_texture);
				bool success = img.save(texture_filename);
				if(!success)
					throw QString("Could not save img: '%1'").arg(texture_filename);
				output->textures.push_back(texture_filename);
			} else {
				current_texture = textures.size()-1;
				output->textures.push_back(filename);
			}
		}

		//const int n_threads = 3;
		//QList<Worker *> workers;

		for(uint block = 0; block < input->nBlocks(); block++) {

			/*if(workers.size() > n_threads) {
			workers.front()->wait();
			delete workers.front();
			workers.pop_front();
		}
		Worker *worker = new Worker(block, input, output, *this);
		worker->start();
		workers.push_back(worker);
		//worker->wait();
		*/

			Soup soup = input->get(block);
			assert(soup.size() < (1<<16));
			if(soup.size() == 0) continue;

			TMesh mesh;
			Mesh mesh1;
			TMesh tmp;
			quint32 mesh_size;

			if(!hasTextures()) {
				mesh1.load(soup);
				input->lock(mesh1, block);
				mesh_size = mesh1.serializedSize(header.signature);
			} else {
				mesh.load(soup);
				input->lock(mesh, block);
				//we need to replicate vertices where textured seams occours

				vcg::tri::Append<TMesh,TMesh>::MeshCopy(tmp,mesh);
				for(int i = 0; i < tmp.face.size(); i++)
					tmp.face[i].node = mesh.face[i].node;
				tmp.splitSeams(header.signature);
				//save node in nexus temporary structure
				mesh_size = tmp.serializedSize(header.signature);
			}
			mesh_size = pad(mesh_size);
			quint32 chunk = chunks.addChunk(mesh_size);
			uchar *buffer = chunks.getChunk(chunk);
			quint32 patch_offset = patches.size();
			std::vector<Patch> node_patches;

			if(!hasTextures()) {
				mesh1.serialize(buffer, header.signature, node_patches);
			} else {
				tmp.serialize(buffer, header.signature, node_patches);
				for(Patch &patch: node_patches)
					patch.texture = current_texture;
			}

			//patches will be reverted later, but the local order is important because of triangle_offset
			std::reverse(node_patches.begin(), node_patches.end());
			patches.insert(patches.end(), node_patches.begin(), node_patches.end());

			nx::Node node;
			if(!hasTextures())
				node = mesh1.getNode(); //get node data before simplification
			else
				node = tmp.getNode();

			float error;
			int nface;
			if(!hasTextures()) {
				error = mesh1.simplify(soup.size()*scaling, Mesh::QUADRICS);
				nface = mesh1.fn;
			} else {
				error = mesh.simplify(soup.size()*scaling, TMesh::QUADRICS);
				nface = mesh.fn;
			}

			quint32 current_node = nodes.size();

			node.offset = chunk; //temporaryle remember which chunk belongs to which node
			node.error = error;
			node.first_patch = patch_offset;
			nodes.push_back(node);
			boxes.push_back(NodeBox(input, block));

			Triangle *triangles = new Triangle[nface];
			//streaming the output TODO be sure not use to much memory on the chunks: we are writing sequentially
			if(!hasTextures()) {
				mesh1.getTriangles(triangles, current_node);
			} else {
				mesh.getTriangles(triangles, current_node);
			}
			for(int i = 0; i < nface; i++) {
				Triangle &t = triangles[i];
				if(!t.isDegenerate())
					output->pushTriangle(triangles[i]);
			}

			delete []triangles;
		}
		/*while(workers.size()) {
		workers.front()->wait();
		delete workers.front();
		workers.pop_front();
	}*/


	}
}

void NexusBuilder::saturate() {
	//we do not have the 'backlinks' so we make a depth first traversal
	//TODO! BIG assumption: the nodes are ordered such that child comes always after parent
	for(int node = nodes.size()-2; node >= 0; node--)
		saturateNode(node);

	//nodes.front().error = nodes.front().sphere.Radius()/2;
	nodes.back().error = 0;
}

void NexusBuilder::testSaturation() {

	//test saturation:
	for(uint n = 0; n < nodes.size()-1; n++) {
		Node &node = nodes[n];
		vcg::Sphere3f &sphere = node.sphere;
		for(uint p = node.first_patch; p < node.last_patch(); p++) {
			Patch &patch = patches[p];
			Node &child = nodes[patch.node];
			vcg::Sphere3f s = child.sphere;
			float dist = (sphere.Center() - s.Center()).Norm();
			float R = sphere.Radius();
			float r = s.Radius();
			assert(sphere.IsIn(child.sphere));
			assert(child.error < node.error);
		}
	}
}

void NexusBuilder::reverseDag() {

	std::reverse(nodes.begin(), nodes.end());
	std::reverse(boxes.begin(), boxes.end());
	std::reverse(patches.begin(), patches.end());

	//first reversal: but we point now to the last_patch, not the first
	for(uint i = 0; i < nodes.size(); i++)
		nodes[i].first_patch = patches.size() -1 - nodes[i].first_patch;

	//get the previous node last +1 to become the first
	for(uint i = nodes.size()-1; i >= 1; i--)
		nodes[i].first_patch = nodes[i-1].first_patch +1;
	nodes[0].first_patch  = 0;

	//and reversed the order of the nodes.
	for(uint i = 0; i < patches.size(); i++) {
		patches[i].node = nodes.size() - 1 - patches[i].node;
	}
}


void NexusBuilder::save(QString filename) {

	cout << "Saving to file: " << qPrintable(filename) << endl;

	file.setFileName(filename);
	if(!file.open(QIODevice::ReadWrite | QIODevice::Truncate))
		throw QString("Could not open file; " + filename);

	if(header.signature.vertex.hasNormals() && header.signature.face.hasIndex())
		uniformNormals();

	if(textures.size())
		textures.push_back(Texture());

	header.nface = 0;
	header.nvert = 0;
	header.n_nodes = nodes.size();
	header.n_patches = patches.size();

	header.n_textures = textures.size();
	header.version = 2;
	header.sphere = nodes[0].tightSphere();

	for(uint i = 0; i < nodes.size()-1; i++) {
		nx::Node &node = nodes[i];
		header.nface += node.nface;
		header.nvert += node.nvert;
	}

	quint64 size = sizeof(Header)  +
			nodes.size()*sizeof(Node) +
			patches.size()*sizeof(Patch) +
			textures.size()*sizeof(Texture);
	size = pad(size);
	quint64 index_size = size;

	std::vector<quint32> node_chunk; //for each node the corresponding chunk
	for(quint32 i = 0; i < nodes.size()-1; i++)
		node_chunk.push_back(nodes[i].offset);

	//compute offsets and store them in nodes
	for(uint i = 0; i < nodes.size()-1; i++) {
		nodes[i].offset = size/NEXUS_PADDING;
		quint32 chunk = node_chunk[i];
		size += chunks.chunkSize(chunk);
	}
	nodes.back().offset = size/NEXUS_PADDING;

	if(textures.size()) {
		for(uint i = 0; i < textures.size()-1; i++) {
			quint32 s = textures[i].offset;
			textures[i].offset = size/NEXUS_PADDING;
			size += s;
			size = pad(size);
		}
		textures.back().offset = size/NEXUS_PADDING;
	}

	qint64 r = file.write((char*)&header, sizeof(Header));
	if(r == -1)
		cout << qPrintable(file.errorString()) << endl;
	assert(nodes.size());
	file.write((char*)&(nodes[0]), sizeof(Node)*nodes.size());
	if(patches.size())
		file.write((char*)&(patches[0]), sizeof(Patch)*patches.size());
	if(textures.size())
		file.write((char*)&(textures[0]), sizeof(Texture)*textures.size());
	file.seek(index_size);

	//NODES
	for(uint i = 0; i < node_chunk.size(); i++) {
		quint32 chunk = node_chunk[i];
		uchar *buffer = chunks.getChunk(chunk);
		optimizeNode(i, buffer);
		file.write((char*)buffer, chunks.chunkSize(chunk));
	}

	//cout << "File pos after nodes: " << file.pos()/NEXUS_PADDING << endl;
	//TEXTURES

	QString basename = filename.left(filename.length()-4);
	//compute textures offsetse
	//Image should store all the mipmaps
	cout << "Textures size: " << textures.size() << endl;
	if(textures.size()) {
		for(int i = 0; i < textures.size()-1; i++) {
			//cout << "Saving: " << qPrintable(images[i]) << endl;
			Texture &tex = textures[i];



			assert(tex.offset == file.pos()/NEXUS_PADDING);
			QFile image(images[i]);
			if(!image.open(QFile::ReadOnly))
				throw QString("Could not load img: '%1'").arg(images[i]);
			bool success = file.write(image.readAll());
			if(!success)
				throw QString("Failed writing texture");

			quint64 s = file.pos();
			s = pad(s);
			file.resize(s);
			file.seek(s);
		}
		for(int i = 0; i < textures.size()-1; i++) {
			//cout << "removing: " << qPrintable(QString("tex%1.jpg").arg(i)) << endl;
			QFile::remove(QString("nexus_tmp_tex%1.png").arg(i));
		}
	}
	file.close();
}

quint32 NexusBuilder::pad(quint32 s) {
	const quint32 padding = NEXUS_PADDING;
	quint64 m = (s-1) & ~(padding -1);
	return m + padding;
}

//include sphere of the children and ensure error s bigger.
void NexusBuilder::saturateNode(quint32 n) {
	const float epsilon = 1.01f;

	nx::Node &node = nodes[n];
	for(quint32 i = node.first_patch; i < node.last_patch(); i++) {
		nx::Patch &patch = patches[i];
		if(patch.node == nodes.size()-1) //sink, get out
			return;

		nx::Node &child = nodes[patch.node];
		if(node.error <= child.error)
			node.error = child.error*epsilon;

		//we cannot just add the sphere, because it moves the center and the tight radius will be wrong
		if(!node.sphere.IsIn(child.sphere)) {
			float dist = (child.sphere.Center() - node.sphere.Center()).Norm();
			dist += child.sphere.Radius();
			if(dist > node.sphere.Radius())
				node.sphere.Radius() = dist;
		}
	}
	node.sphere.Radius() *= epsilon;
}

void NexusBuilder::optimizeNode(quint32 n, uchar *chunk) {
	return;
	Node &node = nodes[n];
	assert(node.nface);

	uint16_t *faces = (uint16_t  *)(chunk + node.nvert*header.signature.vertex.size());

	uint start =  0;
	for(uint i = node.first_patch; i < node.last_patch(); i++) {
		Patch &patch = patches[i];
		uint end = patch.triangle_offset;
		uint nface = end - start;
		//optimizing vertex cache.
		quint16 *triangles = new quint16[nface*3];

		bool success = vmath::vertex_cache_optimizer::optimize_post_tnl(24, faces + 3*start, nface, node.nvert,  triangles);
		if(success)
			memcpy(faces + start, triangles, 3*sizeof(quint16)*nface);
		else
			cout << "Failed cache optimization" << endl;
		delete []triangles;
		start = end;
	}
}


void NexusBuilder::appendBorderVertices(uint32_t origin, uint32_t destination, std::vector<NVertex> &vertices) {
	Node &node = nodes[origin];
	uint32_t chunk = node.offset; //chunk index was stored here.


	uchar *buffer = chunks.getChunk(chunk, true);
	vcg::Point3f *point = (vcg::Point3f *)buffer;
	int size = sizeof(vcg::Point3f) + header.signature.vertex.hasTextures()*sizeof(vcg::Point2f);
	vcg::Point3s *normal = (vcg::Point3s *)(buffer + size * node.nvert);
	uint16_t *face = (uint16_t *)(buffer + header.signature.vertex.size()*node.nvert);

	NodeBox &nodebox = boxes[destination];
	vector<bool> border = nodebox.markBorders(node, point, face);
	for(int i = 0; i < node.nvert; i++) {
		if(border[i])
			vertices.push_back(NVertex(origin, i, point[i], normal + i));
	}
}


void NexusBuilder::uniformNormals() {

	cout << "Unifying normals\n";
	/* level 0: for each node in the lowest level:
			load the neighboroughs
			find common vertices (use lock to find the vertices)
			average normals

	   level > 0: for every other node (goind backwards)
			load child nodes
			find common vertices (use lock!)
			copy normal */


	uint32_t sink = nodes.size()-1;
	for(int t = sink-1; t > 0; t--) {
		Node &target = nodes[t];

		vcg::Box3f box = boxes[t].box;
		box.Offset(box.Diag()/100);


		std::vector<NVertex> vertices;

		appendBorderVertices(t, t, vertices);

		bool last_level = (patches[target.first_patch].node == sink);

		if(last_level) {//find neighboroughs among the same level

			for(int n = t-1; n >= 0; n--) {
				Node &node = nodes[n];
				if(patches[node.first_patch].node != sink) continue;
				if(!box.Collide(boxes[n].box)) continue;

				appendBorderVertices(n, t, vertices);
			}

		} else {

			for(uint p = target.first_patch; p < target.last_patch(); p++) {
				uint n = patches[p].node;

				appendBorderVertices(n, t, vertices);
			}
		}

		if(!vertices.size()) { //THIS IS HIGHLY UNLIKELY
			continue;
		}

		sort(vertices.begin(), vertices.end());

		uint last = 0;
		vcg::Point3f previous(vertices[0].point);

		for(uint k = 0; k < vertices.size(); k++) {
			NVertex &v = vertices[k];
			if(v.point != previous) {
				//uniform normals;
				if(k - last > 1) {   //more than 1 vertex to unify

					vcg::Point3s normals;

					if(last_level) {     //average all normals of the coincident points.

						vcg::Point3f normalf(0, 0, 0);
						for(uint j = last; j < k; j++)
							for(int l = 0; l < 3; l++)
								normalf[l] += (*vertices[j].normal)[l];
						normalf.Normalize();

						//convert back to shorts
						for(int l = 0; l < 3; l++)
							normals[l] = (short)(normalf[l]*32766);

					} else { //just copy the first one (it's from the lowest level.

						normals = (*vertices[last].normal);
					}

					for(uint j = last; j < k; j++)
						*vertices[j].normal = normals;
				}
				previous =v.point;
				last = k;
			}
		}
		if(chunks.memoryUsed() > max_memory)
			chunks.flush();
	}
}
