#ifndef FINDSHADER_H_
#define FINDSHADER_H_

#include <string>
#include <gtk/gtkwidget.h>
#include <gtk/gtkeditable.h>
#include <gtk/gtkliststore.h>
#include "gtkutil/window/PersistentTransientWindow.h"

/* greebo: The dialog providing the Find & Replace shader functionality.
 *
 * Note: Show the dialog by instantiating it. It automatically enters a
 *       GTK main loop after show().
 */
namespace ui {

class FindAndReplaceShader :
	public gtkutil::PersistentTransientWindow
{
	// The entry fields
	GtkWidget* _findEntry;
	GtkWidget* _replaceEntry;

	// The buttons to select the shader
	GtkWidget* _findSelectButton;
	GtkWidget* _replaceSelectButton;

	// The checkbox "Search Selected Only"
	GtkWidget* _selectedOnly;

	// The counter "x shaders replaced."
	GtkWidget* _counterLabel;

public:
	// Constructor
	FindAndReplaceShader(const std::string& find = "", const std::string& replace = "");
	~FindAndReplaceShader();

	/** greebo: Shows the dialog (allocates on heap, dialog self-destructs)
	 */
	static void showDialog();

private:

	// This is called to initialise the dialog window / create the widgets
	void populateWindow();

	/** greebo: As the name states, this runs the replace algorithm
	 */
	void performReplace();

	// Helper method to create the OK/Cancel button
	GtkWidget* createButtons();

	// The callback for the buttons
	static void onReplace(GtkWidget* widget, FindAndReplaceShader* self);
	static void onClose(GtkWidget* widget, FindAndReplaceShader* self);

	static void onChooseFind(GtkWidget* widget, FindAndReplaceShader* self);
	static void onChooseReplace(GtkWidget* widget, FindAndReplaceShader* self);

	static void onFindChanged(GtkEditable* editable, FindAndReplaceShader* self);
	static void onReplaceChanged(GtkEditable* editable, FindAndReplaceShader* self);
}; // class FindAndReplaceShader

} // namespace ui

#endif /*FINDSHADER_H_*/
