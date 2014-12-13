/* KallistiGL for KallistiOS ##version##

   libgl/gl-api.c
   Copyright (C) 2013-2014 Josh Pearson
   Copyright (C) 2014 Lawrence Sebald

   Some functionality adapted from the original KOS libgl:
   Copyright (C) 2001 Dan Potter
   Copyright (C) 2002 Benoit Miller

   This API implements much but not all of the OpenGL 1.1 for KallistiOS.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gl.h"
#include "glu.h"
#include "gl-api.h"
#include "gl-sh4.h"
#include "gl-pvr.h"

//====================================================================================================//
//== Local API State Macine Variables ==//

static GLubyte GL_KOS_DEPTH_FUNC  = PVR_DEPTHCMP_GEQUAL;
static GLubyte GL_KOS_DEPTH_WRITE = PVR_DEPTHWRITE_ENABLE;
static GLubyte GL_KOS_BLEND_FUNC  = (PVR_BLEND_ONE << 4) | (PVR_BLEND_ZERO & 0x0F);
static GLubyte GL_KOS_SHADE_FUNC  = PVR_SHADE_GOURAUD;
static GLubyte GL_KOS_CULL_FUNC   = PVR_CULLING_NONE;
static GLubyte GL_KOS_FACE_FRONT  = 0;
static GLubyte GL_KOS_SUPERSAMPLE = 0;

static GLuint  GL_KOS_VERTEX_COUNT = 0;
static GLuint  GL_KOS_VERTEX_MODE  = GL_TRIANGLES;
static GLuint  GL_KOS_VERTEX_COLOR = 0xFFFFFFFF;
static GLfloat GL_KOS_VERTEX_UV[2] = { 0, 0 };

static GLfloat GL_KOS_COLOR_CLEAR[3] = { 0, 0, 0 };

static GLfloat GL_KOS_POINT_SIZE = 0.02;

static pvr_poly_cxt_t GL_KOS_POLY_CXT;

static inline GLvoid _glKosFlagsSetTriangleStrip();
static inline GLvoid _glKosFlagsSetTriangle();
static inline GLvoid _glKosFlagsSetQuad();

//====================================================================================================//
//== API Initialization ==//

GLvoid APIENTRY glKosInit() {
    _glKosInitPVR();

    _glKosInitTextures();

    _glKosInitMatrix();

    _glKosInitLighting();

    _glKosInitFrameBuffers();
}

//====================================================================================================//
//== Blending / Shading functions ==//

GLvoid APIENTRY glShadeModel(GLenum   mode) {
    switch(mode) {
        case GL_FLAT:
            GL_KOS_SHADE_FUNC = PVR_SHADE_FLAT;
            break;

        case GL_SMOOTH:
            GL_KOS_SHADE_FUNC = PVR_SHADE_GOURAUD;
            break;
    }
}

GLvoid APIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor) {
    GL_KOS_BLEND_FUNC = 0;

    switch(sfactor) {
        case GL_ONE:
            GL_KOS_BLEND_FUNC |= (PVR_BLEND_ONE & 0XF) << 4;
            break;

        case GL_ZERO:
            GL_KOS_BLEND_FUNC |= (PVR_BLEND_ZERO & 0XF) << 4;
            break;

        case GL_SRC_COLOR:
            GL_KOS_BLEND_FUNC |= (PVR_BLEND_SRCALPHA & 0XF) << 4;
            break;

        case GL_DST_COLOR:
            GL_KOS_BLEND_FUNC |= (PVR_BLEND_DESTCOLOR & 0XF) << 4;
            break;

        case GL_SRC_ALPHA:
            GL_KOS_BLEND_FUNC |= (PVR_BLEND_SRCALPHA << 4);
            break;

        case GL_DST_ALPHA:
            GL_KOS_BLEND_FUNC |= (PVR_BLEND_DESTALPHA & 0XF) << 4;
            break;

        case GL_ONE_MINUS_SRC_ALPHA:
            GL_KOS_BLEND_FUNC |= (PVR_BLEND_INVSRCALPHA & 0XF) << 4;
            break;

        case GL_ONE_MINUS_DST_ALPHA:
            GL_KOS_BLEND_FUNC |= (PVR_BLEND_INVDESTALPHA & 0XF) << 4;
            break;

        case GL_ONE_MINUS_DST_COLOR:
            GL_KOS_BLEND_FUNC |= (PVR_BLEND_INVDESTCOLOR & 0XF) << 4;
            break;
    }

    switch(dfactor) {
        case GL_ONE:
            GL_KOS_BLEND_FUNC |= (PVR_BLEND_ONE & 0XF);
            break;

        case GL_ZERO:
            GL_KOS_BLEND_FUNC |= (PVR_BLEND_ZERO & 0XF);
            break;

        case GL_SRC_COLOR:
            GL_KOS_BLEND_FUNC |= (PVR_BLEND_SRCALPHA & 0XF);
            break;

        case GL_DST_COLOR:
            GL_KOS_BLEND_FUNC |= (PVR_BLEND_DESTCOLOR & 0XF);
            break;

        case GL_SRC_ALPHA:
            GL_KOS_BLEND_FUNC |= (PVR_BLEND_SRCALPHA & 0XF);
            break;

        case GL_DST_ALPHA:
            GL_KOS_BLEND_FUNC |= (PVR_BLEND_DESTALPHA & 0XF);
            break;

        case GL_ONE_MINUS_SRC_ALPHA:
            GL_KOS_BLEND_FUNC |= (PVR_BLEND_INVSRCALPHA & 0XF);
            break;

        case GL_ONE_MINUS_DST_ALPHA:
            GL_KOS_BLEND_FUNC |= (PVR_BLEND_INVDESTALPHA & 0XF);
            break;

        case GL_ONE_MINUS_DST_COLOR:
            GL_KOS_BLEND_FUNC |= (PVR_BLEND_INVDESTCOLOR & 0XF);
            break;
    }
}

//====================================================================================================//
//== Depth / Clear functions ==//

GLvoid APIENTRY glClear(GLuint mode) {
    if(mode & GL_COLOR_BUFFER_BIT)
        pvr_set_bg_color(GL_KOS_COLOR_CLEAR[0], GL_KOS_COLOR_CLEAR[1], GL_KOS_COLOR_CLEAR[2]);
}

GLvoid APIENTRY glClearColor(float r, float g, float b, float a) {
    if(r > 1) r = 1;

    if(g > 1) g = 1;

    if(b > 1) b = 1;

    if(a > 1) a = 1;

    GL_KOS_COLOR_CLEAR[0] = r * a;
    GL_KOS_COLOR_CLEAR[1] = g * a;
    GL_KOS_COLOR_CLEAR[2] = b * a;
}

//== NoOp ==//
GLvoid APIENTRY glClearDepthf(GLfloat depth) {
    ;
}

GLvoid APIENTRY glDepthFunc(GLenum func) {
    switch(func) {
        case GL_LESS:
            GL_KOS_DEPTH_FUNC = PVR_DEPTHCMP_GEQUAL;
            break;

        case GL_LEQUAL:
            GL_KOS_DEPTH_FUNC = PVR_DEPTHCMP_GREATER;
            break;

        case GL_GREATER:
            GL_KOS_DEPTH_FUNC = PVR_DEPTHCMP_LEQUAL;
            break;

        case GL_GEQUAL:
            GL_KOS_DEPTH_FUNC = PVR_DEPTHCMP_LESS;
            break;

        default:
            GL_KOS_DEPTH_FUNC = (func & 0x0F);
    }
}

GLvoid APIENTRY glDepthMask(GLboolean flag) {
    GL_KOS_DEPTH_WRITE = !flag;
}

//====================================================================================================//
//== Culling functions ==//

GLvoid APIENTRY glFrontFace(GLenum mode) {
    switch(mode) {
        case GL_CW:
        case GL_CCW:
            GL_KOS_FACE_FRONT = mode;
            break;
    }
}

GLvoid APIENTRY glCullFace(GLenum mode) {
    switch(mode) {
        case GL_FRONT:
        case GL_BACK:
        case GL_FRONT_AND_BACK:
            GL_KOS_CULL_FUNC = mode;
            break;
    }
}

//====================================================================================================//
//== Vertex Attributes Submission Functions ==//

//== Vertex Color Submission ==//

GLvoid APIENTRY glColor1ui(GLuint argb) {
    GL_KOS_VERTEX_COLOR = argb;
}

GLvoid APIENTRY glColor4ub(GLubyte r, GLubyte  g, GLubyte b, GLubyte a) {
    GL_KOS_VERTEX_COLOR = a << 24 | r << 16 | g << 8 | b;
}

GLvoid APIENTRY glColor3f(GLfloat r, GLfloat g, GLfloat b) {
    GL_KOS_VERTEX_COLOR = PVR_PACK_COLOR(1.0f, r, g, b);
}

GLvoid APIENTRY glColor3fv(const GLfloat *rgb) {
    GL_KOS_VERTEX_COLOR = PVR_PACK_COLOR(1.0f, rgb[0], rgb[1], rgb[2]);
}

GLvoid APIENTRY glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    GL_KOS_VERTEX_COLOR = PVR_PACK_COLOR(a, r, g, b);
}

GLvoid APIENTRY glColor4fv(const GLfloat *rgba) {
    GL_KOS_VERTEX_COLOR = PVR_PACK_COLOR(rgba[3], rgba[0], rgba[1], rgba[2]);
}

//== Texture Coordinate Submission ==//

GLvoid APIENTRY glTexCoord2f(GLfloat u, GLfloat v) {
    GL_KOS_VERTEX_UV[0] = u;
    GL_KOS_VERTEX_UV[1] = v;
}

GLvoid APIENTRY glTexCoord2fv(const GLfloat *uv) {
    GL_KOS_VERTEX_UV[0] = uv[0];
    GL_KOS_VERTEX_UV[1] = uv[1];
}

//== Vertex Position Submission Functions ==//

GLvoid APIENTRY(*glVertex3f)(GLfloat, GLfloat, GLfloat);

GLvoid APIENTRY(*glVertex3fv)(GLfloat *);

GLvoid APIENTRY glVertex2f(GLfloat x, GLfloat y) {
    return _glKosVertex3ft(x, y, 0.0f);
}

GLvoid APIENTRY glVertex2fv(const GLfloat *xy) {
    return _glKosVertex3ft(xy[0], xy[1], 0.0f);
}

GLAPI void APIENTRY glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) {
    pvr_vertex_t * v = _glKosVertexBufPointer();
    
    v[0].z = v[3].z = 0;
    
    mat_trans_single3_nomod(x1, y1, v[0].z, v[0].x, v[0].y, v[0].z);
    mat_trans_single3_nomod(x2, y2, v[3].z, v[3].x, v[3].y, v[3].z);
    
    v[0].argb = v[1].argb = v[2].argb = v[3].argb = GL_KOS_VERTEX_COLOR;
    v[0].flags = v[1].flags = v[2].flags = PVR_CMD_VERTEX;
    v[3].flags = PVR_CMD_VERTEX_EOL;
    
    v[1].x = v[0].x;
    v[1].y = v[3].y;
    v[1].z = v[3].z;
    
    v[2].x = v[3].x;
    v[2].y = v[0].y;
    v[2].z = v[0].z;
    
    _glKosVertexBufAdd(4);
    
    GL_KOS_VERTEX_COUNT += 4;    
}

GLAPI void APIENTRY glRectfv(const GLfloat *v1, const GLfloat *v2) {
    pvr_vertex_t * v = _glKosVertexBufPointer();
    
    v[0].z = v[3].z = 0;
    
    mat_trans_single3_nomod(v1[0], v1[1], v[0].z, v[0].x, v[0].y, v[0].z);
    mat_trans_single3_nomod(v2[0], v2[1], v[3].z, v[3].x, v[3].y, v[3].z);
    
    v[0].argb = v[1].argb = v[2].argb = v[3].argb = GL_KOS_VERTEX_COLOR;
    v[0].flags = v[1].flags = v[2].flags = PVR_CMD_VERTEX;
    v[3].flags = PVR_CMD_VERTEX_EOL;
    
    v[1].x = v[0].x;
    v[1].y = v[3].y;
    v[1].z = v[3].z;
    
    v[2].x = v[3].x;
    v[2].y = v[0].y;
    v[2].z = v[0].z;
    
    _glKosVertexBufAdd(4);
    
    GL_KOS_VERTEX_COUNT += 4;    
}

GLAPI void APIENTRY glRecti(GLint x1, GLint y1, GLint x2, GLint y2) {
    return glRectf((GLfloat)x1, (GLfloat)y1, (GLfloat)x2, (GLfloat)y2);
}

GLAPI void APIENTRY glRectiv(const GLint *v1, const GLint *v2) {
    return glRectfv((GLfloat *)v1, (GLfloat *)v2);
}

GLvoid APIENTRY glKosVertex2f(GLfloat x, GLfloat y) {
    pvr_vertex_t *v = _glKosVertexBufPointer();

    v->x = x;
    v->y = y;
    v->z = 10;
    v->u = GL_KOS_VERTEX_UV[0];
    v->v = GL_KOS_VERTEX_UV[1];
    v->argb  = GL_KOS_VERTEX_COLOR;

    _glKosVertexBufIncrement();

    ++GL_KOS_VERTEX_COUNT;
}

GLvoid APIENTRY glKosVertex2fv(const GLfloat *xy) {
    pvr_vertex_t *v = _glKosVertexBufPointer();

    v->x = xy[0];
    v->y = xy[1];
    v->z = 10;
    v->u = GL_KOS_VERTEX_UV[0];
    v->v = GL_KOS_VERTEX_UV[1];
    v->argb  = GL_KOS_VERTEX_COLOR;

    _glKosVertexBufIncrement();

    ++GL_KOS_VERTEX_COUNT;
}

//====================================================================================================//
//== GL Begin / End ==//

GLvoid APIENTRY glBegin(GLenum mode) {
    _glKosMatrixApplyRender();

    _glKosArrayBufReset();

    _glKosEnabledTexture2D() ? _glKosCompileHdrTx() : _glKosCompileHdr();

    GL_KOS_VERTEX_MODE = mode;
    GL_KOS_VERTEX_COUNT = 0;

    if(mode == GL_POINTS) {
        glVertex3f = _glKosVertex3fp;
        glVertex3fv = _glKosVertex3fpv;
    }
    else if(_glKosEnabledNearZClip()
            && _glKosEnabledLighting()) {
        glVertex3f = _glKosVertex3flc;
        glVertex3fv = _glKosVertex3flcv;
    }
    else if(_glKosEnabledLighting()) {
        glVertex3f = _glKosVertex3fl;
        glVertex3fv = _glKosVertex3flv;
    }
    else if(_glKosEnabledNearZClip()) {
        glVertex3f = _glKosVertex3fc;
        glVertex3fv = _glKosVertex3fcv;
    }
    else {
        glVertex3f = _glKosVertex3ft;
        glVertex3fv = _glKosVertex3ftv;
    }
}

GLvoid APIENTRY glEnd() {
    if(_glKosEnabledNearZClip()) { /* Z-Clipping Enabled */
        if(_glKosEnabledLighting()) {
            _glKosVertexComputeLighting(_glKosClipBufAddress(), GL_KOS_VERTEX_COUNT);

            _glKosMatrixLoadRender();
        }

        GLuint cverts;
        pvr_vertex_t *v = _glKosVertexBufPointer();

        switch(GL_KOS_VERTEX_MODE) {
            case GL_TRIANGLES:
                cverts = _glKosClipTriangles(_glKosClipBufAddress(), v, GL_KOS_VERTEX_COUNT);
                _glKosTransformClipBuf(v, cverts);
                _glKosVertexBufAdd(cverts);
                break;

            case GL_TRIANGLE_STRIP:
                cverts = _glKosClipTriangleStrip(_glKosClipBufAddress(), v, GL_KOS_VERTEX_COUNT);
                _glKosTransformClipBuf(v, cverts);
                _glKosVertexBufAdd(cverts);
                break;

            case GL_QUADS:
                cverts = _glKosClipQuads(_glKosClipBufAddress(), v, GL_KOS_VERTEX_COUNT);
                _glKosTransformClipBuf(v, cverts);
                _glKosVertexBufAdd(cverts);
                break;
        }

        _glKosClipBufReset();
    }
    else { /* No Z-Clipping Enabled */
        if(_glKosEnabledLighting())
            _glKosVertexComputeLighting((pvr_vertex_t *)_glKosVertexBufPointer() - GL_KOS_VERTEX_COUNT, GL_KOS_VERTEX_COUNT);

        switch(GL_KOS_VERTEX_MODE) {
            case GL_TRIANGLES:
                _glKosFlagsSetTriangle();
                break;

            case GL_TRIANGLE_STRIP:
                _glKosFlagsSetTriangleStrip();
                break;

            case GL_QUADS:
                _glKosFlagsSetQuad();
                break;
        }
    }
}

//====================================================================================================//
//== Misc. functions ==//

/* Clamp X to [MIN,MAX]: */
#define CLAMP( X, MIN, MAX )  ( (X)<(MIN) ? (MIN) : ((X)>(MAX) ? (MAX) : (X)) )

/* Setup the hardware user clip rectangle.

   The minimum clip rectangle is a 32x32 area which is dependent on the tile
   size use by the tile accelerator. The PVR swithes off rendering to tiles
   outside or inside the defined rectangle dependant upon the 'clipmode'
   bits in the polygon header.

   Clip rectangles therefore must have a size that is some multiple of 32.

    glScissor(0, 0, 32, 32) allows only the 'tile' in the lower left
    hand corner of the screen to be modified and glScissor(0, 0, 0, 0)
    disallows modification to all 'tiles' on the screen.
*/
GLvoid APIENTRY glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
    pvr_cmd_tclip_t *c = _glKosVertexBufPointer();

    GLint miny, maxx, maxy;
    GLsizei gl_scissor_width = CLAMP(width, 0, vid_mode->width);
    GLsizei gl_scissor_height = CLAMP(height, 0, vid_mode->height);

    /* force the origin to the lower left-hand corner of the screen */
    miny = (vid_mode->height - gl_scissor_height) - y;
    maxx = (gl_scissor_width + x);
    maxy = (gl_scissor_height + miny);

    /* load command structure while mapping screen coords to TA tiles */
    c->flags = PVR_CMD_USERCLIP;
    c->d1 = c->d2 = c->d3 = 0;
    c->sx = CLAMP(x / 32, 0, vid_mode->width / 32);
    c->sy = CLAMP(miny / 32, 0, vid_mode->height / 32);
    c->ex = CLAMP((maxx / 32) - 1, 0, vid_mode->width / 32);
    c->ey = CLAMP((maxy / 32) - 1, 0, vid_mode->height / 32);

    _glKosVertexBufIncrement();
}

GLvoid APIENTRY glHint(GLenum target, GLenum mode) {
    switch(target) {
        case GL_PERSPECTIVE_CORRECTION_HINT:
            if(mode == GL_NICEST)
                GL_KOS_SUPERSAMPLE = 1;
            else
                GL_KOS_SUPERSAMPLE = 0;

            break;
    }

}

//====================================================================================================//
//== Internal API Vertex Submission functions ==//

GLvoid _glKosVertex3fs(GLfloat x, GLfloat y, GLfloat z) {
    pvr_vertex_t *v = _glKosVertexBufPointer();

    mat_trans_single3_nomod(x, y, z, v->x, v->y, v->z);

    v->u = GL_KOS_VERTEX_UV[0];
    v->v = GL_KOS_VERTEX_UV[1];

    _glKosVertexBufIncrement();

    ++GL_KOS_VERTEX_COUNT;
}

GLvoid _glKosVertex3fsv(GLfloat *xyz) {
    pvr_vertex_t *v = _glKosVertexBufPointer();

    mat_trans_single3_nomod(xyz[0], xyz[1], xyz[2], v->x, v->y, v->z);

    v->u = GL_KOS_VERTEX_UV[0];
    v->v = GL_KOS_VERTEX_UV[1];

    _glKosVertexBufIncrement();

    ++GL_KOS_VERTEX_COUNT;
}

GLvoid _glKosVertex3ft(GLfloat x, GLfloat y, GLfloat z) {
    pvr_vertex_t *v = _glKosVertexBufPointer();

    mat_trans_single3_nomod(x, y, z, v->x, v->y, v->z);

    v->u = GL_KOS_VERTEX_UV[0];
    v->v = GL_KOS_VERTEX_UV[1];
    v->argb  = GL_KOS_VERTEX_COLOR;

    _glKosVertexBufIncrement();

    ++GL_KOS_VERTEX_COUNT;
}

GLvoid _glKosVertex3ftv(GLfloat *xyz) {
    pvr_vertex_t *v = _glKosVertexBufPointer();

    mat_trans_single3_nomod(xyz[0], xyz[1], xyz[2], v->x, v->y, v->z);

    v->u = GL_KOS_VERTEX_UV[0];
    v->v = GL_KOS_VERTEX_UV[1];
    v->argb  = GL_KOS_VERTEX_COLOR;

    _glKosVertexBufIncrement();

    ++GL_KOS_VERTEX_COUNT;
}

GLvoid _glKosVertex3fc(GLfloat x, GLfloat y, GLfloat z) {
    pvr_vertex_t *v = _glKosClipBufPointer();

    v->x = x;
    v->y = y;
    v->z = z;
    v->u = GL_KOS_VERTEX_UV[0];
    v->v = GL_KOS_VERTEX_UV[1];
    v->argb  = GL_KOS_VERTEX_COLOR;

    _glKosClipBufIncrement();

    ++GL_KOS_VERTEX_COUNT;
}

GLvoid _glKosVertex3fcv(GLfloat *xyz) {
    pvr_vertex_t *v = _glKosClipBufPointer();

    v->x = xyz[0];
    v->y = xyz[1];
    v->z = xyz[2];
    v->u = GL_KOS_VERTEX_UV[0];
    v->v = GL_KOS_VERTEX_UV[1];
    v->argb  = GL_KOS_VERTEX_COLOR;

    _glKosClipBufIncrement();

    ++GL_KOS_VERTEX_COUNT;
}

/* GL_POINTS */

inline GLvoid _glKosVertex3fpa(GLfloat x, GLfloat y, GLfloat z) {
    pvr_vertex_t *v = _glKosVertexBufPointer();

    mat_trans_single3_nomod(x, y, z, v->x, v->y, v->z);

    v->argb  = GL_KOS_VERTEX_COLOR;
    v->flags = PVR_CMD_VERTEX;

    _glKosVertexBufIncrement();
}

inline GLvoid _glKosVertex3fpb(GLfloat x, GLfloat y, GLfloat z) {
    pvr_vertex_t *v = _glKosVertexBufPointer();

    mat_trans_single3_nomod(x, y, z, v->x, v->y, v->z);

    v->argb  = GL_KOS_VERTEX_COLOR;
    v->flags = PVR_CMD_VERTEX_EOL;

    _glKosVertexBufIncrement();
}

GLvoid _glKosVertex3fp(GLfloat x, GLfloat y, GLfloat z) {
    _glKosVertex3fpa(x - GL_KOS_POINT_SIZE, y - GL_KOS_POINT_SIZE, z);
    _glKosVertex3fpa(x + GL_KOS_POINT_SIZE, y - GL_KOS_POINT_SIZE, z);
    _glKosVertex3fpa(x - GL_KOS_POINT_SIZE, y + GL_KOS_POINT_SIZE, z);
    _glKosVertex3fpb(x + GL_KOS_POINT_SIZE, y + GL_KOS_POINT_SIZE, z);
}

GLvoid _glKosVertex3fpv(GLfloat *xyz) {
    _glKosVertex3fpa(xyz[0] - GL_KOS_POINT_SIZE, xyz[1] - GL_KOS_POINT_SIZE, xyz[2]);
    _glKosVertex3fpa(xyz[0] + GL_KOS_POINT_SIZE, xyz[1] - GL_KOS_POINT_SIZE, xyz[2]);
    _glKosVertex3fpa(xyz[0] - GL_KOS_POINT_SIZE, xyz[1] + GL_KOS_POINT_SIZE, xyz[2]);
    _glKosVertex3fpb(xyz[0] + GL_KOS_POINT_SIZE, xyz[1] + GL_KOS_POINT_SIZE, xyz[2]);
}

GLvoid _glKosTransformClipBuf(pvr_vertex_t *v, GLuint verts) {
    register float __x  __asm__("fr12");
    register float __y  __asm__("fr13");
    register float __z  __asm__("fr14");

    while(verts--) {
        __x = v->x;
        __y = v->y;
        __z = v->z;

        mat_trans_fv12();

        v->x = __x;
        v->y = __y;
        v->z = __z;
        ++v;
    }
}

static inline GLvoid _glKosVertexSwap(pvr_vertex_t *v1, pvr_vertex_t *v2) {
    pvr_vertex_t tmp = *v1;
    *v1 = *v2;
    *v2 = * &tmp;
}

static inline GLvoid _glKosFlagsSetQuad() {
    pvr_vertex_t *v = (pvr_vertex_t *)_glKosVertexBufPointer() - GL_KOS_VERTEX_COUNT;
    GLuint i;

    for(i = 0; i < GL_KOS_VERTEX_COUNT; i += 4) {
        _glKosVertexSwap(v + 2, v + 3);
        v->flags = (v + 1)->flags = (v + 2)->flags = PVR_CMD_VERTEX;
        (v + 3)->flags = PVR_CMD_VERTEX_EOL;
        v += 4;
    }
}

static inline GLvoid _glKosFlagsSetTriangle() {
    pvr_vertex_t *v = (pvr_vertex_t *)_glKosVertexBufPointer() - GL_KOS_VERTEX_COUNT;
    GLuint i;

    for(i = 0; i < GL_KOS_VERTEX_COUNT; i += 3) {
        v->flags = (v + 1)->flags = PVR_CMD_VERTEX;
        (v + 2)->flags = PVR_CMD_VERTEX_EOL;
        v += 3;
    }
}

static inline GLvoid _glKosFlagsSetTriangleStrip() {
    pvr_vertex_t *v = (pvr_vertex_t *)_glKosVertexBufPointer() - GL_KOS_VERTEX_COUNT;
    GLuint i;

    for(i = 0; i < GL_KOS_VERTEX_COUNT - 1; i++) {
        v->flags = PVR_CMD_VERTEX;
        v++;
    }

    v->flags = PVR_CMD_VERTEX_EOL;
}

//====================================================================================================//
//== GL KOS PVR Header Parameter Compilation Functions ==//

static inline GLvoid _glKosApplyDepthFunc() {
    if(_glKosEnabledDepthTest())
        GL_KOS_POLY_CXT.depth.comparison = GL_KOS_DEPTH_FUNC;
    else
        GL_KOS_POLY_CXT.depth.comparison = PVR_DEPTHCMP_ALWAYS;

    GL_KOS_POLY_CXT.depth.write = GL_KOS_DEPTH_WRITE;
}

static inline GLvoid _glKosApplyScissorFunc() {
    if(_glKosEnabledScissorTest())
        GL_KOS_POLY_CXT.gen.clip_mode = PVR_USERCLIP_INSIDE;
}

static inline GLvoid _glKosApplyFogFunc() {
    if(_glKosEnabledFog())
        GL_KOS_POLY_CXT.gen.fog_type = PVR_FOG_TABLE;
}

static inline GLvoid _glKosApplyCullingFunc() {
    if(_glKosEnabledCulling()) {
        if(GL_KOS_CULL_FUNC == GL_BACK) {
            if(GL_KOS_FACE_FRONT == GL_CW)
                GL_KOS_POLY_CXT.gen.culling = PVR_CULLING_CCW;
            else
                GL_KOS_POLY_CXT.gen.culling = PVR_CULLING_CW;
        }
        else if(GL_KOS_CULL_FUNC == GL_FRONT) {
            if(GL_KOS_FACE_FRONT == GL_CCW)
                GL_KOS_POLY_CXT.gen.culling = PVR_CULLING_CCW;
            else
                GL_KOS_POLY_CXT.gen.culling = PVR_CULLING_CW;
        }
    }
    else
        GL_KOS_POLY_CXT.gen.culling = PVR_CULLING_NONE;
}

static inline GLvoid _glKosApplyBlendFunc() {
    if(_glKosEnabledBlend()) {
        GL_KOS_POLY_CXT.blend.src = (GL_KOS_BLEND_FUNC & 0xF0) >> 4;
        GL_KOS_POLY_CXT.blend.dst = (GL_KOS_BLEND_FUNC & 0x0F);
    }
}

GLvoid _glKosCompileHdr() {
    pvr_poly_hdr_t *hdr = _glKosVertexBufPointer();

    pvr_poly_cxt_col(&GL_KOS_POLY_CXT, _glKosList() * 2);

    GL_KOS_POLY_CXT.gen.shading = GL_KOS_SHADE_FUNC;

    _glKosApplyDepthFunc();

    _glKosApplyScissorFunc();

    _glKosApplyFogFunc();

    _glKosApplyCullingFunc();

    _glKosApplyBlendFunc();

    pvr_poly_compile(hdr, &GL_KOS_POLY_CXT);

    _glKosVertexBufIncrement();
}

GLvoid _glKosCompileHdrT(GL_TEXTURE_OBJECT *tex) {
    pvr_poly_hdr_t *hdr = _glKosVertexBufPointer();

    pvr_poly_cxt_txr(&GL_KOS_POLY_CXT,
                     _glKosList() * 2,
                     tex->color,
                     tex->width,
                     tex->height,
                     tex->data,
                     tex->filter);

    GL_KOS_POLY_CXT.gen.shading = GL_KOS_SHADE_FUNC;

    _glKosApplyDepthFunc();

    _glKosApplyScissorFunc();

    _glKosApplyFogFunc();

    _glKosApplyCullingFunc();

    _glKosApplyBlendFunc();

    GL_KOS_POLY_CXT.txr.uv_clamp    = tex->uv_clamp;
    GL_KOS_POLY_CXT.txr.mipmap      = tex->mip_map ? 1 : 0;
    GL_KOS_POLY_CXT.txr.mipmap_bias = PVR_MIPBIAS_NORMAL;

    if(_glKosEnabledBlend())
        GL_KOS_POLY_CXT.txr.env = tex->env;

    pvr_poly_compile(hdr, &GL_KOS_POLY_CXT);

    if(GL_KOS_SUPERSAMPLE)
        hdr->mode2 |= GL_PVR_SAMPLE_SUPER << PVR_TA_SUPER_SAMPLE_SHIFT;

    _glKosVertexBufIncrement();
}

//====================================================================================================//
//== Internal GL KOS API State Functions ==//

GLuint _glKosBlendSrcFunc() {
    switch((GL_KOS_BLEND_FUNC & 0xF0) >> 4) {
        case PVR_BLEND_ONE:
            return GL_ONE;

        case PVR_BLEND_ZERO:
            return GL_ZERO;

        case PVR_BLEND_DESTCOLOR:
            return GL_DST_COLOR;

        case PVR_BLEND_SRCALPHA:
            return GL_SRC_ALPHA;

        case PVR_BLEND_DESTALPHA:
            return GL_DST_ALPHA;

        case PVR_BLEND_INVSRCALPHA:
            return GL_ONE_MINUS_SRC_ALPHA;

        case PVR_BLEND_INVDESTALPHA:
            return GL_ONE_MINUS_DST_ALPHA;

        case PVR_BLEND_INVDESTCOLOR:
            return GL_ONE_MINUS_DST_COLOR;
    }

    return 0;
}

GLuint _glKosBlendDstFunc() {
    switch(GL_KOS_BLEND_FUNC & 0xF) {
        case PVR_BLEND_ONE:
            return GL_ONE;

        case PVR_BLEND_ZERO:
            return GL_ZERO;

        case PVR_BLEND_DESTCOLOR:
            return GL_DST_COLOR;

        case PVR_BLEND_SRCALPHA:
            return GL_SRC_ALPHA;

        case PVR_BLEND_DESTALPHA:
            return GL_DST_ALPHA;

        case PVR_BLEND_INVSRCALPHA:
            return GL_ONE_MINUS_SRC_ALPHA;

        case PVR_BLEND_INVDESTALPHA:
            return GL_ONE_MINUS_DST_ALPHA;

        case PVR_BLEND_INVDESTCOLOR:
            return GL_ONE_MINUS_DST_COLOR;
    }

    return 0;
}

GLubyte _glKosCullFaceMode() {
    return GL_KOS_CULL_FUNC;
}

GLubyte _glKosCullFaceFront() {
    return GL_KOS_FACE_FRONT;
}

GLuint _glKosDepthFunc() {
    switch(GL_KOS_DEPTH_FUNC) {
        case PVR_DEPTHCMP_GEQUAL:
            return GL_LESS;

        case PVR_DEPTHCMP_GREATER:
            return GL_LEQUAL;

        case PVR_DEPTHCMP_LEQUAL:
            return GL_GREATER;

        case PVR_DEPTHCMP_LESS:
            return GL_GEQUAL;

        default:
            return GL_NEVER + GL_KOS_DEPTH_FUNC;
    }
}

GLubyte _glKosDepthMask() {
    return !GL_KOS_DEPTH_WRITE;
}