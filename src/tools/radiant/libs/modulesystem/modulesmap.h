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

#if !defined(INCLUDED_MODULESYSTEM_MODULESMAP_H)
#define INCLUDED_MODULESYSTEM_MODULESMAP_H

#include "modulesystem.h"
#include "string/StringTokeniser.h"
#include <map>
#include <set>
#include <iostream>

template<typename Type>
class ModulesMap: public Modules<Type>
{
		typedef std::map<std::string, Module*> modules_t;
		modules_t m_modules;
	public:
		~ModulesMap ()
		{
			for (modules_t::iterator i = m_modules.begin(); i != m_modules.end(); ++i) {
				(*i).second->release();
			}
		}

		typedef modules_t::const_iterator iterator;

		iterator begin () const
		{
			return m_modules.begin();
		}
		iterator end () const
		{
			return m_modules.end();
		}

		void insert (const std::string& name, Module& module)
		{
			module.capture();
			if (globalModuleServer().getError()) {
				module.release();
				globalModuleServer().setError(false);
			} else {
				m_modules.insert(modules_t::value_type(name, &module));
			}
		}

		Type* find (const std::string& name)
		{
			modules_t::iterator i = m_modules.find(name);
			if (i != m_modules.end()) {
				return static_cast<Type*> (Module_getTable(*(*i).second));
			}
			return 0;
		}

		Type* findModule (const std::string& name)
		{
			return find(name);
		}
		void foreachModule (const typename Modules<Type>::Visitor& visitor)
		{
			for (modules_t::iterator i = m_modules.begin(); i != m_modules.end(); ++i) {
				visitor.visit((*i).first, *static_cast<const Type*> (Module_getTable(*(*i).second)));
			}
		}
};

template<typename Type>
class InsertModules: public ModuleServer::Visitor
{
		ModulesMap<Type>& m_modules;
	public:
		InsertModules (ModulesMap<Type>& modules) :
			m_modules(modules)
		{
		}

		void visit (const std::string& name, Module& module) const
		{
			m_modules.insert(name, module);
		}
};

// The ModulesRef class appears to be a container for a certain subset of Modules specified in
// its constructor
template<typename Type>
class ModulesRef
{
		ModulesMap<Type> m_modules;
	public:
		ModulesRef (const std::string& names)
		{
			if (!globalModuleServer().getError()) {
				if (names == "*") {
					InsertModules<Type> visitor(m_modules);
					globalModuleServer().foreachModule(typename Type::Name(), typename Type::Version(), visitor);
				} else {
					StringTokeniser tokeniser(names);
					for (;;) {
						const std::string name = tokeniser.getToken();
						if (name.empty())
							break;
						Module* module = globalModuleServer().findModule(typename Type::Name(),
								typename Type::Version(), name);
						// Module not found in the global module server
						if (module == 0) {
							globalModuleServer().setError(true);
							break;
						} else {
							m_modules.insert(name, *module);
						}
					}
				}
			}
		}
		ModulesMap<Type>& get ()
		{
			return m_modules;
		}
};

#endif
