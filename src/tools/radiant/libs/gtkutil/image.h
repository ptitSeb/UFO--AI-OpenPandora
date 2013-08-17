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

#if !defined(INCLUDED_GTKUTIL_IMAGE_H)
#define INCLUDED_GTKUTIL_IMAGE_H

#include <string>

void BitmapsPath_set (const std::string& path);

typedef struct _GtkWidget GtkWidget;
typedef struct _GdkPixbuf GdkPixbuf;

GdkPixbuf* pixbuf_new_from_file_with_mask (const std::string& filename);

namespace gtkutil
{
	// filename is relative to local bitmaps path
	GtkWidget* getImage (const std::string& filename);

	// Convenience functions to load a local image (from the bitmaps directory)
	// and return a GdkPixBuf for use by certain GTK widgets (e.g. TreeView).
	GdkPixbuf* getLocalPixbuf (const std::string& filename);

	GdkPixbuf* getLocalPixbufWithMask (const std::string& filename);
} // namespace gtkutil

#endif
