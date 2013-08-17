/*
 Copyright (C) 2001-2006, William Joseph.
 All Rights Reserved.

 This file is part of GtkRadiant.

 GtkRadiant is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 GtkRadiant is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with GtkRadiant; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#if !defined(INCLUDED_IRENDER_H)
#define INCLUDED_IRENDER_H

#include <string>
#include "generic/constant.h"
#include "generic/callbackfwd.h"

// Rendering states to sort by.
// Higher bits have the most effect - slowest state changes should be highest.

const unsigned int RENDER_DEFAULT = 0;
const unsigned int RENDER_LINESTIPPLE = 1 << 0; // glEnable(GL_LINE_STIPPLE)
const unsigned int RENDER_LINESMOOTH = 1 << 1; // glEnable(GL_LINE_SMOOTH)
const unsigned int RENDER_POLYGONSTIPPLE = 1 << 2; // glEnable(GL_POLYGON_STIPPLE)
const unsigned int RENDER_POLYGONSMOOTH = 1 << 3; // glEnable(GL_POLYGON_SMOOTH)
const unsigned int RENDER_ALPHATEST = 1 << 4; // glEnable(GL_ALPHA_TEST)
const unsigned int RENDER_DEPTHTEST = 1 << 5; // glEnable(GL_DEPTH_TEST)
const unsigned int RENDER_DEPTHWRITE = 1 << 6; // glDepthMask(GL_TRUE)
const unsigned int RENDER_COLOURWRITE = 1 << 7; // glColorMask(GL_TRUE; GL_TRUE; GL_TRUE; GL_TRUE)
const unsigned int RENDER_CULLFACE = 1 << 8; // glEnable(GL_CULL_FACE)
const unsigned int RENDER_SCALED = 1 << 9; // glEnable(GL_NORMALIZE)
const unsigned int RENDER_SMOOTH = 1 << 10; // glShadeModel
const unsigned int RENDER_LIGHTING = 1 << 11; // glEnable(GL_LIGHTING)
const unsigned int RENDER_BLEND = 1 << 12; // glEnable(GL_BLEND)
const unsigned int RENDER_OFFSETLINE = 1 << 13; // glEnable(GL_POLYGON_OFFSET_LINE)
const unsigned int RENDER_FILL = 1 << 14; // glPolygonMode
const unsigned int RENDER_COLOURARRAY = 1 << 15; // glEnableClientState(GL_COLOR_ARRAY)
const unsigned int RENDER_COLOURCHANGE = 1 << 16; // render() is allowed to call glColor*()
const unsigned int RENDER_MATERIAL_VCOL = 1 << 17; // material requests per-vertex colour
const unsigned int RENDER_VCOL_INVERT = 1 << 18; // vertex colours should be inverted
const unsigned int RENDER_TEXTURE_2D = 1 << 19; // glEnable(GL_TEXTURE_2D)
const unsigned int RENDER_TEXTURE_CUBEMAP = 1 << 20; // glEnable(GL_TEXTURE_CUBE_MAP)
const unsigned int RENDER_BUMP = 1 << 21;
const unsigned int RENDER_PROGRAM = 1 << 22;
const unsigned int RENDER_SCREEN = 1 << 23;
const unsigned int RENDER_OVERRIDE = 1 << 24;
typedef unsigned int RenderStateFlags;

class AABB;
class Matrix4;

template<typename Element> class BasicVector3;
typedef BasicVector3<float> Vector3;

class Shader;
class Renderable;
typedef Callback1<const Renderable&> RenderableCallback;

const int c_attr_TexCoord0 = 1;
const int c_attr_Tangent = 3;
const int c_attr_Binormal = 4;

/**
 * \brief
 * Interface for objects which can render themselves in OpenGL.
 *
 * This interface is used by the render backend, after renderable objects have
 * first been submitted using the Renderable interface. The backend render()
 * function should contain the OpenGL calls necessary to submit vertex, normal
 * and texture-coordinate data.
 *
 * No GL state changes should occur in render(), other than those specifically
 * allowed by the render flags (such as glColor() if RENDER_COLOURWRITE is set).
 */
class OpenGLRenderable
{
	public:
		virtual ~OpenGLRenderable ()
		{
		}
		/**
		 * \brief
		 * Submit OpenGL render calls.
		 */
		virtual void render (RenderStateFlags state) const = 0;
};

class Matrix4;
class GLTexture;
class ModuleObserver;

#include "math/Vector3.h"

/**
 * A Shader represents a single material which can be rendered in OpenGL, which
 * may correspond to an actual material (Material), a raw colour or a special
 * GL shader.
 *
 * Importantly, a Shader also maintains its own list of OpenGLRenderable objects
 * which use it -- the actual rendering is performed by traversing a list of
 * Shaders and rendering the geometry attached to each one.
 */
class Shader
{
	public:
		virtual ~Shader ()
		{
		}
		virtual void addRenderable (const OpenGLRenderable& renderable, const Matrix4& modelview) = 0;
		virtual void incrementUsed () = 0;
		virtual void decrementUsed () = 0;
		virtual void attach (ModuleObserver& observer) = 0;
		virtual void detach (ModuleObserver& observer) = 0;
		virtual GLTexture& getTexture () const = 0;
		virtual unsigned int getFlags () const = 0;
};

class ShaderCache
{
	public:
		INTEGER_CONSTANT(Version, 1);
		STRING_CONSTANT(Name, "renderstate");

		virtual ~ShaderCache ()
		{
		}

		/** Capture the given shader, increasing its reference count and
		 * returning a pointer to the Shader object. The object must be freed
		 * after use by calling release().
		 *
		 * @param name
		 * The name of the shader to capture.
		 *
		 * @returns
		 * Shader* object corresponding to the given material shader name.
		 */

		virtual Shader* capture (const std::string& name) = 0;

		/** Release a named shader previously captured with capture(). The
		 * reference count is decremented.
		 *
		 * @param name
		 * The name of the shader to release.
		 */

		virtual void release (const std::string& name) = 0;

		/*! Render all Shader objects. */
		virtual void render (RenderStateFlags globalstate, const Matrix4& modelview, const Matrix4& projection,
				const Vector3& viewer = Vector3(0, 0, 0)) = 0;

		virtual void realise () = 0;
		virtual void unrealise () = 0;

		virtual void attachRenderable (const Renderable& renderable) = 0;
		virtual void detachRenderable (const Renderable& renderable) = 0;
		virtual void forEachRenderable (const RenderableCallback& callback) const = 0;
};

#include "modulesystem.h"

template<typename Type>
class GlobalModule;
typedef GlobalModule<ShaderCache> GlobalShaderCacheModule;

template<typename Type>
class GlobalModuleRef;
typedef GlobalModuleRef<ShaderCache> GlobalShaderCacheModuleRef;

inline ShaderCache& GlobalShaderCache ()
{
	return GlobalShaderCacheModule::getTable();
}

#endif
