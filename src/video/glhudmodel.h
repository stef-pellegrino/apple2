/*
 * Apple // emulator for *ix
 *
 * This software package is subject to the GNU General Public License
 * version 3 or later (your choice) as published by the Free Software
 * Foundation.
 *
 * Copyright 2013-2015 Aaron Culliney
 *
 */

#ifndef _GLHUDMODEL_H_
#define _GLHUDMODEL_H_

#include "common.h"
#include "video_util/modelUtil.h"

#define HUD_CLASS(CLS, ...) \
    MODEL_CLASS(CLS, \
        char *tpl;                  /* ASCII template */ \
        unsigned int tplWidth;      /* template width */ \
        unsigned int tplHeight;     /* template height */ \
        \
        uint8_t *pixels;            /* raw texture/FB data */ \
        unsigned int pixWidth;      /* FB width -- FIXME TODO : this is really the same as GLModel.texWidth */ \
        unsigned int pixHeight;     /* FB height -- FIXME TODO : this is really the same as GLModel.texHeight */ \
        \
        interface_colorscheme_t colorScheme; \
        bool blackIsTransparent; \
        bool opaquePixelHalo; \
        \
        __VA_ARGS__ \
    )

HUD_CLASS(GLModelHUDElement);

// default model creation
void *glhud_createDefault(void);

// default model setup
void glhud_setupDefault(GLModel *parent);

// render default
void glhud_renderDefault(GLModel *parent);

// default model destruction
void glhud_destroyDefault(GLModel *parent);

// translate screen coordinates to model
void glhud_screenToModel(const float x, const float y, const int screenW, const int screenH, float *centerX, float *centerY);

// orthographic translation of model coordinates to screen coordinates
void glhud_quadModelToScreen(const GLModel *model, const int screenW, const int screenH, float screenCoords[4]);

#endif
