#define NANOVG_GL3_IMPLEMENTATION

#if defined(__APPLE__)
#define GL_SILENCE_DEPRECATION 1
#include <OpenGL/gl3.h>
#else
#include <SDL_opengl.h>
#endif

#include "nanovg.h"
#include "nanovg_gl.h"
