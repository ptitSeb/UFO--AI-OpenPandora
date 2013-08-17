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

#if !defined(INCLUDED_MODEL_H)
#define INCLUDED_MODEL_H

#include "imodel.h"

namespace scene
{
	class Node;
}
class ArchiveFile;
typedef struct picoModule_s picoModule_t;
/** Load a model and return a Node for insertion into a scene graph
 */
scene::Node& loadPicoModel (const picoModule_t* module, ArchiveFile& file);

/** Load a model and return an OpenGLRenderable shared pointer for immediate
 * rendering.
 */
model::IModelPtr loadIModel (const picoModule_t* module, ArchiveFile& file);

extern bool g_showModelNormals;
extern bool g_showModelBoundingBoxes;

#endif
