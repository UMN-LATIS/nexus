#ifndef GLOBALGL_H
#define GLOBALGL_H


  #ifdef GL_ES

    #include <OpenGLES/ES2/gl.h>

  #else

    #include <GL/glew.h>

  #endif

#if defined(__APPLE__)
	#include <OpenGL/glu.h>
	#include <OpenGL/gl.h>
#endif

#endif // GLOBALGL_H
