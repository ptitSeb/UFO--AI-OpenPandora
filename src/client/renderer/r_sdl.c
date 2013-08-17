/**
 * @file r_sdl.c
 * @brief This file contains SDL specific stuff having to do with the OpenGL refresh
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
#include "r_main.h"
#include "r_sdl.h"
#include "../../ports/system.h"

#ifdef HAVE_GLES
#include "eglport.h"

void myglMultiTexCoord2f( GLenum texture, GLfloat s, GLfloat t )
{
	glMultiTexCoord4f(texture, s, t, 0, 1);
}
#endif

r_sdl_config_t r_sdl_config;

#ifndef _WIN32
static void R_SetSDLIcon (void)
{
#include "../../ports/linux/ufoicon.xbm"
	SDL_Surface *icon;
	SDL_Color color;
	Uint8 *ptr;
	unsigned int i, mask;

	icon = SDL_CreateRGBSurface(SDL_SWSURFACE, ufoicon_width, ufoicon_height, 8, 0, 0, 0, 0);
	if (icon == NULL)
		return;
	SDL_SetColorKey(icon, SDL_SRCCOLORKEY, 0);

	color.r = color.g = color.b = 255;
	SDL_SetColors(icon, &color, 0, 1); /* just in case */
	color.r = color.b = 0;
	color.g = 16;
	SDL_SetColors(icon, &color, 1, 1);

	ptr = (Uint8 *)icon->pixels;
	for (i = 0; i < sizeof(ufoicon_bits); i++) {
		for (mask = 1; mask != 0x100; mask <<= 1) {
			*ptr = (ufoicon_bits[i] & mask) ? 1 : 0;
			ptr++;
		}
	}

	SDL_WM_SetIcon(icon, NULL);
	SDL_FreeSurface(icon);
}
#endif

qboolean Rimp_Init (void)
{
	SDL_version version;
	int attrValue;
	const SDL_VideoInfo* info;
	char videoDriverName[MAX_VAR] = "";

	Com_Printf("\n------- video initialization -------\n");

	OBJZERO(r_sdl_config);

	if (r_driver->string[0] != '\0') {
		Com_Printf("using driver: %s\n", r_driver->string);
		SDL_GL_LoadLibrary(r_driver->string);
	}

	Sys_Setenv("SDL_VIDEO_CENTERED", "1");
	Sys_Setenv("SDL_VIDEO_ALLOW_SCREENSAVER", "0");

	if (SDL_WasInit(SDL_INIT_VIDEO) == 0) {
		if (SDL_Init(SDL_INIT_VIDEO) < 0)
			Com_Error(ERR_FATAL, "Video SDL_Init failed: %s", SDL_GetError());
	}

	SDL_VERSION(&version)
	Com_Printf("SDL version: %i.%i.%i\n", version.major, version.minor, version.patch);

	info = SDL_GetVideoInfo();
	if (info) {
		SDL_VideoInfo videoInfo;
		SDL_PixelFormat pixelFormat;
		SDL_Rect **modes;
		Com_Printf("I: desktop depth: %ibpp\n", info->vfmt->BitsPerPixel);
		r_config.videoMemory = info->video_mem;
		Com_Printf("I: video memory: %i\n", r_config.videoMemory);
		memcpy(&pixelFormat, info->vfmt, sizeof(pixelFormat));
		memcpy(&videoInfo, info, sizeof(videoInfo));
		videoInfo.vfmt = &pixelFormat;
		modes = SDL_ListModes(videoInfo.vfmt, SDL_OPENGL | SDL_FULLSCREEN);
		if (modes) {
			char buf[4096] = "";
			Q_strcat(buf, "I: Available resolutions:", sizeof(buf));
			if (modes == (SDL_Rect **)-1) {
				Com_Printf("%s any resolution is supported\n", buf);
				r_sdl_config.modes = NULL;
			} else {
				int i;
				for (r_sdl_config.numModes = 0; modes[r_sdl_config.numModes]; r_sdl_config.numModes++);

				r_sdl_config.modes = Mem_AllocExt(sizeof(rect_t) * r_sdl_config.numModes, qfalse);
				for (i = 0; i < r_sdl_config.numModes; i++) {
					r_sdl_config.modes[i][0] = modes[i]->w;
					r_sdl_config.modes[i][1] = modes[i]->h;
					const char *modeStr = va(" %ix%i", r_sdl_config.modes[i][0], r_sdl_config.modes[i][1]);
					Q_strcat(buf, modeStr, sizeof(buf));
				}
				Com_Printf("%s (%i)\n", buf, r_sdl_config.numModes);
			}
		} else {
			r_sdl_config.modes = NULL;
			Com_Printf("I: Could not get list of available resolutions\n");
		}
	} else {
		r_config.videoMemory = 0;
	}
	SDL_VideoDriverName(videoDriverName, sizeof(videoDriverName));
	Com_Printf("I: video driver: %s\n", videoDriverName);

	if (!R_SetMode())
		Com_Error(ERR_FATAL, "Video subsystem failed to initialize");

	SDL_WM_SetCaption(GAME_TITLE, GAME_TITLE_LONG);

#ifndef _WIN32
	R_SetSDLIcon(); /* currently uses ufoicon.xbm data */
#endif

	#ifdef HAVE_GLES
	attrValue = eglStencilbits;
	#else
	if (!SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &attrValue))
	#endif
		Com_Printf("I: got %d bits of stencil\n", attrValue);
	#ifdef HAVE_GLES
	attrValue = eglDepthbits;
	#else
	if (!SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &attrValue))
	#endif
		Com_Printf("I: got %d bits of depth buffer\n", attrValue);
	#ifdef HAVE_GLES
	attrValue = 1;
	#else
	if (!SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &attrValue))
	#endif
		Com_Printf("I: got double buffer\n");
	#ifdef HAVE_GLES
	attrValue = eglRedbits;
	#else
	if (!SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &attrValue))
	#endif
		Com_Printf("I: got %d bits for red\n", attrValue);
	#ifdef HAVE_GLES
	attrValue = eglGreenbits;
	#else
	if (!SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &attrValue))
	#endif
		Com_Printf("I: got %d bits for green\n", attrValue);
	#ifdef HAVE_GLES
	attrValue = eglBluebits;
	#else
	if (!SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &attrValue))
	#endif
		Com_Printf("I: got %d bits for blue\n", attrValue);
	#ifdef HAVE_GLES
	attrValue = eglAlphabits;
	#else
	if (!SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &attrValue))
	#endif
		Com_Printf("I: got %d bits for alpha\n", attrValue);
	#ifdef HAVE_GLES
	attrValue = 0;
	#else
	if (!SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &attrValue))
	#endif
		Com_Printf("I: got %d multisample buffers\n", attrValue);

	/* we need this in the renderer because if we issue an vid_restart we have
	 * to set these values again, too */
	SDL_EnableUNICODE(SDL_ENABLE);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	return qtrue;
}

/**
 * @brief Init the SDL window
 */
qboolean R_InitGraphics (const viddefContext_t *context)
{
	uint32_t flags;
	int i;
	SDL_Surface* screen = NULL;

	#ifdef HAVE_GLES
	flags = 0;
	#else
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	/* valid values are between 0 and 4 */
	i = min(4, max(0, context->multisample));
	Com_Printf("I: set multisample buffers to %i\n", i);
	if (i > 0) {
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, i);
	} else {
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
	}

	/* valid values are between 0 and 2 */
	i = min(2, max(0, context->swapinterval));
	Com_Printf("I: set swap control to %i\n", i);
#if SDL_VERSION_ATLEAST(1,3,0)
	SDL_GL_SetSwapInterval(1);
#else
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, i);
#endif

	flags = SDL_OPENGL;
	#endif
	#ifndef PANDORA
	if (context->fullscreen)
	#endif
		flags |= SDL_FULLSCREEN;
	/*flags |= SDL_NOFRAME;*/

	screen = SDL_SetVideoMode(context->width, context->height, 0, flags);
	if (!screen) {
		const char *error = SDL_GetError();
		Com_Printf("SDL SetVideoMode failed: %s\n", error);
		SDL_ClearError();
		return qfalse;
	}
	
	#ifdef HAVE_GLES
	EGL_Open(context->width, context->height);
	#endif

	SDL_ShowCursor(SDL_DISABLE);

	return qtrue;
}

void Rimp_Shutdown (void)
{
	SDL_ShowCursor(SDL_ENABLE);

	if (SDL_WasInit(SDL_INIT_EVERYTHING) == SDL_INIT_VIDEO)
		SDL_Quit();
	else
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
}
