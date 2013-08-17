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

#if !defined (INCLUDED_ENTITYLIB_H)
#define INCLUDED_ENTITYLIB_H

#include "ireference.h"
#include "debugging/debugging.h"

#include "ientity.h"
#include "irender.h"
#include "iradiant.h"
#include "igl.h"
#include "selectable.h"

#include "generic/callback.h"
#include "math/aabb.h"
#include "undolib.h"
#include "generic/referencecounted.h"
#include "scenelib.h"
#include "container/container.h"
#include "eclasslib.h"

#include <vector>
#include <list>
#include <set>

class EntityFindByClassname: public scene::Graph::Walker
{
		const std::string& m_name;
		Entity*& m_entity;
	public:
		EntityFindByClassname (const std::string& name, Entity*& entity) :
			m_name(name), m_entity(entity)
		{
			m_entity = 0;
		}
		bool pre (const scene::Path& path, scene::Instance& instance) const
		{
			if (m_entity == 0) {
				Entity* entity = Node_getEntity(path.top());
				if (entity != 0 && m_name == entity->getKeyValue("classname")) {
					m_entity = entity;
				}
			}
			return true;
		}
};

inline Entity* Scene_FindEntityByClass (const std::string& name)
{
	Entity* entity;
	GlobalSceneGraph().traverse(EntityFindByClassname(name, entity));
	return entity;
}

inline bool node_is_worldspawn (scene::Node& node)
{
	Entity* entity = Node_getEntity(node);
	return entity != 0 && entity->getKeyValue("classname") == "worldspawn";
}

inline void arrow_draw (const Vector3& origin, const Vector3& direction)
{
	Vector3 up(0, 0, 1);
	Vector3 left(-direction[1], direction[0], 0);

	Vector3 endpoint(origin + direction * 32.0);

	Vector3 tip1(endpoint + direction * (-8.0) + up * (-4.0));
	Vector3 tip2(tip1 + up * 8.0);
	Vector3 tip3(endpoint + direction * (-8.0) + left * (-4.0));
	Vector3 tip4(tip3 + left * 8.0);

	glBegin(GL_LINES);

	glVertex3fv(origin);
	glVertex3fv(endpoint);

	glVertex3fv(endpoint);
	glVertex3fv(tip1);

	glVertex3fv(endpoint);
	glVertex3fv(tip2);

	glVertex3fv(endpoint);
	glVertex3fv(tip3);

	glVertex3fv(endpoint);
	glVertex3fv(tip4);

	glVertex3fv(tip1);
	glVertex3fv(tip3);

	glVertex3fv(tip3);
	glVertex3fv(tip2);

	glVertex3fv(tip2);
	glVertex3fv(tip4);

	glVertex3fv(tip4);
	glVertex3fv(tip1);

	glEnd();
}

class SelectionIntersection;

inline void aabb_testselect (const AABB& aabb, SelectionTest& test, SelectionIntersection& best)
{
	const IndexPointer::index_type indices[24] = { 2, 1, 5, 6, 1, 0, 4, 5, 0, 1, 2, 3, 3, 7, 4, 0, 3, 2, 6, 7, 7, 6, 5,
			4, };

	Vector3 points[8];
	aabb_corners(aabb, points);
	test.TestQuads(VertexPointer(reinterpret_cast<VertexPointer::vector_pointer> (points), sizeof(Vector3)), IndexPointer(
			indices, 24), best);
}

inline void aabb_draw_wire (const Vector3 points[8])
{
	const unsigned char indices[] = { 0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7 };
	glVertexPointer(3, GL_FLOAT, 0, points);
	glDrawElements(GL_LINES, sizeof(indices) / sizeof(indices[0]), GL_UNSIGNED_BYTE, indices);
}

inline void aabb_draw_flatshade (const Vector3 points[8])
{
	glBegin(GL_QUADS);

	glNormal3fv(aabb_normals[0]);
	glVertex3fv(points[2]);
	glVertex3fv(points[1]);
	glVertex3fv(points[5]);
	glVertex3fv(points[6]);

	glNormal3fv(aabb_normals[1]);
	glVertex3fv(points[1]);
	glVertex3fv(points[0]);
	glVertex3fv(points[4]);
	glVertex3fv(points[5]);

	glNormal3fv(aabb_normals[2]);
	glVertex3fv(points[0]);
	glVertex3fv(points[1]);
	glVertex3fv(points[2]);
	glVertex3fv(points[3]);

	glNormal3fv(aabb_normals[3]);
	glVertex3fv(points[0]);
	glVertex3fv(points[3]);
	glVertex3fv(points[7]);
	glVertex3fv(points[4]);

	glNormal3fv(aabb_normals[4]);
	glVertex3fv(points[3]);
	glVertex3fv(points[2]);
	glVertex3fv(points[6]);
	glVertex3fv(points[7]);

	glNormal3fv(aabb_normals[5]);
	glVertex3fv(points[7]);
	glVertex3fv(points[6]);
	glVertex3fv(points[5]);
	glVertex3fv(points[4]);

	glEnd();
}

inline void aabb_draw_wire (const AABB& aabb)
{
	Vector3 points[8];
	aabb_corners(aabb, points);
	aabb_draw_wire(points);
}

inline void aabb_draw_flatshade (const AABB& aabb)
{
	Vector3 points[8];
	aabb_corners(aabb, points);
	aabb_draw_flatshade(points);
}

inline void aabb_draw_textured (const AABB& aabb)
{
	Vector3 points[8];
	aabb_corners(aabb, points);

	glBegin(GL_QUADS);

	glNormal3fv(aabb_normals[0]);
	glTexCoord2fv(aabb_texcoord_topleft);
	glVertex3fv(points[2]);
	glTexCoord2fv(aabb_texcoord_topright);
	glVertex3fv(points[1]);
	glTexCoord2fv(aabb_texcoord_botright);
	glVertex3fv(points[5]);
	glTexCoord2fv(aabb_texcoord_botleft);
	glVertex3fv(points[6]);

	glNormal3fv(aabb_normals[1]);
	glTexCoord2fv(aabb_texcoord_topleft);
	glVertex3fv(points[1]);
	glTexCoord2fv(aabb_texcoord_topright);
	glVertex3fv(points[0]);
	glTexCoord2fv(aabb_texcoord_botright);
	glVertex3fv(points[4]);
	glTexCoord2fv(aabb_texcoord_botleft);
	glVertex3fv(points[5]);

	glNormal3fv(aabb_normals[2]);
	glTexCoord2fv(aabb_texcoord_topleft);
	glVertex3fv(points[0]);
	glTexCoord2fv(aabb_texcoord_topright);
	glVertex3fv(points[1]);
	glTexCoord2fv(aabb_texcoord_botright);
	glVertex3fv(points[2]);
	glTexCoord2fv(aabb_texcoord_botleft);
	glVertex3fv(points[3]);

	glNormal3fv(aabb_normals[3]);
	glTexCoord2fv(aabb_texcoord_topleft);
	glVertex3fv(points[0]);
	glTexCoord2fv(aabb_texcoord_topright);
	glVertex3fv(points[3]);
	glTexCoord2fv(aabb_texcoord_botright);
	glVertex3fv(points[7]);
	glTexCoord2fv(aabb_texcoord_botleft);
	glVertex3fv(points[4]);

	glNormal3fv(aabb_normals[4]);
	glTexCoord2fv(aabb_texcoord_topleft);
	glVertex3fv(points[3]);
	glTexCoord2fv(aabb_texcoord_topright);
	glVertex3fv(points[2]);
	glTexCoord2fv(aabb_texcoord_botright);
	glVertex3fv(points[6]);
	glTexCoord2fv(aabb_texcoord_botleft);
	glVertex3fv(points[7]);

	glNormal3fv(aabb_normals[5]);
	glTexCoord2fv(aabb_texcoord_topleft);
	glVertex3fv(points[7]);
	glTexCoord2fv(aabb_texcoord_topright);
	glVertex3fv(points[6]);
	glTexCoord2fv(aabb_texcoord_botright);
	glVertex3fv(points[5]);
	glTexCoord2fv(aabb_texcoord_botleft);
	glVertex3fv(points[4]);

	glEnd();
}

inline void aabb_draw_solid (const AABB& aabb, RenderStateFlags state)
{
	if (state & RENDER_TEXTURE_2D) {
		aabb_draw_textured(aabb);
	} else {
		aabb_draw_flatshade(aabb);
	}
}

inline void aabb_draw (const AABB& aabb, RenderStateFlags state)
{
	if (state & RENDER_FILL) {
		aabb_draw_solid(aabb, state);
	} else {
		aabb_draw_wire(aabb);
	}
}

class RenderableSolidAABB: public OpenGLRenderable
{
		const AABB& m_aabb;
	public:
		RenderableSolidAABB (const AABB& aabb) :
			m_aabb(aabb)
		{
		}
		void render (RenderStateFlags state) const
		{
			aabb_draw_solid(m_aabb, state);
		}
};

class RenderableWireframeAABB: public OpenGLRenderable
{
		const AABB& m_aabb;
	public:
		RenderableWireframeAABB (const AABB& aabb) :
			m_aabb(aabb)
		{
		}
		void render (RenderStateFlags state) const
		{
			aabb_draw_wire(m_aabb);
		}
};

/// \brief A key/value pair of strings.
///
/// - Notifies observers when value changes - value changes to "" on destruction.
/// - Provides undo support through the global undo system.
class KeyValue: public EntityKeyValue
{
		typedef UnsortedSet<KeyObserver> KeyObservers;

		std::size_t m_refcount;
		KeyObservers m_observers;
		std::string m_string;
		const std::string m_empty;
		ObservedUndoableObject<std::string> m_undo;
		static EntityCreator::KeyValueChangedFunc m_entityKeyValueChanged;
	public:

		KeyValue (const std::string& string, const std::string& empty) :
			m_refcount(0), m_string(string), m_empty(empty), m_undo(m_string, UndoImportCaller(*this))
		{
			notify();
		}
		~KeyValue ()
		{
			ASSERT_MESSAGE(m_observers.empty(), "KeyValue::~KeyValue: observers still attached");
		}

		static void setKeyValueChangedFunc (EntityCreator::KeyValueChangedFunc func)
		{
			m_entityKeyValueChanged = func;
		}

		void IncRef ()
		{
			++m_refcount;
		}
		void DecRef ()
		{
			if (--m_refcount == 0) {
				delete this;
			}
		}

		void instanceAttach (MapFile* map)
		{
			m_undo.instanceAttach(map);
		}
		void instanceDetach (MapFile* map)
		{
			m_undo.instanceDetach(map);
		}

		void attach (const KeyObserver& observer)
		{
			(*m_observers.insert(observer))(get());
		}
		void detach (const KeyObserver& observer)
		{
			observer(m_empty);
			m_observers.erase(observer);
		}
		const std::string& get () const
		{
			if (m_string.empty())
				return m_empty;
			return m_string;
		}
		void assign (const std::string& other)
		{
			if (m_string != other) {
				m_undo.save();
				m_string = other;
				notify();
			}
		}

		void notify ()
		{
			m_entityKeyValueChanged();
			KeyObservers::reverse_iterator i = m_observers.rbegin();
			while (i != m_observers.rend()) {
				(*i++)(get());
			}
		}

		void importState (const std::string& string)
		{
			m_string = string;

			notify();
		}
		typedef MemberCaller1<KeyValue, const std::string&, &KeyValue::importState> UndoImportCaller;
};

/// \brief An unsorted list of key/value pairs.
///
/// - Notifies observers when a pair is inserted or removed.
/// - Provides undo support through the global undo system.
/// - New keys are appended to the end of the list.
class EntityKeyValues: public Entity
{
	public:
		typedef KeyValue Value;

	private:
		static EntityCreator::KeyValueChangedFunc m_entityKeyValueChanged;
		EntityClass* m_eclass;

		typedef KeyValue* KeyValuePtr;
		// A key value pair using a dynamically allocated value
		typedef std::pair<std::string, KeyValuePtr> KeyValuePair;

		// The unsorted list of KeyValue pairs
		typedef std::vector<KeyValuePair> KeyValues;

		KeyValues m_keyValues;

		typedef UnsortedSet<Observer*> Observers;
		Observers m_observers;

		ObservedUndoableObject<KeyValues> m_undo;
		bool m_instanced;

		bool m_observerMutex;

		void notifyInsert (const std::string& key, Value& value)
		{
			m_observerMutex = true;
			for (Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
				(*i)->onKeyInsert(key, value);
			}
			m_observerMutex = false;
		}
		void notifyErase (const std::string& key, Value& value)
		{
			m_observerMutex = true;
			for (Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
				(*i)->onKeyErase(key, value);
			}
			m_observerMutex = false;
		}
		void forEachKeyValue_notifyInsert ()
		{
			for (KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
				notifyInsert((*i).first, *(*i).second);
			}
		}
		void forEachKeyValue_notifyErase ()
		{
			for (KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
				notifyErase((*i).first, *(*i).second);
			}
		}

		void insert (const std::string& key, const KeyValuePtr& keyValue)
		{
			KeyValues::iterator i = m_keyValues.insert(m_keyValues.end(), KeyValues::value_type(key, keyValue));
			notifyInsert(key, *(*i).second);

			if (m_instanced) {
				(*i).second->instanceAttach(m_undo.map());
			}
		}

		void insert (const std::string& key, const std::string& value)
		{
			KeyValues::iterator i = find(key);
			if (i != m_keyValues.end()) {
				(*i).second->assign(value);
			} else {
				m_undo.save();
				insert(key, KeyValuePtr(new KeyValue(value, EntityClass_valueForKey(*m_eclass, key))));
			}
		}

		void erase (KeyValues::iterator i)
		{
			if (m_instanced) {
				(*i).second->instanceDetach(m_undo.map());
			}

			std::string key = i->first;
			KeyValuePtr value = i->second;
			m_keyValues.erase(i);
			notifyErase(key, *value);
		}

		void erase (const std::string& key)
		{
			KeyValues::iterator i = find(key);
			if (i != m_keyValues.end()) {
				m_undo.save();
				erase(i);
			}
		}

		KeyValues::const_iterator find (const std::string& key) const
		{
			for (KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); i++) {
				if (i->first == key) {
					return i;
				}
			}
			// Not found
			return m_keyValues.end();
		}

		KeyValues::iterator find (const std::string& key)
		{
			for (KeyValues::iterator i = m_keyValues.begin(); i != m_keyValues.end(); i++) {
				if (i->first == key) {
					return i;
				}
			}
			// Not found
			return m_keyValues.end();
		}

	public:
		bool m_isContainer;

		EntityKeyValues (EntityClass* eclass) :
			m_eclass(eclass), m_undo(m_keyValues, UndoImportCaller(*this)), m_instanced(false), m_observerMutex(false),
					m_isContainer(!eclass->fixedsize)
		{
		}
		EntityKeyValues (const EntityKeyValues& other) :
			Entity(other), m_eclass(&other.getEntityClass()), m_undo(m_keyValues, UndoImportCaller(*this)),
					m_instanced(false), m_observerMutex(false), m_isContainer(other.m_isContainer)
		{
			for (KeyValues::const_iterator i = other.m_keyValues.begin(); i != other.m_keyValues.end(); ++i) {
				insert((*i).first, (*i).second->get());
			}
		}
		~EntityKeyValues ()
		{
			for (KeyValues::iterator i = m_keyValues.begin(); i != m_keyValues.end(); i++) {
				delete i->second;
			}
			m_keyValues.clear();

			ASSERT_MESSAGE(m_observers.empty(), "EntityKeyValues::~EntityKeyValues: observers still attached");
		}

		static void setKeyValueChangedFunc (EntityCreator::KeyValueChangedFunc func)
		{
			m_entityKeyValueChanged = func;
			KeyValue::setKeyValueChangedFunc(func);
		}

		void importState (const KeyValues& keyValues)
		{
			for (KeyValues::iterator i = m_keyValues.begin(); i != m_keyValues.end();) {
				erase(i++);
			}

			for (KeyValues::const_iterator i = keyValues.begin(); i != keyValues.end(); ++i) {
				insert(i->first, i->second);
			}
			m_entityKeyValueChanged();
		}
		typedef MemberCaller1<EntityKeyValues, const KeyValues&, &EntityKeyValues::importState> UndoImportCaller;

		void attach (Observer& observer)
		{
			ASSERT_MESSAGE(!m_observerMutex, "observer cannot be attached during iteration");
			m_observers.insert(&observer);
			for (KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
				observer.onKeyInsert(i->first, *(*i).second);
			}
		}
		void detach (Observer& observer)
		{
			ASSERT_MESSAGE(!m_observerMutex, "observer cannot be detached during iteration");
			m_observers.erase(&observer);
			for (KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
				observer.onKeyErase(i->first, *(*i).second);
			}
		}

		void forEachKeyValue_instanceAttach (MapFile* map)
		{
			for (KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
				(*i).second->instanceAttach(map);
			}
		}
		void forEachKeyValue_instanceDetach (MapFile* map)
		{
			for (KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
				(*i).second->instanceDetach(map);
			}
		}

		void instanceAttach (MapFile* map)
		{
			GlobalRadiant().getCounter(counterEntities).increment();

			m_instanced = true;
			forEachKeyValue_instanceAttach(map);
			m_undo.instanceAttach(map);
		}
		void instanceDetach (MapFile* map)
		{
			GlobalRadiant().getCounter(counterEntities).decrement();

			m_undo.instanceDetach(map);
			forEachKeyValue_instanceDetach(map);
			m_instanced = false;
		}

		// entity
		EntityClass& getEntityClass () const
		{
			return *m_eclass;
		}
		void forEachKeyValue (Visitor& visitor) const
		{
			for (KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
				visitor.visit((*i).first, (*i).second->get());
			}
		}

		/** Set a keyvalue on the entity.
		 */

		void setKeyValue (const std::string& key, const std::string& value)
		{
			if (value.empty()) {
				erase(key);
				m_entityKeyValueChanged();
			} else {
				insert(key, value);
			}
		}

		/**
		 * @brief Searches the value for a given property key, returns default key if not found.
		 * @param[in] key The property key to get the value for
		 * @return The value for the given property key or the default value
		 */
		std::string getKeyValue (const std::string& key) const
		{
			// Lookup the key in the map
			KeyValues::const_iterator i = find(key);

			// If key is found, return it, otherwise lookup the default value on
			// the entity class
			if (i != m_keyValues.end()) {
				return i->second->get();
			} else {
				return EntityClass_valueForKey(*m_eclass, key);
			}
		}

		/**
		 * @brief Add missing mandatory key/value-pairs to actual entity.
		 */
		void addMandatoryKeyValues ()
		{
			for (EntityClassAttributes::const_iterator i = m_eclass->m_attributes.begin(); i
					!= m_eclass->m_attributes.end(); ++i) {
				if (i->second.mandatory && find(i->first) == m_keyValues.end()) {
					this->setKeyValue(i->first, m_eclass->getDefaultForAttribute(i->first));
				}
			}
		}

		bool isContainer () const
		{
			return m_isContainer;
		}
};

/// \brief A Resource reference with a controlled lifetime.
/// \brief The resource is released when the ResourceReference is destroyed.
class ResourceReference
{
		std::string m_name;
		Resource* m_resource;
	public:
		ResourceReference (const std::string& name) :
			m_name(name)
		{
			capture();
		}
		ResourceReference (const ResourceReference& other) :
			m_name(other.m_name)
		{
			capture();
		}
		ResourceReference& operator= (const ResourceReference& other)
		{
			ResourceReference tmp(other);
			tmp.swap(*this);
			return *this;
		}
		~ResourceReference ()
		{
			GlobalReferenceCache().release(m_name);
		}

		void capture ()
		{
			m_resource = GlobalReferenceCache().capture(m_name);
		}

		const std::string& getName () const
		{
			return m_name;
		}
		void setName (const std::string& name)
		{
			ResourceReference tmp(name);
			tmp.swap(*this);
		}

		void swap (ResourceReference& other)
		{
			std::swap(m_resource, other.m_resource);
			std::swap(m_name, other.m_name);
		}

		void attach (ModuleObserver& observer)
		{
			m_resource->attach(observer);
		}
		void detach (ModuleObserver& observer)
		{
			m_resource->detach(observer);
		}

		Resource* get ()
		{
			return m_resource;
		}
};

namespace std
{
	/// \brief Swaps the values of \p self and \p other.
	/// Overloads std::swap.
	inline void swap (ResourceReference& self, ResourceReference& other)
	{
		self.swap(other);
	}
}

#endif
