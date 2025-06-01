#pragma once
#ifndef GL_DYNAMIC_H
#define GL_DYNAMIC_H

#if (_WIN32 || (__unix__ && !__ANDROID__) || __APPLE__)
#define OPENGL_AVAILABLE 1
#else
#define OPENGL_AVAILABLE 0
#endif

#if OPENGL_AVAILABLE

#if defined (_WIN32)

#else // _WIN32

#define APIENTRY // __stdcall ?

#endif // _WIN32

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

typedef void (APIENTRY *GLAPI_glFogi)(GLenum pname, GLint param);

typedef void (APIENTRY *GLAPI_glPixelStorei)(GLenum pname, GLint param);
typedef void (APIENTRY *GLAPI_glGenTextures)(GLsizei n, GLuint *textures);
typedef void (APIENTRY *GLAPI_glBindTexture)(GLenum target, GLuint texture);
typedef void (APIENTRY *GLAPI_glTexImage2D)(GLenum target, GLint level,
										   GLint internalFormat,
										   GLsizei width, GLsizei height,
										   GLint border, GLenum format, GLenum type,
										   const GLvoid *pixels);
typedef void (APIENTRY *GLAPI_glTexParameteri)(GLenum target, GLenum pname, GLint param);

// For fog
extern GLAPI_glFogi GL_glFogi;

// For fullbright textures on models
extern GLAPI_glPixelStorei GL_glPixelStorei;
extern GLAPI_glGenTextures GL_glGenTextures;
extern GLAPI_glBindTexture GL_glBindTexture;
extern GLAPI_glTexImage2D GL_glTexImage2D;
extern GLAPI_glTexParameteri GL_glTexParameteri;

#endif

#endif // GL_DYNAMIC_H
