#ifndef INCLUDE_UIMANAGER_H_
#define INCLUDE_UIMANAGER_H_

#include <string>
#include "generic/constant.h"

// Forward declarations
typedef struct _GtkWidget GtkWidget;

namespace ui {
	/** greebo: The possible menu item types, one of these
	 * 			has to be passed when creating menu items.
	 */
	enum eMenuItemType {
		menuNothing,
		menuRoot,
		menuBar,
		menuFolder,
		menuItem,
		menuSeparator
	};
}

/** greebo: Implementation documentation: see MenuManager.h.
 */
class IMenuManager
{
public:
	virtual ~IMenuManager() {}

	/** greebo: Retrieves the menuitem widget specified by the path.
	 *
	 * Example: get("main/file/open") delivers the widget for the "Open..." command.
	 *
	 * @returns: the widget, or NULL, if path hasn't been found.
	 */
	virtual GtkWidget* get(const std::string& name) = 0;

	/** greebo: Shows/hides the menuitem under the given path.
	 *
	 * @param path: the path to the item (e.g. "main/view/cameraview")
	 * @param visible: FALSE, if the widget should be hidden, TRUE otherwise
	 */
	virtual void setVisibility(const std::string& path, bool visible) = 0;

	/** greebo: Adds a new item as child under the given path.
	 *
	 * @param insertPath: the path where to insert the item: "main/filters"
	 * @param name: the name of the new item
	 * @param type: the item type (usually menuFolder / menuItem)
	 * @param caption: the display string of the menu item (incl. mnemonic)
	 * @param icon: the image file name relative to "bitmaps/", can be empty.
	 * 		  the icon filename (can be empty)
	 * @param eventname: the event name (e.g. "ToggleShowSizeInfo")
	 */
	virtual GtkWidget* add(const std::string& insertPath,
						const std::string& name,
						ui::eMenuItemType type,
						const std::string& caption,
						const std::string& icon,
						const std::string& eventName) = 0;

	/** greebo: Inserts a new menuItem as sibling _before_ the given insertPath.
	 *
	 * @insertPath: the path where to insert the item: "main/filters"
	 * @name: the name of the new menu item (no path, just the name)
	 * @caption: the display string including mnemonic
	 * @eventName: the event name this item is associated with (can be empty).
	 *
	 * @returns: the GtkWidget*
	 */
	virtual GtkWidget* insert(const std::string& insertPath,
							const std::string& name,
							ui::eMenuItemType type,
							const std::string& caption,
							const std::string& icon,
							const std::string& eventName) = 0;

	/**
	 * Removes an entire menu subtree.
	 */
	virtual void remove(const std::string& path) = 0;
};


/** greebo: The UI Manager abstract base class.
 *
 * The UIManager provides an interface to add UI items like menu commands
 * toolbar icons, update status bar texts and such.
 */
class IUIManager
{
public:
	INTEGER_CONSTANT(Version, 1);
	STRING_CONSTANT(Name, "UIManager");

	virtual ~IUIManager()
	{
	}

	virtual IMenuManager* getMenuManager() = 0;
};

// Module definitions

#include "modulesystem.h"

template<typename Type>
class GlobalUIModule;
typedef GlobalModule<IUIManager> GlobalUIManagerModule;

template<typename Type>
class GlobalModuleRef;
typedef GlobalModuleRef<IUIManager> GlobalUIManagerModuleRef;

// This is the accessor for the event manager
inline IUIManager& GlobalUIManager() {
	return GlobalUIManagerModule::getTable();
}

#endif /*INCLUDE_UIMANAGER_H_*/
