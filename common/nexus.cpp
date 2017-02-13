#define _FILE_OFFSET_BITS 64

#include "nexus.h"
#include "controller.h"
#include "globalgl.h"

#include <QGLWidget>


using namespace nx;

Nexus::Nexus(Controller *control): controller(control), loaded(false), http_stream(false) {
}

Nexus::~Nexus() {
	close();
}

bool Nexus::open(const char *_uri) {
	filename = _uri;

	url = std::string(_uri);

	if(url.compare(0, 7, "http://") == 0) {
		if(!controller)
			throw "No controller, no http streaming";
		http_stream = true;
	}

	if(url.compare(0, 7, "file://") == 0)
		url = url.substr(7, url.size());

	if(!isStreaming()) {
		file.setFileName(url.c_str());
		if(!file.open(QIODevice::ReadWrite))
			//file = fopen(_uri, "rb+");
			//if(!file)
			return false;
	}
	if(!controller) {
		try {
			loadHeader();
			loadIndex();
		} catch(const char *error) {
			return false;
		}
	} else {
		controller->load(this);
	}
	return true;
}

void Nexus::flush() {
	if(controller) {
		controller->flush(this);
		return;
	}
	NexusData::flush();
	delete []tokens;
}

void Nexus::initIndex() {
	NexusData::initIndex();
	tokens = new Token[header.n_nodes];
	for(uint i = 0; i < header.n_nodes; i++) {
		tokens[i] = Token(this, i);
	}
}

void Nexus::loadIndex(char *buffer) {
	NexusData::loadIndex(buffer);

	loaded = true;
}
void Nexus::loadIndex() {
	NexusData::loadIndex();

	loaded = true;
}


bool Nexus::isReady() {
	return loaded;
}

uint64_t Nexus::loadGpu(uint32_t n) {
	NodeData &data = nodedata[n];
	assert(data.memory);
	assert(data.vbo == 0);

	Node &node = nodes[n];

	Signature &sig = header.signature;
	uint32_t vertex_size = node.nvert*sig.vertex.size();
	uint32_t face_size = node.nface*sig.face.size();

	char *vertex_start = data.memory;
	char *face_start = vertex_start + vertex_size;

	assert(!glGetError());

	glGenBuffers(1, (GLuint *)(&(data.vbo)));
	glBindBuffer(GL_ARRAY_BUFFER, data.vbo);
	glBufferData(GL_ARRAY_BUFFER, vertex_size, vertex_start, GL_STATIC_DRAW);

	if(node.nface) {
		glGenBuffers(1, (GLuint *)(&(data.fbo)));
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.fbo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, face_size, face_start, GL_STATIC_DRAW);
	}

	if(header.n_textures) {
		//be sure to load images
		for(uint32_t p = node.first_patch; p < node.last_patch(); p++) {
			Patch &patch = patches[p];
			uint32_t t = patch.texture;
			if(t == 0xffffffff) continue;
			TextureData &data = texturedata[t];
			data.count_gpu++;
			if(texturedata[t].tex) continue;
			assert(data.count_gpu == 1);
			Texture &texture = textures[t];

			QImage image;

//			QString basename = filename.left(filename.length()-4);
//			bool success = image.load(QString("%1_%2.jpg").arg(basename).arg(t)); */
			//temporary loading from files.
			quint64 size = texture.getSize();
			bool success = image.loadFromData((uchar *)data.memory, size);
			//assert(success);
			image = QGLWidget::convertToGLFormat(image);
			assert(!glGetError());
			glGenTextures(1, &data.tex);
			glBindTexture(GL_TEXTURE_2D, data.tex);
//			glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.bits());
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.bits());
			glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			assert(!glGetError());
			size += texture.getSize(); //careful with cache... might create problems to return different sizes in get drop and size
/*			glGenerateMipmap(GL_TEXTURE_2D);  //Generate mipmaps now!!!
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); */

		}
	}

	return node.getSize();
}

uint64_t Nexus::dropGpu(uint32_t n) {
	NodeData &data = nodedata[n];
	Node &node = nodes[n];
	//assert(data.vbo != 0);
#ifndef NO_OPENGL
	glDeleteBuffers(1, (GLuint *)(&(data.vbo)));
	if(node.nface)
		glDeleteBuffers(1, (GLuint *)(&(data.fbo)));
#endif
	data.vbo = data.fbo = 0;
	if(header.n_textures) {
		//be sure to load images
		for(uint32_t p = node.first_patch; p < node.last_patch(); p++) {
			Patch &patch = patches[p];
			uint32_t t = patch.texture;
			if(t == 0xffffffff) continue;
			TextureData &tdata = texturedata[t];
			tdata.count_gpu--;
			if(tdata.count_gpu != 0) continue;
			glDeleteTextures(1, &tdata.tex);
			tdata.tex = 0;
			//Texture &texture = textures[t];
			//size += texture.getSize(); //careful with cache... might create problems to return different sizes in get drop and size
		}
	}
	return node.getSize();
}

