#ifndef TRANSFORMDIALOG_H_
#define TRANSFORMDIALOG_H_

#include <string>
#include <map>
#include "iselection.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/RegistryConnector.h"
namespace gtkutil { class ControlButton; }

// Forward Declarations
typedef struct _GtkWidget GtkWidget;
typedef struct _GtkEditable GtkEditable;
typedef struct _GtkTable GtkTable;

/* greebo: The dialog providing the Free Transform functionality.
 *
 */
namespace ui {

class TransformDialog :
	public gtkutil::PersistentTransientWindow, public SelectionSystem::Observer
{
	// The overall vbox (for global sensitivity toggle)
	GtkWidget* _dialogVBox;

	// The entry fields
	struct EntryRow {
		bool isRotator;
		int axis;
		GtkWidget* hbox;
		GtkWidget* label;
		GtkWidget* step;
		GtkWidget* stepLabel;
		gtkutil::ControlButton* smaller;
		gtkutil::ControlButton* larger;
	};
	typedef std::map<std::string, EntryRow> EntryRowMap;
	EntryRowMap _entries;

	GtkWidget* _rotateLabel;
	GtkTable* _rotateTable;
	GtkWidget* _scaleLabel;
	GtkTable* _scaleTable;

	// The reference to the SelectionInfo (number of brushes, etc.)
	//const SelectionInfo& _selectionInfo;

	// The window position tracker
	gtkutil::WindowPosition _windowPosition;

	// The helper class that syncs the registry to/from widgets
	gtkutil::RegistryConnector _connector;

public:
	// Constructor
	TransformDialog();

	/** greebo: Contains the static instance of this dialog.
	 * Constructs the instance and calls toggle() when invoked.
	 */
	static TransformDialog& Instance();

	/** greebo: This toggles the visibility of the dialog.
	 * It is constructed only once and never destructed during runtime.
	 */
	void toggleDialog();

	/** greebo: SelectionSystem::Observer implementation. Gets called by
	 * the SelectionSystem upon selection change to allow updating of the
	 * widget sensitivity.
	 */
	void selectionChanged(scene::Instance& instance, bool isComponent);

	/** greebo: The command target compatible with FreeCaller<> to connect
	 * 			this method to the EventManager.
	 */
	static void toggle();

private:

	// TransientWindow callbacks
	virtual void _preShow();
	virtual void _preHide();

	/** greebo: Updates the sensitivity of the widgets according to
	 * 			the current selection.
	 */
	void update();

	/** greebo: Saves the step values to the registry
	 */
	void saveToRegistry();

	// This is called to initialise the dialog window / create the widgets
	void populateWindow();

	/** greebo: Helper method that creates a single row
	 *
	 * @param row: the row index of <table> where the widgets should be packed into.
	 * @param isRotator: set to true if a rotator row is to be created.
	 * @param axis: the axis this transformation is referring to.
	 */
	EntryRow createEntryRow(const std::string& label, GtkTable* table,
							int row, bool isRotator, int axis);

	// Callbacks to catch the scale/rotation button clicks
	static void onClickSmaller(GtkWidget* button, EntryRow* row);
	static void onClickLarger(GtkWidget* button, EntryRow* row);

	// The callback ensuring that the step changes are written to the registry
	static void onStepChanged(GtkEditable* editable, TransformDialog* self);

}; // class TransformDialog

} // namespace ui


#endif /*TRANSFORMDIALOG_H_*/
