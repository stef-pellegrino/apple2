/*
 * Apple // emulator for *nix
 *
 * This software package is subject to the GNU General Public License
 * version 2 or later (your choice) as published by the Free Software
 * Foundation.
 *
 * THERE ARE NO WARRANTIES WHATSOEVER.
 *
 */

// Modified sample code from https://developer.apple.com/library/mac/samplecode/GLEssentials/Introduction/Intro.html

#ifndef __MODEL_UTIL_H__
#define __MODEL_UTIL_H__

#include "glUtil.h"

#define UNINITIALIZED_GL 31337

typedef struct GLModel {
    GLuint numVertices;

    GLvoid *positions;
    GLenum positionType;
    GLuint positionSize;
    GLsizei positionArraySize;

    GLvoid *texcoords;
    GLenum texcoordType;
    GLuint texcoordSize;
    GLsizei texcoordArraySize;

    GLvoid *normals;
    GLenum normalType;
    GLuint normalSize;
    GLsizei normalArraySize;

    GLvoid *elements;
    GLenum elementType;
    GLuint numElements;
    GLsizei elementArraySize;

    GLenum primType;

} GLModel;

GLModel *mdlLoadModel(const char *filepathname);

GLModel *mdlLoadQuadModel();

void mdlDestroyModel(GLModel *model);

#endif //__MODEL_UTIL_H__
