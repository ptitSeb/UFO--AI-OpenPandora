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

// OpenGL widget based on GtkGLExt

#include "glwidget.h"
#include "debugging/debugging.h"

#include "igl.h"

#include <gtk/gtkdrawingarea.h>
#include <gtk/gtkglwidget.h>

#include "pointer.h"

void (*GLWidget_sharedContextCreated) () = 0;
void (*GLWidget_sharedContextDestroyed) () = 0;

typedef int* attribs_t;
struct config_t
{
		const char* name;
		attribs_t attribs;
};
typedef const config_t* configs_iterator;

static int config_rgba32[] = { GDK_GL_RGBA, GDK_GL_DOUBLEBUFFER, GDK_GL_BUFFER_SIZE, 24, GDK_GL_ATTRIB_LIST_NONE, };

static int config_rgba[] = { GDK_GL_RGBA, GDK_GL_DOUBLEBUFFER, GDK_GL_BUFFER_SIZE, 16, GDK_GL_ATTRIB_LIST_NONE, };

static const config_t configs[] = { { "colour-buffer = 32bpp, depth-buffer = none", config_rgba32, }, {
		"colour-buffer = 16bpp, depth-buffer = none", config_rgba, } };

static int config_rgba32_depth32[] = { GDK_GL_RGBA, GDK_GL_DOUBLEBUFFER, GDK_GL_BUFFER_SIZE, 24, GDK_GL_DEPTH_SIZE, 32,
		GDK_GL_ATTRIB_LIST_NONE, };

static int config_rgba32_depth24[] = { GDK_GL_RGBA, GDK_GL_DOUBLEBUFFER, GDK_GL_BUFFER_SIZE, 24, GDK_GL_DEPTH_SIZE, 24,
		GDK_GL_ATTRIB_LIST_NONE, };

static int config_rgba32_depth16[] = { GDK_GL_RGBA, GDK_GL_DOUBLEBUFFER, GDK_GL_BUFFER_SIZE, 24, GDK_GL_DEPTH_SIZE, 16,
		GDK_GL_ATTRIB_LIST_NONE, };

static int config_rgba32_depth[] = { GDK_GL_RGBA, GDK_GL_DOUBLEBUFFER, GDK_GL_BUFFER_SIZE, 24, GDK_GL_DEPTH_SIZE, 1,
		GDK_GL_ATTRIB_LIST_NONE, };

static int config_rgba_depth16[] = { GDK_GL_RGBA, GDK_GL_DOUBLEBUFFER, GDK_GL_BUFFER_SIZE, 16, GDK_GL_DEPTH_SIZE, 16,
		GDK_GL_ATTRIB_LIST_NONE, };

static int config_rgba_depth[] = { GDK_GL_RGBA, GDK_GL_DOUBLEBUFFER, GDK_GL_BUFFER_SIZE, 16, GDK_GL_DEPTH_SIZE, 1,
		GDK_GL_ATTRIB_LIST_NONE, };

const config_t configs_with_depth[] = { { "colour-buffer = 32bpp, depth-buffer = 32bpp", config_rgba32_depth32, }, {
		"colour-buffer = 32bpp, depth-buffer = 24bpp", config_rgba32_depth24, }, {
		"colour-buffer = 32bpp, depth-buffer = 16bpp", config_rgba32_depth16, }, {
		"colour-buffer = 32bpp, depth-buffer = auto", config_rgba32_depth, }, {
		"colour-buffer = 16bpp, depth-buffer = 16bpp", config_rgba_depth16, }, {
		"colour-buffer = auto, depth-buffer = auto", config_rgba_depth, }, };

static unsigned int g_context_count = 0;

namespace
{
	GtkWidget* g_shared = 0;
}

#include "glwidget.h"

namespace gtkutil {

// Constructor, pass TRUE to enable depth-buffering
GLWidget::GLWidget(bool zBuffer) :
	_widget(gtk_drawing_area_new()), _zBuffer(zBuffer) {
	g_signal_connect(G_OBJECT(_widget), "hierarchy-changed", G_CALLBACK(onHierarchyChanged), this);
	g_signal_connect(G_OBJECT(_widget), "realize", G_CALLBACK(onRealise), this);
	g_signal_connect(G_OBJECT(_widget), "unrealize", G_CALLBACK(onUnRealise), this);
}

// Operator cast to GtkWidget*, for packing into parent containers
GLWidget::operator GtkWidget*() const {
	return _widget;
}

GdkGLConfig* GLWidget::createGLConfigWithDepth() {
	for (configs_iterator i = configs_with_depth, end = configs_with_depth + 6; i
			!= end; ++i) {
		GdkGLConfig* glconfig = gdk_gl_config_new(i->attribs);

		if (glconfig != NULL) {
			globalOutputStream() << "OpenGL window configuration: " << i->name
					<< "\n";
			return glconfig;
		}
	}

	globalOutputStream()
			<< "OpenGL window configuration: colour-buffer = auto, depth-buffer = auto (fallback)\n";
	return gdk_gl_config_new_by_mode((GdkGLConfigMode) (GDK_GL_MODE_RGBA
			| GDK_GL_MODE_DOUBLE | GDK_GL_MODE_DEPTH));
}

GdkGLConfig* GLWidget::createGLConfig() {
	for (configs_iterator i = configs, end = configs + 2; i != end; ++i) {
		GdkGLConfig* glconfig = gdk_gl_config_new(i->attribs);

		if (glconfig != NULL) {
			globalOutputStream() << "OpenGL window configuration: " << i->name
					<< "\n";
			return glconfig;
		}
	}

	globalOutputStream()
			<< "OpenGL window configuration: colour-buffer = auto, depth-buffer = none\n";
	return gdk_gl_config_new_by_mode((GdkGLConfigMode) (GDK_GL_MODE_RGBA
			| GDK_GL_MODE_DOUBLE));
}

bool GLWidget::makeCurrent(GtkWidget* widget) {
	 GdkGLContext* glcontext = gtk_widget_get_gl_context(widget);
	 GdkGLDrawable* gldrawable = gtk_widget_get_gl_drawable(widget);
	 return static_cast<bool>(gdk_gl_drawable_gl_begin(gldrawable, glcontext));
}

void GLWidget::swapBuffers(GtkWidget* widget) {
	GdkGLDrawable* gldrawable = gtk_widget_get_gl_drawable(widget);
	gdk_gl_drawable_swap_buffers(gldrawable);
}

gboolean GLWidget::onHierarchyChanged(GtkWidget* widget,
		GtkWidget* previous_toplevel, GLWidget* self) {
	if (previous_toplevel == NULL && !gtk_widget_is_gl_capable(widget)) {
		// Create a new GL config structure
		GdkGLConfig* glconfig = (self->_zBuffer) ? createGLConfigWithDepth()
				: createGLConfig();
		ASSERT_MESSAGE(glconfig != NULL, "failed to create OpenGL config");

		gtk_widget_set_gl_capability(widget, glconfig,
				g_shared != NULL ? gtk_widget_get_gl_context(g_shared) : NULL,
				TRUE, GDK_GL_RGBA_TYPE);

		gtk_widget_realize(widget);

		// Shared not set yet?
		if (g_shared == 0) {
			g_shared = widget;
		}
	}

	return FALSE;
}

gint GLWidget::onRealise(GtkWidget* widget, GLWidget* self) {
	if (++g_context_count == 1) {
		g_shared = widget;
		gtk_widget_ref(g_shared);

		makeCurrent(g_shared);
		GlobalOpenGL().contextValid = true;

		GLWidget_sharedContextCreated();
	}

	return FALSE;
}

gint GLWidget::onUnRealise(GtkWidget* widget, GLWidget* self) {
	if (--g_context_count == 0) {
		GlobalOpenGL().contextValid = false;

		GLWidget_sharedContextDestroyed();

		gtk_widget_unref(g_shared);
		g_shared = NULL;
	}

	return FALSE;
}

//GtkWidget* GLWidget::_shared = NULL;
//int GLWidget::_realisedWidgets = 0;

} // namespace gtkutil
