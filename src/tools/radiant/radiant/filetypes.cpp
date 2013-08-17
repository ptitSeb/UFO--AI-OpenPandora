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

#include "filetypes.h"

#include "debugging/debugging.h"

#include "ifiletypes.h"
#include "radiant_i18n.h"

#include "string/string.h"
#include "os/path.h"
#include <vector>
#include <map>

class RadiantFileTypeRegistry: public IFileTypeRegistry
{
	private:
		struct filetype_copy_t
		{
				filetype_copy_t (const std::string& moduleName, const filetype_t& other) :
					m_moduleName(moduleName), m_name(other.name), m_pattern(other.pattern)
				{
				}
				const std::string& getModuleName () const
				{
					return m_moduleName;
				}
				filetype_t getType () const
				{
					return filetype_t(m_name, m_pattern);
				}
				std::string m_moduleName;
				std::string m_name;
				std::string m_pattern;
		};
		typedef std::vector<filetype_copy_t> filetype_list_t;
		std::map<std::string, filetype_list_t> m_typelists;
	public:
		RadiantFileTypeRegistry ()
		{
			addType("*", "*", filetype_t(_("All Files"), "*"));
		}
		void addType (const std::string& moduleType, const std::string& moduleName, filetype_t type)
		{
			m_typelists[moduleType].push_back(filetype_copy_t(moduleName, type));
		}
		void getTypeList (const std::string& moduleType, IFileTypeList* typelist)
		{
			filetype_list_t& list_ref = m_typelists[moduleType];
			for (filetype_list_t::iterator i = list_ref.begin(); i != list_ref.end(); ++i) {
				typelist->addType((*i).getModuleName(), (*i).getType());
			}
		}
};

static RadiantFileTypeRegistry g_patterns;

IFileTypeRegistry* GetFileTypeRegistry ()
{
	return &g_patterns;
}

class SearchFileTypeList: public IFileTypeList
{
		std::string m_moduleName;
		std::string m_pattern;
	public:
		SearchFileTypeList (const std::string& ext) :
			m_moduleName(""), m_pattern("*." + ext)
		{
		}
		void addType (const std::string& moduleName, filetype_t type)
		{
			if (extension_equal(m_pattern, type.pattern)) {
				m_moduleName = moduleName;
			}
		}

		const std::string& getModuleName ()
		{
			return m_moduleName;
		}
};

const std::string findModuleName (IFileTypeRegistry* registry, const std::string& moduleType, const std::string& extension)
{
	SearchFileTypeList search(extension);
	registry->getTypeList(moduleType, &search);
	return search.getModuleName();
}

#include "modulesystem/singletonmodule.h"
#include "modulesystem/moduleregistry.h"

class FiletypesAPI
{
		IFileTypeRegistry* m_filetypes;
	public:
		typedef IFileTypeRegistry Type;
		STRING_CONSTANT(Name, "*");

		FiletypesAPI ()
		{
			m_filetypes = GetFileTypeRegistry();
		}
		IFileTypeRegistry* getTable ()
		{
			return m_filetypes;
		}
};

typedef SingletonModule<FiletypesAPI> FiletypesModule;
typedef Static<FiletypesModule> StaticFiletypesModule;
StaticRegisterModule staticRegisterFiletypes(StaticFiletypesModule::instance());
