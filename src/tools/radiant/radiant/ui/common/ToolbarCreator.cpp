#include "ToolbarCreator.h"

#include <gtk/gtk.h>
#include <string>
#include <stdexcept>

#include "ieventmanager.h"

#include "radiant_i18n.h"
#include "stringio.h"
#include "stream/stringstream.h"
#include "stream/textfilestream.h"

#include "gtkutil/pointer.h"
#include "gtkutil/image.h"
#include "gtkutil/button.h"
#include "iregistry.h"


// This is needed to correctly connect the ToggleButton to Radiant's callbacks
// The "handler" object data was set in CreateToolItem
inline void toggleButtonSetActiveNoSignal(GtkToggleToolButton* button, gboolean active)
{
  guint handler_id = gpointer_to_int(g_object_get_data(G_OBJECT(button), "handler"));
  g_signal_handler_block(G_OBJECT(button), handler_id);
  gtk_toggle_tool_button_set_active(button, active);
  g_signal_handler_unblock(G_OBJECT(button), handler_id);
}

inline void toggleButtonSetActiveCallback(GtkToggleToolButton& button, bool active)
{
  toggleButtonSetActiveNoSignal(&button, active);
}
typedef ReferenceCaller1<GtkToggleToolButton, bool, toggleButtonSetActiveCallback> ToggleButtonSetActiveCaller;

namespace ui {

	/*	Returns the toolbar that is named toolbarName
	 */
	GtkToolbar* ToolbarCreator::getToolbar(const std::string& toolbarName) {
		if (toolbarExists(toolbarName)) {
			return _toolbars[toolbarName];
		}
		else {
			return NULL;
		}
	}

	bool ToolbarCreator::toolbarExists(const std::string& toolbarName) {
		ToolbarMap::iterator it = _toolbars.find(toolbarName);
		return (it != _toolbars.end());
	}

	/* Checks the passed xmlNode for a recognized item (ToolButton, ToggleToolButton, Separator)
	 * Returns the widget or NULL if nothing useful is found
	 */
	GtkWidget* ToolbarCreator::createToolItem(xml::Node& node, GtkToolbar* toolbar) {
		const std::string nodeName = node.getName();
		GtkWidget* toolItem;

		if (nodeName == "separator") {
			toolItem = GTK_WIDGET(gtk_separator_tool_item_new());
		}
		else if (nodeName == "toolbutton" || nodeName == "toggletoolbutton") {
			// Found a button, load the values that are shared by both types
			const std::string name 		= node.getAttributeValue("name");
			const std::string icon 		= node.getAttributeValue("icon");
			const std::string tooltip 	= _(node.getAttributeValue("tooltip").c_str());
			const std::string action 	= node.getAttributeValue("action");

			if (nodeName == "toolbutton") {
				// Create a new GtkToolButton and assign the right callback
				toolItem = GTK_WIDGET(gtk_tool_button_new(NULL, name.c_str()));

			}
			else {
				// Create a new GtkToggleToolButton and assign the right callback
				toolItem = GTK_WIDGET(gtk_toggle_tool_button_new());

			}

			IEvent* event = GlobalEventManager().findEvent(action);
			if (event != NULL) {
				event->connectWidget(GTK_WIDGET(toolItem));

				// Tell the event to update the state of this button
				event->updateWidgets();
			} else {
				globalErrorStream() << "ToolbarCreator: Failed to lookup command " << action << "\n";
			}

			// Set the tooltip, if not empty
			if (!tooltip.empty()) {
				gtk_tooltips_set_tip(_tooltips, GTK_WIDGET(toolItem), tooltip.c_str(), "");
				//gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(toolItem), _tooltips, tooltip.c_str(), "");
			}

			// Load and assign the icon, if specified
			if (icon != "") {
				GtkWidget* image = gtk_image_new_from_pixbuf(gtkutil::getLocalPixbufWithMask(icon));
				gtk_widget_show(image);
				gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(toolItem), image);
			}
		}
		else {
			return NULL;
		}

		gtk_widget_show(toolItem);
		return toolItem;
	}

	/*	Creates a toolbar based on the data found in the passed xmlNode
	 * 	Returns the fully populated GtkToolbar
	 */
	GtkToolbar* ToolbarCreator::createToolbar(xml::Node& node) {
		// Get all action children elements
		xml::NodeList toolItemList = node.getChildren();
		GtkWidget* toolbar;

		if (toolItemList.size() > 0) {
			// Create a new toolbar
			toolbar = gtk_toolbar_new();
			gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
			gtk_toolbar_set_show_arrow(GTK_TOOLBAR(toolbar), TRUE);

			// Try to set the alignment, if the attribute is properly set
			std::string align = node.getAttributeValue("align");

			gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), align == "vertical" ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL);

			for (unsigned int i = 0; i < toolItemList.size(); i++) {
				// Create and get the toolItem with the parsing
				GtkWidget* toolItem = createToolItem(toolItemList[i], GTK_TOOLBAR(toolbar));

				// It is possible that no toolItem is returned, only add it if it's safe to do so
				if (toolItem != NULL) {
					gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(toolItem), -1);
				}
			}
		}
		else {
			throw std::runtime_error("No elements in toolbar.");
		}

		return GTK_TOOLBAR(toolbar);
	}

	/* Parses the XML Document for toolbars and instantiates them
	 * Returns nothing, toolbars can be obtained via GetToolbar()
	 */
	void ToolbarCreator::loadToolbars() {
		xml::NodeList toolbarList = GlobalRegistry().findXPath("//ui//toolbar");

		if (toolbarList.size() > 0) {
			_tooltips = gtk_tooltips_new();
			gtk_tooltips_enable(_tooltips);

			for (unsigned int i = 0; i < toolbarList.size(); i++) {
				std::string toolbarName = toolbarList[i].getAttributeValue("name");

				if (toolbarExists(toolbarName)) {
					continue;
				}

				globalOutputStream() << "Found toolbar: " << toolbarName.c_str();
				globalOutputStream() << "\n";

				_toolbars[toolbarName] = createToolbar(toolbarList[i]);
			}
		}
		else {
			throw std::runtime_error("No toolbars found.");
		}
	}


	/* Constructor: Load the definitions from the XMLRegistry
	 */
	ToolbarCreator::ToolbarCreator()
	{
		globalOutputStream() << "ToolbarCreator: Loading toolbar information from registry.\n";
		try {
			// Query the registry
			loadToolbars();
		}
		catch (std::runtime_error &e) {
			globalOutputStream() << "ToolbarCreator: Warning: " << e.what() << "\n";
		}
		globalOutputStream() << "ToolbarCreator: Finished loading toolbar information.\n";
	}

} // namespace toolbar
