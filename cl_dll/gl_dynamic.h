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
#include <TargetConditionals.h>
#if TARGET_OS_IOS
#include <OpenGLES/ES1/gl.h>
#else
#include <OpenGL/gl.h>
#endif //TARGET_OS_IOS
#else
#include <GL/gl.h>
#endif //__APPLE__

// For fog
extern decltype(&glFogi) GL_glFogi;

// For fullbright textures on models
extern decltype(&glPixelStorei) GL_glPixelStorei;
extern decltype(&glGenTextures) GL_glGenTextures;
extern decltype(&glBindTexture) GL_glBindTexture;
extern decltype(&glTexImage2D) GL_glTexImage2D;
extern decltype(&glTexParameteri) GL_glTexParameteri;

extern decltype(&glMatrixMode) GL_glMatrixMode;
extern decltype(&glLoadIdentity) GL_glLoadIdentity;
extern decltype(&glPushMatrix) GL_glPushMatrix;
extern decltype(&glPopMatrix) GL_glPopMatrix;
extern decltype(&glFrustum) GL_glFrustum;

#endif

#endif // GL_DYNAMIC_H
