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

#if !defined(INCLUDED_TEXTUREENTRY_H)
#define INCLUDED_TEXTUREENTRY_H

#include <gtk/gtkentry.h>
#include <gtk/gtkliststore.h>
#include "gtkutil/idledraw.h"

#include "generic/static.h"
#include "signal/isignal.h"
#include "shaderlib.h"

#include "sidebar/texturebrowser.h"

template<typename StringList>
class EntryCompletion
{
		GtkListStore* m_store;
		IdleDraw m_idleUpdate;
	public:
		EntryCompletion () :
			m_store(0), m_idleUpdate(UpdateCaller(*this))
		{
		}

		void connect (GtkEntry* entry)
		{
			if (m_store == 0) {
				m_store = gtk_list_store_new(1, G_TYPE_STRING);

				fill();

				StringList().connect(IdleDraw::QueueDrawCaller(m_idleUpdate));
			}

			GtkEntryCompletion* completion = gtk_entry_completion_new();
			gtk_entry_set_completion(entry, completion);
			gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(m_store));
			gtk_entry_completion_set_text_column(completion, 0);
		}

		void append (const std::string& string)
		{
			GtkTreeIter iter;
			gtk_list_store_append(m_store, &iter);
			gtk_list_store_set(m_store, &iter, 0, string.c_str(), -1);
		}
		typedef MemberCaller1<EntryCompletion, const std::string&, &EntryCompletion::append> AppendCaller;

		void fill ()
		{
			StringList().forEach(AppendCaller(*this));
		}

		void clear ()
		{
			if (m_store != 0)
				gtk_list_store_clear(m_store);
		}

		void update ()
		{
			clear();
			fill();
		}
		typedef MemberCaller<EntryCompletion, &EntryCompletion::update> UpdateCaller;
};

class TextureNameList
{
	public:
		void forEach (const ShaderNameCallback& callback) const
		{
			for (GlobalShaderSystem().beginActiveShadersIterator(); !GlobalShaderSystem().endActiveShadersIterator(); GlobalShaderSystem().incrementActiveShadersIterator()) {
				IShader *shader = GlobalShaderSystem().dereferenceActiveShadersIterator();

				const std::string prefix = GlobalTexturePrefix_get();
				if (shader_equal_prefix(shader->getName(), prefix)) {
					callback(shader->getName().substr(prefix.length()));
				}
			}
		}
		void connect (const SignalHandler& update) const
		{
			GlobalTextureBrowser().addActiveShadersChangedCallback(update);
		}
};

typedef Static<EntryCompletion<TextureNameList> > GlobalTextureEntryCompletion;

#endif
