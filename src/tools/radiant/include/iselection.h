/**
 * @file iselection.h
 */

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

#if !defined(INCLUDED_ISELECTION_H)
#define INCLUDED_ISELECTION_H

#include <cstddef>
#include "generic/constant.h"
#include "generic/callbackfwd.h"
#include "signal/signalfwd.h"

class Renderer;
class View;

/**
 * greebo: A Selectable is everything that can be highlighted
 *         by the user in the scene (e.g. by interaction with the mouse).
 */
class Selectable
{
	public:
		STRING_CONSTANT(Name, "Selectable");

		virtual ~Selectable ()
		{
		}

		// Set the selection status of this object
		virtual void setSelected (bool select) = 0;
		// Check the selection status of this object (TRUE == selected)
		virtual bool isSelected () const = 0;
		// Toggle the selection status
		virtual void invertSelected() = 0;
};

namespace scene
{
	class Instance;
}

class InstanceSelectionObserver
{
	public:
		virtual ~InstanceSelectionObserver ()
		{
		}

		virtual void onSelectedChanged (scene::Instance& instance) = 0;
};

template<typename Element> class BasicVector3;
typedef BasicVector3<float> Vector3;
template<typename Element> class BasicVector4;
typedef BasicVector4<float> Vector4;
class Matrix4;
typedef Vector4 Quaternion;

typedef Callback1<const Selectable&> SelectionChangeCallback;
typedef SignalHandler1<const Selectable&> SelectionChangeHandler;

class SelectionInfo;
namespace selection { struct WorkZone; }

class SelectionSystem
{
	public:
		INTEGER_CONSTANT(Version, 1);
		STRING_CONSTANT(Name, "selection");

		enum EModifier
		{
			eManipulator, // greebo: This is the standard case (drag, click without modifiers)
			eToggle, // This is for Shift-Clicks to toggle the selection of an instance
			eReplace, // This is active if the mouse is moved to a NEW location and Alt-Shift is held
			eCycle // This is active if the mouse STAYS at the same position and Alt-Shift is held
		};

		enum EMode
		{
			eEntity, /**< entity editing */
			ePrimitive, /**< brush editing */
			eComponent	/**< vertex, edge or face editing */
		};

		// The possible modes when in "component manipulation mode"
		enum EComponentMode
		{
			eDefault, eVertex, eEdge, eFace
		};

		// The possible manipulator modes
		enum EManipulatorMode
		{
			eTranslate, eRotate, eScale, eDrag, eClip
		};

		virtual ~SelectionSystem ()
		{
		}

		/** greebo: An SelectionSystem::Observer gets notified
		 * as soon as the selection is changed.
		 */
		class Observer
		{
			public:
				virtual ~Observer () {
				}
				virtual void selectionChanged (scene::Instance& instance, bool isComponent) = 0;
		};

		virtual void addObserver (Observer* observer) = 0;
		virtual void removeObserver (Observer* observer) = 0;

		virtual const SelectionInfo& getSelectionInfo() = 0;

		virtual void SetMode (EMode mode) = 0;
		virtual EMode Mode () const = 0;
		virtual void SetComponentMode (EComponentMode mode) = 0;
		virtual EComponentMode ComponentMode () const = 0;
		virtual void SetManipulatorMode (EManipulatorMode mode) = 0;
		virtual EManipulatorMode ManipulatorMode () const = 0;

		// amount of selected primitives (brushes)
		virtual std::size_t countSelected () const = 0;
		// amount of selected components (vertices, edges, faces)
		virtual std::size_t countSelectedComponents () const = 0;
		// amount of selected entities
		virtual std::size_t countSelectedEntities () const = 0;
		// amount of selected faces
		virtual std::size_t countSelectedFaces () const = 0;
		// only checks whether faces are selected, but does not count them
		virtual bool areFacesSelected () const = 0;
		virtual void onSelectedChanged (scene::Instance& instance, const Selectable& selectable) = 0;
		virtual void onComponentSelection (scene::Instance& instance, const Selectable& selectable) = 0;
		virtual scene::Instance& ultimateSelected () const = 0;
		virtual scene::Instance& penultimateSelected () const = 0;
		/**
		 * \brief
		 * Set the selection status of all objects in the scene.
		 *
		 * \param selected
		 * true to select all objects, false to deselect all objects.
		 */
		virtual void setSelectedAll (bool selected) = 0;
		virtual void setSelectedAllComponents (bool selected) = 0;

		/**
		 * @brief
		 * Visitor interface the for the selection system.
		 *
		 * This defines the Visitor interface which is used in the foreachSelected()
		 * and foreachSelectedComponent() visit methods.
		 */
		class Visitor
		{
			public:
				virtual ~Visitor ()
				{
				}

				/**
				 * @brief Called by the selection system for each visited node.
				 */
				virtual void visit (scene::Instance& instance) const = 0;
		};
		/**
		 * @brief Use the provided Visitor object to enumerate each selected node.
		 */
		virtual void foreachSelected (const Visitor& visitor) const = 0;
		/**
		 * @brief Use the provided Visitor object to enumerate each selected component.
		 */
		virtual void foreachSelectedComponent (const Visitor& visitor) const = 0;

		virtual void addSelectionChangeCallback (const SelectionChangeHandler& handler) = 0;

		virtual void NudgeManipulator (const Vector3& nudge, const Vector3& view) = 0;

		virtual void translateSelected (const Vector3& translation) = 0;
		virtual void rotateSelected (const Quaternion& rotation) = 0;
		virtual void scaleSelected (const Vector3& scaling) = 0;

		virtual void pivotChanged () const = 0;

		virtual bool SelectManipulator(const View& view, const float device_point[2], const float device_epsilon[2]) = 0;
		virtual void SelectPoint(const View& view, const float device_point[2], const float device_epsilon[2],
				EModifier modifier, bool face) = 0;
		virtual void SelectArea(const View& view, const float device_point[2], const float device_delta[2],
				EModifier modifier, bool face) = 0;

		virtual void MoveSelected(const View& view, const float device_point[2]) = 0;
		virtual void endMove() = 0;

		/**
		 * Returns the current "work zone", which is defined by the
		 * currently selected elements. Each time a scene node is selected,
		 * the workzone is adjusted to surround the current selection.
		 * Deselecting nodes doesn't change the workzone.
		 *
		 * The result is used to determine the "third" component of operations
		 * performed in the 2D views, like placing an entity.
		 *
		 * Note: the struct is defined in selectionlib.h.
		 */
		virtual const selection::WorkZone& getWorkZone() = 0;
};

#include "modulesystem.h"

template<typename Type>
class GlobalModule;
typedef GlobalModule<SelectionSystem> GlobalSelectionModule;

template<typename Type>
class GlobalModuleRef;
typedef GlobalModuleRef<SelectionSystem> GlobalSelectionModuleRef;

inline SelectionSystem& GlobalSelectionSystem ()
{
	return GlobalSelectionModule::getTable();
}

#endif
