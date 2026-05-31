#define NANOVG_GL3_IMPLEMENTATION

#if defined(__APPLE__)
#define GL_SILENCE_DEPRECATION 1
#include <OpenGL/gl3.h>
#elif defined(_WIN32)
#include <SDL_opengl.h>
#else
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include "nanovg.h"
#include "nanovg_gl.h"
