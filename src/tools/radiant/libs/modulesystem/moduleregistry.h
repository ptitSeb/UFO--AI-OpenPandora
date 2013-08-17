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

#if !defined(INCLUDED_MODULESYSTEM_MODULEREGISTRY_H)
#define INCLUDED_MODULESYSTEM_MODULEREGISTRY_H

#include "generic/static.h"
#include <list>

// Registerable module interface
class ModuleRegisterable
{
	public:
		virtual ~ModuleRegisterable ()
		{
		}
		virtual void selfRegister () = 0;
		virtual const std::string& getName() const = 0;
};

class ModuleRegistryList
{
		typedef std::list<ModuleRegisterable*> RegisterableModules;
		RegisterableModules m_modules;
	public:
		void addModule (ModuleRegisterable& module)
		{
			m_modules.push_back(&module);
		}
		void registerModules () const
		{
			for (RegisterableModules::const_iterator i = m_modules.begin(); i != m_modules.end(); ++i) {
				(*i)->selfRegister();
			}
		}
		void registerModule (const std::string& name)
		{
			for (RegisterableModules::iterator i = m_modules.begin(); i != m_modules.end(); ++i) {
				if ((*i)->getName() == name) {
					(*i)->selfRegister();
					m_modules.erase(i);
					break;
				}
			}
		}
};

typedef SmartStatic<ModuleRegistryList> StaticModuleRegistryList;

class StaticRegisterModule: public StaticModuleRegistryList
{
	public:
		StaticRegisterModule (ModuleRegisterable& module)
		{
			StaticModuleRegistryList::instance().addModule(module);
		}
};

#endif
