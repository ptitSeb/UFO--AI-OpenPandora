/**
 * @file r_draw.c
 */

/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "r_local.h"
#include "r_sphere.h"
#include "r_error.h"
#include "r_draw.h"
#include "r_mesh.h"
#include "r_framebuffer.h"
#include "r_program.h"
#include "../cl_console.h"

image_t *shadow;

/* console font */
static image_t *draw_chars;

#define MAX_CHARS 8192

/** @brief GLES doesn't support QUADS, so we have to draw 2 triangles instead of 1 Quads */
#ifdef HAVE_GLES
#define __sommets__ 6
#else
#define __sommets__ 4
#endif

/** @brief Characters are batched per frame and drawn in one shot
 * accumulate coordinates and colors as vertex arrays */
typedef struct char_arrays_s {
	GLfloat texcoords[MAX_CHARS * __sommets__ * 2];
	int texcoord_index;

	GLshort verts[MAX_CHARS * __sommets__ * 2];
	int vert_index;

	GLbyte colors[MAX_CHARS * __sommets__ * 4];
	int color_index;
} char_arrays_t;

static char_arrays_t r_char_arrays;

#define MAX_BATCH_ENTRIES 512

/** @brief array to store batched vertices and colors per frame */
typedef struct batch_arrays_s {
	GLshort verts[MAX_BATCH_ENTRIES * __sommets__ * 2];
	int vert_index;

	GLbyte colors[MAX_BATCH_ENTRIES * __sommets__ * 4];
	int color_index;
} batch_arrays_t;

static batch_arrays_t r_fill_arrays;

#define MAX_BBOX_ENTRIES 256

typedef struct bbox_arrays_s {
	float bboxes[3 * 8 * MAX_BBOX_ENTRIES];
	int bboxes_index;
} bbox_arrays_t;

static bbox_arrays_t r_bbox_array;

#undef __sommets__

/**
 * @brief Loads some textures and init the 3d globe
 * @sa R_Init
 */
void R_DrawInitLocal (void)
{
	shadow = R_FindImage("pics/sfx/shadow", it_effect);
	if (shadow == r_noTexture)
		Com_Printf("Could not find shadow image in game pics/sfx directory!\n");

	draw_chars = R_FindImage("pics/conchars", it_chars);
	if (draw_chars == r_noTexture)
		Com_Error(ERR_FATAL, "Could not find conchars image in game pics directory!");
}

/**
 * @brief Draws one 8*8 graphics character with 0 being transparent.
 * It can be clipped to the top of the screen to allow the console to be
 * smoothly scrolled off.
 */
void R_DrawChar (int x, int y, int num, uint32_t color)
{
	int row, col;
	float frow, fcol;

	num &= 255;

	if ((num & 127) == ' ')		/* space */
		return;

	if (y <= -con_fontHeight)
		return;					/* totally off screen */

	if (r_char_arrays.vert_index >= lengthof(r_char_arrays.verts))
		return;

	row = (int) num >> 4;
	col = (int) num & 15;

	/* 0.0625 => 16 cols (conchars images) */
	frow = row * 0.0625;
	fcol = col * 0.0625;

	memcpy(&r_char_arrays.colors[r_char_arrays.color_index +  0], &color, 4);
	memcpy(&r_char_arrays.colors[r_char_arrays.color_index +  4], &color, 4);
	memcpy(&r_char_arrays.colors[r_char_arrays.color_index +  8], &color, 4);
	memcpy(&r_char_arrays.colors[r_char_arrays.color_index + 12], &color, 4);
#ifdef HAVE_GLES
	memcpy(&r_char_arrays.colors[r_char_arrays.color_index + 16], &color, 4);
	memcpy(&r_char_arrays.colors[r_char_arrays.color_index + 20], &color, 4);
#endif

#ifdef HAVE_GLES
	r_char_arrays.color_index += 24;
#else
	r_char_arrays.color_index += 16;
#endif

	r_char_arrays.texcoords[r_char_arrays.texcoord_index + 0] = fcol;
	r_char_arrays.texcoords[r_char_arrays.texcoord_index + 1] = frow;

	r_char_arrays.texcoords[r_char_arrays.texcoord_index + 2] = fcol + 0.0625;
	r_char_arrays.texcoords[r_char_arrays.texcoord_index + 3] = frow;

	r_char_arrays.texcoords[r_char_arrays.texcoord_index + 4] = fcol + 0.0625;
	r_char_arrays.texcoords[r_char_arrays.texcoord_index + 5] = frow + 0.0625;

#ifdef HAVE_GLES
	r_char_arrays.texcoords[r_char_arrays.texcoord_index + 6] = fcol + 0.0625;
	r_char_arrays.texcoords[r_char_arrays.texcoord_index + 7] = frow + 0.0625;

	r_char_arrays.texcoords[r_char_arrays.texcoord_index + 8] = fcol;
	r_char_arrays.texcoords[r_char_arrays.texcoord_index + 9] = frow + 0.0625;

	r_char_arrays.texcoords[r_char_arrays.texcoord_index +10] = fcol;
	r_char_arrays.texcoords[r_char_arrays.texcoord_index +11] = frow;

	r_char_arrays.texcoord_index += 12;
#else
	r_char_arrays.texcoords[r_char_arrays.texcoord_index + 6] = fcol;
	r_char_arrays.texcoords[r_char_arrays.texcoord_index + 7] = frow + 0.0625;

	r_char_arrays.texcoord_index += 8;
#endif

	r_char_arrays.verts[r_char_arrays.vert_index + 0] = x;
	r_char_arrays.verts[r_char_arrays.vert_index + 1] = y;

	r_char_arrays.verts[r_char_arrays.vert_index + 2] = x + con_fontWidth;
	r_char_arrays.verts[r_char_arrays.vert_index + 3] = y;

	r_char_arrays.verts[r_char_arrays.vert_index + 4] = x + con_fontWidth;
	r_char_arrays.verts[r_char_arrays.vert_index + 5] = y + con_fontHeight;

#ifdef HAVE_GLES
	r_char_arrays.verts[r_char_arrays.vert_index + 6] = x + con_fontWidth;
	r_char_arrays.verts[r_char_arrays.vert_index + 7] = y + con_fontHeight;

	r_char_arrays.verts[r_char_arrays.vert_index + 8] = x;
	r_char_arrays.verts[r_char_arrays.vert_index + 9] = y + con_fontHeight;

	r_char_arrays.verts[r_char_arrays.vert_index +10] = x;
	r_char_arrays.verts[r_char_arrays.vert_index +11] = y;

	r_char_arrays.vert_index += 12;
#else
	r_char_arrays.verts[r_char_arrays.vert_index + 6] = x;
	r_char_arrays.verts[r_char_arrays.vert_index + 7] = y + con_fontHeight;

	r_char_arrays.vert_index += 8;
#endif
}

void R_DrawChars (void)
{
	if (!r_char_arrays.vert_index)
		return;

	R_BindTexture(draw_chars->texnum);

	R_EnableColorArray(qtrue);

	/* alter the array pointers */
	R_BindArray(GL_COLOR_ARRAY, GL_UNSIGNED_BYTE, r_char_arrays.colors);
	R_BindArray(GL_TEXTURE_COORD_ARRAY, GL_FLOAT, r_char_arrays.texcoords);
	glVertexPointer(2, GL_SHORT, 0, r_char_arrays.verts);

#ifdef HAVE_GLES
	glDrawArrays(GL_TRIANGLES, 0, r_char_arrays.vert_index / 2);
#else
	glDrawArrays(GL_QUADS, 0, r_char_arrays.vert_index / 2);
#endif

	refdef.batchCount++;

	r_char_arrays.color_index = 0;
	r_char_arrays.texcoord_index = 0;
	r_char_arrays.vert_index = 0;

	/* and restore them */
	R_BindDefaultArray(GL_TEXTURE_COORD_ARRAY);
	R_BindDefaultArray(GL_VERTEX_ARRAY);
	R_BindDefaultArray(GL_COLOR_ARRAY);

	R_EnableColorArray(qfalse);
}

/**
 * @brief Fills a box of pixels with a single color
 */
void R_DrawFill (int x, int y, int w, int h, const vec4_t color)
{
	const float nx = x * viddef.rx;
	const float ny = y * viddef.ry;
	const float nw = w * viddef.rx;
	const float nh = h * viddef.ry;
	const int r = color[0] * 255.0;
	const int g = color[1] * 255.0;
	const int b = color[2] * 255.0;
	const int a = color[3] * 255.0;
	const uint32_t c = LittleLong((r << 0) + (g << 8) + (b << 16) + (a << 24));

	if (r_fill_arrays.color_index >= lengthof(r_fill_arrays.colors))
		return;

	/* duplicate color data to all 4 verts */
	memcpy(&r_fill_arrays.colors[r_fill_arrays.color_index +  0], &c, 4);
	memcpy(&r_fill_arrays.colors[r_fill_arrays.color_index +  4], &c, 4);
	memcpy(&r_fill_arrays.colors[r_fill_arrays.color_index +  8], &c, 4);
	memcpy(&r_fill_arrays.colors[r_fill_arrays.color_index + 12], &c, 4);
#ifdef HAVE_GLES
	memcpy(&r_fill_arrays.colors[r_fill_arrays.color_index + 16], &c, 4);
	memcpy(&r_fill_arrays.colors[r_fill_arrays.color_index + 20], &c, 4);

	r_fill_arrays.color_index += 24;
#else

	r_fill_arrays.color_index += 16;
#endif

	/* populate verts */
	r_fill_arrays.verts[r_fill_arrays.vert_index + 0] = nx;
	r_fill_arrays.verts[r_fill_arrays.vert_index + 1] = ny;

	r_fill_arrays.verts[r_fill_arrays.vert_index + 2] = nx + nw;
	r_fill_arrays.verts[r_fill_arrays.vert_index + 3] = ny;

	r_fill_arrays.verts[r_fill_arrays.vert_index + 4] = nx + nw;
	r_fill_arrays.verts[r_fill_arrays.vert_index + 5] = ny + nh;

#ifdef HAVE_GLES
	r_fill_arrays.verts[r_fill_arrays.vert_index + 6] = nx + nw;
	r_fill_arrays.verts[r_fill_arrays.vert_index + 7] = ny + nh;

	r_fill_arrays.verts[r_fill_arrays.vert_index + 8] = nx;
	r_fill_arrays.verts[r_fill_arrays.vert_index + 9] = ny + nh;

	r_fill_arrays.verts[r_fill_arrays.vert_index +10] = nx;
	r_fill_arrays.verts[r_fill_arrays.vert_index +11] = ny;

	r_fill_arrays.vert_index += 12;
#else
	r_fill_arrays.verts[r_fill_arrays.vert_index + 6] = nx;
	r_fill_arrays.verts[r_fill_arrays.vert_index + 7] = ny + nh;

	r_fill_arrays.vert_index += 8;
#endif

	/** @todo this shouldn't be here, but only called once at the end of the frame
	 * but this is needed here because a) we don't want them to get rendered on top of the console
	 * and b) the ui stuff relies on the order of these renderings */
	R_DrawFills();
}

void R_DrawFills (void)
{
	if (!r_fill_arrays.vert_index)
		return;

	R_EnableTexture(&texunit_diffuse, qfalse);

	R_EnableColorArray(qtrue);

	/* alter the array pointers */
	R_BindArray(GL_COLOR_ARRAY, GL_UNSIGNED_BYTE, r_fill_arrays.colors);
	glVertexPointer(2, GL_SHORT, 0, r_fill_arrays.verts);

#ifdef HAVE_GLES
	glDrawArrays(GL_TRIANGLES, 0, r_fill_arrays.vert_index / 2);
#else
	glDrawArrays(GL_QUADS, 0, r_fill_arrays.vert_index / 2);
#endif

	refdef.batchCount++;

	/* and restore them */
	R_BindDefaultArray(GL_VERTEX_ARRAY);
	R_BindDefaultArray(GL_COLOR_ARRAY);

	R_EnableColorArray(qfalse);

	R_EnableTexture(&texunit_diffuse, qtrue);

	r_fill_arrays.vert_index = r_fill_arrays.color_index = 0;

	R_Color(NULL);
}

/**
 * @brief Uploads image data
 * @param[in] name The name of the texture to use for this data
 * @param[in] frame The frame data that is uploaded
 * @param[in] width The width of the texture
 * @param[in] height The height of the texture
 * @return the texture number of the uploaded images
 */
int R_UploadData (const char *name, byte *frame, int width, int height)
{
	image_t *img;
	unsigned *scaled;
	int scaledWidth, scaledHeight;
	int samples = r_config.gl_compressed_solid_format ? r_config.gl_compressed_solid_format : r_config.gl_solid_format;
	int i, c;
	byte *scan;

	R_GetScaledTextureSize(width, height, &scaledWidth, &scaledHeight);

	img = R_FindImage(name, it_pic);
	if (img == r_noTexture)
		Com_Error(ERR_FATAL, "Could not find the searched image: %s", name);

	/* scan the texture for any non-255 alpha */
	c = scaledWidth * scaledHeight;
	/* set scan to the first alpha byte */
	for (i = 0, scan = ((byte *) frame) + 3; i < c; i++, scan += 4) {
		if (*scan != 255) {
			samples = r_config.gl_compressed_alpha_format ? r_config.gl_compressed_alpha_format : r_config.gl_alpha_format;
			break;
		}
	}

	if (scaledWidth != width || scaledHeight != height) {  /* whereas others need to be scaled */
		scaled = (unsigned *)Mem_PoolAllocExt(scaledWidth * scaledHeight * sizeof(unsigned), qfalse, vid_imagePool, 0);
		R_ScaleTexture((unsigned *)frame, width, height, scaled, scaledWidth, scaledHeight);
	} else {
		scaled = (unsigned *)frame;
	}

	R_BindTexture(img->texnum);
	if (img->upload_width == scaledWidth && img->upload_height == scaledHeight) {
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, scaledWidth, scaledHeight, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
	} else {
		/* Reallocate the texture */
		img->width = width;
		img->height = height;
		img->upload_width = scaledWidth;
		img->upload_height = scaledHeight;
#ifdef HAVE_GLES
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, scaledWidth, scaledHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
#else
		glTexImage2D(GL_TEXTURE_2D, 0, samples, scaledWidth, scaledHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
#endif
	}
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	R_CheckError();

	if (scaled != (unsigned *)frame)
		Mem_Free(scaled);

	return img->texnum;
}

/**
 * @brief Bind and draw a texture
 * @param[in] texnum The texture id (already uploaded of course)
 * @param[in] x normalized x value on the screen
 * @param[in] y normalized y value on the screen
 * @param[in] w normalized width value
 * @param[in] h normalized height value
 */
void R_DrawTexture (int texnum, int x, int y, int w, int h)
{
	const vec2_t vertexes[] = {{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}};

	R_BindTexture(texnum);
	R_DrawImageArray(default_texcoords, vertexes, NULL);
}

/**
 * @brief Draws an image or parts of it
 * @param x X position to draw the image to
 * @param y Y position to draw the image to
 * @param[in] image Pointer to the imlage to display
 */
void R_DrawImage (float x, float y, const image_t *image)
{
	if (!image)
		return;

	R_DrawTexture(image->texnum, x * viddef.rx, y * viddef.ry, image->width * viddef.rx, image->height * viddef.ry);
}

void R_DrawStretchImage (float x, float y, int w, int h, const image_t *image)
{
	if (!image)
		return;

	R_DrawTexture(image->texnum, x * viddef.rx, y * viddef.ry, w * viddef.rx, h * viddef.ry);
}

const image_t *R_DrawImageArray (const vec2_t texcoords[4], const vec2_t verts[4], const image_t *image)
{
	/* alter the array pointers */
	glVertexPointer(2, GL_FLOAT, 0, verts);
	R_BindArray(GL_TEXTURE_COORD_ARRAY, GL_FLOAT, texcoords);

	if (image != NULL)
		R_BindTexture(image->texnum);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	refdef.batchCount++;

	/* and restore them */
	R_BindDefaultArray(GL_TEXTURE_COORD_ARRAY);
	R_BindDefaultArray(GL_VERTEX_ARRAY);

	return image;
}

/**
 * @brief Draws a rect to the screen. Also has support for stippled rendering of the rect.
 *
 * @param[in] x X-position of the rect
 * @param[in] y Y-position of the rect
 * @param[in] w Width of the rect
 * @param[in] h Height of the rect
 * @param[in] color RGBA color of the rect
 * @param[in] lineWidth Line strength in pixel of the rect
 * @param[in] pattern Specifies a 16-bit integer whose bit pattern determines
 * which fragments of a line will be drawn when the line is rasterized.
 * Bit zero is used first; the default pattern is all 1's
 * @note The stipple factor is @c 2 for this function
 */
void R_DrawRect (int x, int y, int w, int h, const vec4_t color, float lineWidth, int pattern)
{
	const float nx = x * viddef.rx;
	const float ny = y * viddef.ry;
	const float nw = w * viddef.rx;
	const float nh = h * viddef.ry;
	const vec2_t points[] = { { nx, ny }, { nx + nw, ny }, { nx + nw, ny + nh }, { nx, ny + nh } };

	R_Color(color);

	glDisable(GL_TEXTURE_2D);
	glLineWidth(lineWidth);
#ifndef HAVE_GLES
	glLineStipple(2, pattern);
	glEnable(GL_LINE_STIPPLE);
#endif

	glVertexPointer(2, GL_FLOAT, 0, points);
	glDrawArrays(GL_LINE_LOOP, 0, 4);
	R_BindDefaultArray(GL_VERTEX_ARRAY);

	refdef.batchCount++;

	glEnable(GL_TEXTURE_2D);
	glLineWidth(1.0f);
#ifndef HAVE_GLES
	glDisable(GL_LINE_STIPPLE);
#endif

	R_Color(NULL);
}

void R_DrawCircle (float radius, const vec4_t color, float thickness, const vec3_t shift)
{
	vec3_t points[16];
	const size_t steps = lengthof(points);
	unsigned int i;

	glEnable(GL_LINE_SMOOTH);
	glLineWidth(thickness);

	R_Color(color);

	for (i = 0; i < steps; i++) {
		const float a = 2.0f * M_PI * (float) i / (float) steps;
		VectorSet(points[i], shift[0] + radius * cos(a), shift[1] + radius * sin(a), shift[2]);
	}

	R_BindArray(GL_VERTEX_ARRAY, GL_FLOAT, points);
	glDrawArrays(GL_LINE_LOOP, 0, steps);
	R_BindDefaultArray(GL_VERTEX_ARRAY);

	refdef.batchCount++;

	R_Color(NULL);

	glLineWidth(1.0f);
	glDisable(GL_LINE_SMOOTH);
}

#define MAX_LINEVERTS 256

static inline void R_Draw2DArray (int points, int *verts, GLenum mode)
{
	int i;

	/* fit it on screen */
	if (points > MAX_LINEVERTS * 2)
		points = MAX_LINEVERTS * 2;

	/* set vertex array pointer */
	glVertexPointer(2, GL_SHORT, 0, r_state.vertex_array_2d);

	for (i = 0; i < points * 2; i += 2) {
		r_state.vertex_array_2d[i] = verts[i] * viddef.rx;
		r_state.vertex_array_2d[i + 1] = verts[i + 1] * viddef.ry;
	}

	glDisable(GL_TEXTURE_2D);
	glDrawArrays(mode, 0, points);
	glEnable(GL_TEXTURE_2D);
	glVertexPointer(3, GL_FLOAT, 0, r_state.vertex_array_3d);

	refdef.batchCount++;
}

/**
 * @brief 2 dimensional line strip
 * @sa R_DrawLineLoop
 */
void R_DrawLineStrip (int points, int *verts)
{
	R_Draw2DArray(points, verts, GL_LINE_STRIP);
}

/**
 * @sa R_DrawLineStrip
 */
void R_DrawLineLoop (int points, int *verts)
{
	R_Draw2DArray(points, verts, GL_LINE_LOOP);
}

/**
 * @brief Draws one line with only one start and one end point
 * @sa R_DrawLineStrip
 */
void R_DrawLine (int *verts, float thickness)
{
	if (thickness > 0.0)
		glLineWidth(thickness);

	R_Draw2DArray(2, verts, GL_LINES);

	if (thickness > 0.0)
		glLineWidth(1.0);
}

/**
 * @sa R_DrawLineStrip
 * @sa R_DrawLineLoop
 */
void R_DrawPolygon (int points, int *verts)
{
	R_Draw2DArray(points, verts, GL_TRIANGLE_FAN);
}

typedef struct {
	int x;
	int y;
	int width;
	int height;
} rect_t;

#define MAX_CLIPRECT 16

static rect_t clipRect[MAX_CLIPRECT];

static int currentClipRect = 0;

/**
 * @brief Compute the intersection of 2 rect
 * @param[in] a A rect
 * @param[in] b A rect
 * @param[out] out The intersection rect
 */
static void R_RectIntersection (const rect_t* a, const rect_t* b, rect_t* out)
{
	out->x = (a->x > b->x) ? a->x : b->x;
	out->y = (a->y > b->y) ? a->y : b->y;
	out->width = ((a->x + a->width < b->x + b->width) ? a->x + a->width : b->x + b->width) - out->x;
	out->height = ((a->y + a->height < b->y + b->height) ? a->y + a->height : b->y + b->height) - out->y;
	if (out->width < 0)
		out->width = 0;
	if (out->height < 0)
		out->height = 0;
}

/**
 * @brief Force to draw only on a rect
 * @note Don't forget to call @c R_EndClipRect
 * @sa R_PopClipRect
 */
void R_PushClipRect (int x, int y, int width, int height)
{
	const int depth = currentClipRect;
	assert(depth < MAX_CLIPRECT);

	if (depth == 0) {
		clipRect[depth].x = x * viddef.rx;
		clipRect[depth].y = (viddef.virtualHeight - (y + height)) * viddef.ry;
		clipRect[depth].width = width * viddef.rx;
		clipRect[depth].height = height * viddef.ry;
	} else {
		rect_t rect;
		rect.x = x * viddef.rx;
		rect.y = (viddef.virtualHeight - (y + height)) * viddef.ry;
		rect.width = width * viddef.rx;
		rect.height = height * viddef.ry;
		R_RectIntersection(&clipRect[depth - 1], &rect, &clipRect[depth]);
	}

	glScissor(clipRect[depth].x, clipRect[depth].y, clipRect[depth].width, clipRect[depth].height);

	if (currentClipRect == 0)
		glEnable(GL_SCISSOR_TEST);
	currentClipRect++;
}

/**
 * @sa R_PushClipRect
 */
void R_PopClipRect (void)
{
	assert(currentClipRect > 0);
	currentClipRect--;
	if (currentClipRect == 0)
		glDisable(GL_SCISSOR_TEST);
	else {
		const int depth = currentClipRect - 1;
		glScissor(clipRect[depth].x, clipRect[depth].y, clipRect[depth].width, clipRect[depth].height);
	}
}

/**
 * @brief "Clean up" the depth buffer into a rect
 * @note we use a big value (but not too big) to set the depth buffer, then it is not really a clean up
 * @todo can we fix bigZ with a value come from glGet?
 */
void R_CleanupDepthBuffer (int x, int y, int width, int height)
{
	const int nx = x * viddef.rx;
	const int ny = y * viddef.ry;
	const int nwidth = width * viddef.rx;
	const int nheight = height * viddef.ry;
	const GLboolean hasDepthTest = glIsEnabled(GL_DEPTH_TEST);
	const GLdouble bigZ = 2000;
	const vec3_t points [] = { { nx, ny, bigZ }, { nx + nwidth, ny, bigZ }, { nx + nwidth, ny + nheight, bigZ }, { nx, ny + nheight, bigZ } };

	GLint depthFunc;
	glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);

	/* we want to overwrite depth buffer not to have his constraints */
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_ALWAYS);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	R_BindArray(GL_VERTEX_ARRAY, GL_FLOAT, points);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	R_BindDefaultArray(GL_VERTEX_ARRAY);

	refdef.batchCount++;

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	if (!hasDepthTest)
		glDisable(GL_DEPTH_TEST);
	glDepthFunc(depthFunc);
}

/**
 * @brief Compute the bounding box for an entity out of the mins, maxs
 * @sa R_DrawBoundingBox
 */
static void R_ComputeBoundingBox (const vec3_t mins, const vec3_t maxs, vec3_t bbox[8])
{
	int i;

	/* compute a full bounding box */
	for (i = 0; i < 8; i++) {
		bbox[i][0] = (i & 1) ? mins[0] : maxs[0];
		bbox[i][1] = (i & 2) ? mins[1] : maxs[1];
		bbox[i][2] = (i & 4) ? mins[2] : maxs[2];
	}
}

#ifdef HAVE_GLES
#define glIndex		unsigned short
#define GLINDEX		GL_UNSIGNED_SHORT
#else
#define glIndex		unsigned int
#define GLINDEX		GL_UNSIGNED_INT
#endif
void R_DrawBoundingBoxes (void)
{
	int i;
	const int step = 3 * 8;
	const int bboxes = r_bbox_array.bboxes_index / step;
	const glIndex indexes[] = { 2, 1, 0, 1, 4, 5, 1, 7, 3, 2, 7, 6, 2, 4, 0 };
	const glIndex indexes2[] = { 4, 6, 7 };

	if (!r_bbox_array.bboxes_index)
		return;

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	R_Color(NULL);

	for (i = 0; i < bboxes; i++) {
		const float *bbox = &r_bbox_array.bboxes[i * step];
		R_BindArray(GL_VERTEX_ARRAY, GL_FLOAT, bbox);
		/* Draw top and sides */
		glDrawElements(GL_TRIANGLE_FAN, 15, GLINDEX, indexes);
		/* Draw bottom */
		glDrawElements(GL_TRIANGLE_FAN, 3, GLINDEX, indexes2);
	}

	R_BindDefaultArray(GL_VERTEX_ARRAY);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	r_bbox_array.bboxes_index = 0;
}

void R_DrawBoundingBoxBatched (const vec3_t absmins, const vec3_t absmaxs)
{
	int i;
	vec3_t bbox[8];
	const size_t max = lengthof(r_bbox_array.bboxes);

	if (r_bbox_array.bboxes_index >= max)
		return;

	R_ComputeBoundingBox(absmins, absmaxs, bbox);

	for (i = 0; i < 8; i++) {
		VectorCopy(bbox[i], &r_bbox_array.bboxes[r_bbox_array.bboxes_index]);
		r_bbox_array.bboxes_index += 3;
	}
}

/**
 * @brief Draws the model bounding box
 * @sa R_EntityComputeBoundingBox
 */
void R_DrawBoundingBox (const vec3_t absmins, const vec3_t absmaxs)
{
	vec3_t bbox[8];
	const glIndex indexes[] = { 2, 1, 0, 1, 4, 5, 1, 7, 3, 2, 7, 6, 2, 4, 0 };
	const glIndex indexes2[] = { 4, 6, 7 };

	R_ComputeBoundingBox(absmins, absmaxs, bbox);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	R_BindArray(GL_VERTEX_ARRAY, GL_FLOAT, bbox);
	/* Draw top and sides */
	glDrawElements(GL_TRIANGLE_STRIP, 15, GLINDEX, indexes);
	/* Draw bottom */
	glDrawElements(GL_TRIANGLE_STRIP, 3, GLINDEX, indexes2);
	R_BindDefaultArray(GL_VERTEX_ARRAY);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

/**
 * @brief Draws the textured box, the caller should bind the texture
 * @sa R_DrawBox
 */
void R_DrawTexturedBox (const vec3_t a0, const vec3_t a1)
{
	const GLfloat texcoords[] = { 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, };
	const vec3_t bbox[] = {
					{ a0[0], a0[1], a0[2] }, { a0[0], a0[1], a1[2] }, { a1[0], a0[1], a1[2] }, { a1[0], a0[1], a0[2] },
					{ a1[0], a1[1], a0[2] }, { a1[0], a1[1], a1[2] }, { a0[0], a1[1], a1[2] }, { a0[0], a1[1], a0[2] },
					{ a0[0], a0[1], a0[2] }, { a0[0], a0[1], a1[2] }, { a1[0], a0[1], a1[2] }, { a1[0], a0[1], a0[2] },
					{ a1[0], a1[1], a0[2] }, { a1[0], a1[1], a1[2] }, { a0[0], a1[1], a1[2] }, { a0[0], a1[1], a0[2] } };
	const glIndex indexes[] = { 0, 1, 2, 1, 2, 3, 4, 5, 6, 6, 7, 4, 2 + 8, 3 + 8, 4 + 8, 2 + 8, 5 + 8, 4 + 8, 6 + 8, 7 + 8,
			0 + 8, 0 + 8, 1 + 8, 6 + 8, };

	R_BindArray(GL_TEXTURE_COORD_ARRAY, GL_FLOAT, texcoords);
	R_BindArray(GL_VERTEX_ARRAY, GL_FLOAT, bbox);

	/* Draw sides only */
	glDrawElements(GL_TRIANGLES, 8 * 3, GLINDEX, indexes);

	R_BindDefaultArray(GL_VERTEX_ARRAY);
	R_BindDefaultArray(GL_TEXTURE_COORD_ARRAY);
}
