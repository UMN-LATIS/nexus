
#include "plyloader.h"

using namespace vcg;
using namespace vcg::ply;

#include <iostream>
using namespace std;

//Ugly ply stuff from vcg.

struct PlyFace {
    quint32 f[3];
#ifdef TEXTURE
    float t[6];
	quint32 texNumber;
#endif
    unsigned char n;
};
//TODO add uv
PropDescriptor plyprop1[13]= {
    {"vertex", "x",     T_FLOAT, T_FLOAT, offsetof(Vertex,v[0]),0,0,0,0,0,0},
    {"vertex", "y",     T_FLOAT, T_FLOAT, offsetof(Vertex,v[1]),0,0,0,0,0,0},
    {"vertex", "z",     T_FLOAT, T_FLOAT, offsetof(Vertex,v[2]),0,0,0,0,0,0},
    {"vertex", "red"  , T_UCHAR, T_UCHAR, offsetof(Vertex,c[0]),0,0,0,0,0,0},
    {"vertex", "green", T_UCHAR, T_UCHAR, offsetof(Vertex,c[1]),0,0,0,0,0,0},
    {"vertex", "blue" , T_UCHAR, T_UCHAR, offsetof(Vertex,c[2]),0,0,0,0,0,0},
    {"vertex", "alpha", T_UCHAR, T_UCHAR, offsetof(Vertex,c[3]),0,0,0,0,0,0},
    {"vertex", "nx",    T_FLOAT, T_FLOAT, offsetof(Splat, n[0]),0,0,0,0,0,0},
    {"vertex", "ny",    T_FLOAT, T_FLOAT, offsetof(Splat, n[1]),0,0,0,0,0,0},
    {"vertex", "nz",    T_FLOAT, T_FLOAT, offsetof(Splat, n[2]),0,0,0,0,0,0},
    {"vertex", "diffuse_red",   T_UCHAR, T_UCHAR, offsetof(Vertex,c[0]),0,0,0,0,0,0},
    {"vertex", "diffuse_green", T_UCHAR, T_UCHAR, offsetof(Vertex,c[1]),0,0,0,0,0,0},
	{"vertex", "diffuse_blue" , T_UCHAR, T_UCHAR, offsetof(Vertex,c[2]),0,0,0,0,0,0},
};
PropDescriptor doublecoords[3] = {
	{"vertex", "x",     T_DOUBLE, T_FLOAT, offsetof(Vertex,v[0]),0,0,0,0,0,0},
	{"vertex", "y",     T_DOUBLE, T_FLOAT, offsetof(Vertex,v[1]),0,0,0,0,0,0},
	{"vertex", "z",     T_DOUBLE, T_FLOAT, offsetof(Vertex,v[2]),0,0,0,0,0,0}
};

PropDescriptor plyprop2[1]=	{
    {"face", "vertex_indices",T_INT,T_UINT,offsetof(PlyFace,f[0]),
     1,0,T_UCHAR,T_UCHAR, offsetof(PlyFace,n) ,0}
};

PropDescriptor plyprop2_uint[1]=	{
    {"face", "vertex_indices",T_UINT,T_UINT,offsetof(PlyFace,f[0]),
     1,0,T_UCHAR,T_UCHAR, offsetof(PlyFace,n) ,0}
};

PropDescriptor plyprop3[1]=	{
    {"face", "vertex_index",T_INT,T_UINT,offsetof(PlyFace,f[0]),
     1,0,T_UCHAR,T_UCHAR, offsetof(PlyFace,n) ,0}
};

PropDescriptor plyprop3_uint[1]=	{
    {"face", "vertex_index",T_UINT,T_UINT,offsetof(PlyFace,f[0]),
     1,0,T_UCHAR,T_UCHAR, offsetof(PlyFace,n) ,0}
};

#ifdef TEXTURE
PropDescriptor plyprop4[1]=	{
    {"face", "texcoord",T_FLOAT,T_FLOAT,offsetof(PlyFace,t[0]),
     1,0,T_UCHAR,T_UCHAR, offsetof(PlyFace,n) ,0}
};

PropDescriptor plyprop5[1]=	{
	{"face", "texnumber",T_INT,T_INT,offsetof(PlyFace, texNumber), 0,0,0,0,0,0}
};
#endif


PlyLoader::PlyLoader(QString filename):
    vertices_element(-1),
    faces_element(-1),
    vertices("cache_plyvertex"),
    n_vertices(0),
    n_triangles(0),
    current_triangle(0),
    current_vertex(0) {

    int val = pf.Open(filename.toLatin1().data(), PlyFile::MODE_READ);
    if(val == -1) {
        int error = pf.GetError();
        throw QString("Could not open file '" + filename + "' error: %1").arg(error);

	}
    init();
    for(int co = 0; co < int(pf.comments.size()); ++co) {
        std::string TFILE = "TextureFile";
        std::string &c = pf.comments[co];
        std::string bufstr,bufclean;
        int i,n;

        if( TFILE == c.substr(0,TFILE.length()) )
        {
            bufstr = c.substr(TFILE.length()+1);
            n = static_cast<int>(bufstr.length());
            for(i=0;i<n;i++)
                if( bufstr[i]!=' ' && bufstr[i]!='\t' && bufstr[i]>32 && bufstr[i]<125 )	bufclean.push_back(bufstr[i]);

            char buf2[255];
            ply::interpret_texture_name( bufclean.c_str(),filename.toLatin1().data(), buf2 );
			texture_filenames.push_back(buf2);
        }
    }
	if(has_textures && texture_filenames.size() == 0)
		has_textures = false;
}

PlyLoader::~PlyLoader() {
    pf.Destroy();
}

void PlyLoader::init() {
    for(unsigned int i = 0; i < pf.elements.size(); i++) {

        if(!strcmp( pf.ElemName(i),"vertex")) {
            n_vertices = pf.ElemNumber(i);
            vertices_element = i;

        } else if( !strcmp( pf.ElemName(i),"face") ) {
            n_triangles = pf.ElemNumber(i);
            faces_element = i;
        }
	}
    //testing for required vertex fields.
    if(pf.AddToRead(plyprop1[0])==-1 ||
            pf.AddToRead(plyprop1[1])==-1 ||
			pf.AddToRead(plyprop1[2])==-1) {
		if(pf.AddToRead(doublecoords[0])==-1 ||
				pf.AddToRead(doublecoords[1])==-1 ||
				pf.AddToRead(doublecoords[2])==-1) {
			throw QString("Ply file has not xyz coords");
		}
	}

    //these calls will silently fail if no color is present
    int error = pf.AddToRead(plyprop1[3]);
    pf.AddToRead(plyprop1[4]);
    pf.AddToRead(plyprop1[5]);
    pf.AddToRead(plyprop1[6]);

    if(error ==  vcg::ply::E_NOERROR)
        has_colors = true;
    else {
        error = pf.AddToRead(plyprop1[10]);
        pf.AddToRead(plyprop1[11]);
        pf.AddToRead(plyprop1[12]);


        if(error ==  vcg::ply::E_NOERROR)
            has_colors = true;
    }

    //these calls will fail silently if no normals on vertices
    if(n_triangles == 0) {
        error = pf.AddToRead(plyprop1[7]);
        pf.AddToRead(plyprop1[8]);
        pf.AddToRead(plyprop1[9]);
        if(error == vcg::ply::E_NOERROR)
            has_normals = true;
    }

    pf.AddToRead(plyprop2[0]);
    pf.AddToRead(plyprop2_uint[0]);
    pf.AddToRead(plyprop3[0]);
    pf.AddToRead(plyprop3_uint[0]);
    if(pf.AddToRead(plyprop4[0]) == vcg::ply::E_NOERROR) {
        has_textures = true;
	}
	if(pf.AddToRead(plyprop5[0]) == vcg::ply::E_NOERROR) {
		//do nothing.
	}


    pf.SetCurElement(vertices_element);
}

void PlyLoader::cacheVertices() {
    vertices.setElementsPerBlock(1<<20);
    vertices.resize(n_vertices);

    //caching vertices on temporary file
    for(quint64 i = 0; i < n_vertices; i++) {
        Vertex &v = vertices[i];
		pf.Read((void *)&v);
        if(quantization) {
            quantize(v.v[0]);
            quantize(v.v[1]);
            quantize(v.v[2]);
        }
    }

    pf.SetCurElement(faces_element);
}

void PlyLoader::setMaxMemory(quint64 max_memory) {
    vertices.setMaxMemory(max_memory);
}

quint32 PlyLoader::getTriangles(quint32 size, Triangle *buffer) {
    if(faces_element == -1)
        throw "Ply has no faces.";

    if(current_triangle == 0)
        cacheVertices();

    if(current_triangle >= n_triangles) return 0;

    quint32 count = 0;

    PlyFace face;
	face.texNumber = 0;
    for(quint32 i = 0; i < size && current_triangle < n_triangles; i++) {

        pf.Read((void *) &face);
        Triangle &current = buffer[count];
        for(int k = 0; k < 3; k++) {
            Vertex &vertex = vertices[face.f[k]];
#ifdef TEXTURE
            vertex.t[0] = face.t[k*2];
            vertex.t[1] = face.t[k*2+1];
#endif
            current.vertices[k] = vertex;
        }
        current.node = 0;
		current.tex = face.texNumber + texOffset;

        current_triangle++;

        //ignore degenerate triangles
        if(current.isDegenerate())
            continue;

        count++;
    }
    return count;
}

quint32 PlyLoader::getVertices(quint32 size, Splat *vertex) {
    if(current_triangle > n_triangles) return 0;

    Splat v;
    v.node = 0;
    v.c[3] = 255;
    quint32 count = 0;
    for(quint32 i = 0; i < size && current_vertex < n_vertices; i++) {
        pf.Read((void *)&v);
        if(quantization) {
            quantize(v.v[0]);
            quantize(v.v[1]);
            quantize(v.v[2]);
        }
        vertex[count] = v;
        current_vertex++;
        count++;
    }
    return count;
}
