/**
 * @file r_state.h
 * @brief
 */

/*
All original material Copyright (C) 2002-2011 UFO: Alien Invasion.

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

#ifndef R_STATE_H
#define R_STATE_H

#include "r_program.h"
#include "r_material.h"
#include "r_framebuffer.h"
#include "r_light.h"
#include "r_image.h"

/* vertex arrays are used for many things */
#define GL_ARRAY_LENGTH_CHUNK 4096
#ifdef HAVE_GLES
extern const vec2_t default_texcoords2[6];
#endif
extern const vec2_t default_texcoords[4];


/** @brief texunits maintain multitexture state */
typedef struct gltexunit_s {
	qboolean enabled;	/**< GL_TEXTURE_2D on / off */
	GLenum texture;		/**< e.g. GL_TEXTURE0 */
	GLint texnum;		/**< e.g 123 */
	GLenum texenv;		/**< e.g. GL_MODULATE */
	GLfloat *texcoord_array;
	/* Size of the array above - it's dynamically reallocated */
	int array_size;
} gltexunit_t;

#define MAX_GL_TEXUNITS		6

/* these are defined for convenience */
#define texunit_0	        r_state.texunits[0]
#define texunit_1	        r_state.texunits[1]
#define texunit_2	        r_state.texunits[2]
#define texunit_3	        r_state.texunits[3]
#define texunit_4	        r_state.texunits[4]
#define texunit_5	        r_state.texunits[5]

#define texunit_diffuse			texunit_0
#define texunit_lightmap		texunit_1
#define texunit_deluxemap		texunit_2
#define texunit_normalmap		texunit_3
#define texunit_glowmap			texunit_4
#define texunit_specularmap		texunit_5
#define texunit_roughnessmap	texunit_2


#define DOWNSAMPLE_PASSES	5
#define DOWNSAMPLE_SCALE	2

#define fbo_screen			NULL
#define fbo_render			r_state.renderBuffer
#define fbo_bloom0			r_state.bloomBuffer0
#define fbo_bloom1			r_state.bloomBuffer1

#define default_program		NULL

struct mAliasMesh_s;

typedef struct rstate_s {
	qboolean fullscreen;

	/* arrays */
	GLfloat *vertex_array_3d;
	GLshort *vertex_array_2d;
	GLfloat *color_array;
	GLfloat *normal_array;
	GLfloat *tangent_array;
	GLfloat *next_vertex_array_3d;
	GLfloat *next_normal_array;
	GLfloat *next_tangent_array;

	/* Size of all arrays above - it's dynamically reallocated */
	int array_size;

	/* multitexture texunits */
	gltexunit_t texunits[MAX_GL_TEXUNITS];

	/* texunit in use */
	gltexunit_t *active_texunit;

	/* framebuffer objects*/
	r_framebuffer_t *renderBuffer;
	r_framebuffer_t *bloomBuffer0;
	r_framebuffer_t *bloomBuffer1;
	r_framebuffer_t *buffers0[DOWNSAMPLE_PASSES];
	r_framebuffer_t *buffers1[DOWNSAMPLE_PASSES];
	r_framebuffer_t *buffers2[DOWNSAMPLE_PASSES];
	qboolean frameBufferObjectsInitialized;
	const r_framebuffer_t *activeFramebuffer;

	/* shaders */
	r_shader_t shaders[MAX_SHADERS];
	r_program_t programs[MAX_PROGRAMS];
	r_program_t *world_program;
	r_program_t *model_program;
	r_program_t *warp_program;
	r_program_t *geoscape_program;
	r_program_t *convolve_program;
	r_program_t *combine2_program;
	r_program_t *atmosphere_program;
	r_program_t *simple_glow_program;
	r_program_t *active_program;

	/* blend function */
	GLenum blend_src, blend_dest;

	const material_t *active_material;

	/* states */
	qboolean shell_enabled;
	qboolean blend_enabled;
	qboolean color_array_enabled;
	qboolean alpha_test_enabled;
	qboolean stencil_test_enabled;
	qboolean lighting_enabled;
	qboolean warp_enabled;
	qboolean fog_enabled;
	qboolean blur_enabled;
	qboolean glowmap_enabled;
	qboolean draw_glow_enabled;
	qboolean dynamic_lighting_enabled;
	qboolean specularmap_enabled;
	qboolean roughnessmap_enabled;
	qboolean animation_enabled;
	qboolean renderbuffer_enabled; /**< renderbuffer vs screen as render target*/

	const struct image_s *active_normalmap;
} rstate_t;

extern rstate_t r_state;

void R_SetDefaultState(void);
void R_Setup2D(void);
void R_Setup3D(void);
void R_ReallocateStateArrays(int size);
void R_ReallocateTexunitArray(gltexunit_t * texunit, int size);

void R_TexEnv(GLenum value);
void R_BlendFunc(GLenum src, GLenum dest);

qboolean R_SelectTexture(gltexunit_t *texunit);

void R_BindTextureDebug(GLuint texnum, const char *file, int line, const char *function);
#define R_BindTexture(tn) R_BindTextureDebug(tn, __FILE__, __LINE__, __PRETTY_FUNCTION__)
void R_BindTextureForTexUnit(GLuint texnum, gltexunit_t *texunit);
void R_BindLightmapTexture(GLuint texnum);
void R_BindDeluxemapTexture(GLuint texnum);
void R_BindNormalmapTexture(GLuint texnum);
void R_BindBuffer(GLenum target, GLenum type, GLuint id);
void R_BindArray(GLenum target, GLenum type, const void *array);
void R_BindDefaultArray(GLenum target);
void R_EnableStencilTest(qboolean enable);
void R_EnableTexture(gltexunit_t *texunit, qboolean enable);
void R_EnableBlend(qboolean enable);
void R_EnableAlphaTest(qboolean enable);
void R_EnableColorArray(qboolean enable);
qboolean R_EnableLighting(r_program_t *program, qboolean enable);
void R_EnableBumpmap(const struct image_s *normalmap);
void R_EnableWarp(r_program_t *program, qboolean enable);
void R_EnableBlur(r_program_t *program, qboolean enable, r_framebuffer_t *source, r_framebuffer_t *dest, int dir);
void R_EnableShell(qboolean enable);
void R_EnableFog(qboolean enable);
void R_EnableDrawAsGlow(qboolean enable);
void R_EnableGlowMap(const struct image_s *image);
void R_EnableSpecularMap(const struct image_s *image, qboolean enable);
void R_EnableRoughnessMap(const struct image_s *image, qboolean enable);
void R_SetupSpotLight(int index, const light_t *light);
void R_DisableSpotLight(int index);
void R_EnableAnimation(const struct mAliasMesh_s *mesh, float backlerp, qboolean enable);

void R_UseMaterial (const material_t *material);

#endif
