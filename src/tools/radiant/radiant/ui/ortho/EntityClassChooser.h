#ifndef ENTITYCLASSCHOOSER_H_
#define ENTITYCLASSCHOOSER_H_

#include <gtk/gtk.h>

#include "radiant_i18n.h"
#include "math/Vector3.h"

namespace ui
{
	/** Dialog window displaying a tree of Entity Classes, allowing the selection
	 * of a class to create at the current location. This widget is displayed
	 * by the OrthoContextMenu.
	 */
	class EntityClassChooser
	{
		private:

			// Main dialog window
			GtkWidget* _widget;

			// Tree model holding the classnames
			GtkTreeStore* _treeStore;

			// GtkTreeSelection holding the currently-selected classname
			GtkTreeSelection* _selection;

			// Usage information textview
			GtkWidget* _usageTextView;

			// Add button. Needs to be a member since we enable/disable it in the
			// selectionchanged callback.
			GtkWidget* _addButton;

			// The 3D coordinates of the point where the entity must be created.
			Vector3 _lastPoint;

		private:

			/* Widget construction helpers */
			GtkWidget* createTreeView ();
			GtkWidget* createUsagePanel ();
			GtkWidget* createButtonPanel ();

			// Update the usage panel with information from the provided entityclass
			void updateUsageInfo (const std::string& eclass);

			/* GTK callbacks */

			static void addEntity (EntityClassChooser*);

			// Called when close button is clicked, ensure that window is hidden
			// not destroyed.
			static void callbackHide (GtkWidget*, GdkEvent*, EntityClassChooser*);

			// Button callbacks
			static void callbackCancel (GtkWidget*, EntityClassChooser*);
			static void callbackAdd (GtkWidget*, EntityClassChooser*);

			// Check when the selection changes, disable the add button if there
			// is nothing selected.
			static void callbackSelectionChanged (GtkWidget*, EntityClassChooser*);

			// Callbacks for double clicks or button presses
			static gint callbackKeyPress (GtkWidget* widget, GdkEventKey* event, EntityClassChooser*);
			static gint callbackMouseButtonPress (GtkWidget *widget, GdkEventButton *event, EntityClassChooser*);

		public:

			/// Constructor. Creates the GTK widgets.
			EntityClassChooser ();

			/** Show the dialog and choose an entity class.
			 *
			 * @param point
			 * The point at which the new entity should be created
			 */
			void show (const Vector3& point);

			/** Obtain the singleton instance and show it, passing in the
			 * required entity creation coordinates.
			 *
			 * @param point
			 * The point at which the new entity should be created
			 */
			static void displayInstance (const Vector3& point);
	};
}

#endif /*ENTITYCLASSCHOOSER_H_*/
