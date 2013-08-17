#include "ColourSchemeEditor.h"
#include "ColourSchemeManager.h"

#include "iregistry.h"
#include "iscenegraph.h"

#include "radiant_i18n.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/ScrolledFrame.h"

#include "../../plugin.h"
#include "../../ui/mainframe/mainframe.h"
#include "../../brush/BrushModule.h"

namespace ui {

	// Constants
	namespace {
		const int EDITOR_DEFAULT_SIZE_X = 650;
		const int EDITOR_DEFAULT_SIZE_Y = 450;
		const int COLOURS_PER_COLUMN = 10;
		const std::string EDITOR_WINDOW_TITLE = _("Edit Colour Schemes");
		const unsigned int GDK_FULL_INTENSITY = 65535;
	}

/*	Loads all the scheme items into the list
 */
void ColourSchemeEditor::populateTree() {
	GtkTreeIter iter;

	ColourSchemeMap allSchemes = ColourSchemes().getSchemeList();

	for (ColourSchemeMap::iterator scheme = allSchemes.begin(); scheme != allSchemes.end(); ++scheme) {
		gtk_list_store_append(_listStore, &iter);
		gtk_list_store_set(_listStore, &iter, 0, scheme->first.c_str(), -1);
	}
}

void ColourSchemeEditor::createTreeView() {
	// Create the listStore
	_listStore = gtk_list_store_new(1, G_TYPE_STRING);

	// Create the treeView
	_treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_listStore));
	// Unreference the list store so it will be destroyed with the treeview
	g_object_unref(G_OBJECT(_listStore));

	// Create a new column and set its parameters
	GtkTreeViewColumn* col = gtk_tree_view_column_new();

	// Pack the new column into the treeView
	gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView), col);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(_treeView), FALSE);

	// Create a new text renderer and attach it to the column
	GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", 0);
}

GtkWidget* ColourSchemeEditor::constructButtons() {
	// Create the buttons and put them into a horizontal box
	GtkWidget* buttonBox = gtk_hbox_new(TRUE, 12);

	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);

	gtk_box_pack_end(GTK_BOX(buttonBox), okButton, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(buttonBox), cancelButton, TRUE, TRUE, 0);

	g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(callbackOK), this);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(callbackCancel), this);
	return gtkutil::RightAlignment(buttonBox);
}

GtkWidget* ColourSchemeEditor::constructTreeviewButtons() {
	GtkWidget* buttonBox = gtk_hbox_new(TRUE, 6);

	_deleteButton = gtk_button_new_from_stock(GTK_STOCK_DELETE);
	GtkWidget* copyButton = gtk_button_new_from_stock(GTK_STOCK_COPY);

	gtk_box_pack_start(GTK_BOX(buttonBox), copyButton, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(buttonBox), _deleteButton, TRUE, TRUE, 0);

	g_signal_connect(G_OBJECT(copyButton), "clicked", G_CALLBACK(callbackCopy), this);
	g_signal_connect(G_OBJECT(_deleteButton), "clicked", G_CALLBACK(callbackDelete), this);

	return buttonBox;
}

GtkWidget* ColourSchemeEditor::constructWindow() {
	// The vbox that separates the buttons and the upper part of the window
	GtkWidget* vbox = gtk_vbox_new(FALSE, 12);

	// Place the buttons at the bottom of the window
	gtk_box_pack_end(GTK_BOX(vbox), constructButtons(), FALSE, FALSE, 0);

	// This is the box for the treeview and the whole rest
	GtkWidget* hbox = gtk_hbox_new(FALSE, 12);

	// VBox containing the tree view and copy/delete buttons underneath
	GtkWidget* treeAndButtons = gtk_vbox_new(FALSE, 6);

	// Create the treeview and pack it into the treeViewFrame
	createTreeView();
	gtk_box_pack_start(GTK_BOX(treeAndButtons), gtkutil::ScrolledFrame(_treeView), TRUE, TRUE, 0);

	gtk_box_pack_end(GTK_BOX(treeAndButtons), constructTreeviewButtons(), FALSE, FALSE, 0);

	// Pack the treeViewFrame into the hbox
	gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(treeAndButtons), FALSE, FALSE, 0);

	// The Box containing the Colour, pack it into the right half of the hbox
	_colourFrame = gtk_frame_new(NULL);
	_colourBox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(_colourFrame), _colourBox);

	gtk_box_pack_start(GTK_BOX(hbox), _colourFrame, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

	return vbox;
}

void ColourSchemeEditor::selectActiveScheme() {
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(_listStore), &iter)) {

		do {
			// Get the value
			GValue val;
			val.g_type = 0;
			gtk_tree_model_get_value(GTK_TREE_MODEL(_listStore), &iter, 0, &val);
			// Get the string
			std::string name = g_value_get_string(&val);

			if (ColourSchemes().isActive(name)) {
				gtk_tree_selection_select_iter(_selection, &iter);

				// Set the button sensitivity correctly for read-only schemes
				gtk_widget_set_sensitive(_deleteButton, (!ColourSchemes().getScheme(name).isReadOnly()));

				return;
			}
		} while (gtk_tree_model_iter_next(GTK_TREE_MODEL(_listStore), &iter));
	}
}

ColourSchemeEditor::ColourSchemeEditor()
 :	_editorWidget(gtk_window_new(GTK_WINDOW_TOPLEVEL))
{
	gtk_window_set_transient_for(GTK_WINDOW(_editorWidget), MainFrame_getWindow());
	gtk_window_set_modal(GTK_WINDOW(_editorWidget), TRUE);
	gtk_window_set_position(GTK_WINDOW(_editorWidget), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_default_size(GTK_WINDOW(_editorWidget), EDITOR_DEFAULT_SIZE_X, EDITOR_DEFAULT_SIZE_Y);
	gtk_window_set_title(GTK_WINDOW(_editorWidget), EDITOR_WINDOW_TITLE.c_str());

	// Get the constructed windowframe and pack it into the editor widget
	gtk_container_set_border_width(GTK_CONTAINER(_editorWidget), 12);
	gtk_container_add(GTK_CONTAINER(_editorWidget), constructWindow());

	// Load all the list items
	populateTree();

	// Connect the selection callback
	_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView));

	// Highlight the currently selected scheme
	selectActiveScheme();
	updateColourSelectors();

	// Connect the signal AFTER selecting the active scheme
	g_signal_connect(G_OBJECT(_selection), "changed", G_CALLBACK(callbackSelChanged), this);

	// Be sure that everything is properly destroyed upon window closure
	g_signal_connect(G_OBJECT(_editorWidget), "delete_event", G_CALLBACK(_onDeleteEvent), this);

	// Show all the widgets and enter the loop
	gtk_widget_show_all(_editorWidget);
}

void ColourSchemeEditor::deleteSchemeFromList() {
	GtkTreeIter iter;
	GtkTreeModel* model;

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView));

	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
	}

	// Select the first element
	gtk_tree_model_get_iter_first(model, &iter);
	gtk_tree_selection_select_iter(_selection, &iter);
}

std::string ColourSchemeEditor::getSelectedScheme() {
	GtkTreeIter iter;
	GtkTreeModel* model;

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView));

	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		// Get the value
		GValue val;
		val.g_type = 0;
		gtk_tree_model_get_value(model, &iter, 0, &val);
		// Get the string
		return g_value_get_string(&val);
	}
	else {
		return "";
	}
}

GtkWidget* ColourSchemeEditor::constructColourSelector(ColourItem& colour, const std::string& name) {
	// Get the description of this colour item from the registry
	std::string descriptionPath = std::string("user/ui/colourschemes/descriptions/") + name;
	std::string description = GlobalRegistry().get(descriptionPath);

	// Create a new horizontal divider
	GtkWidget* hbox = gtk_hbox_new(FALSE, 10);

	GdkColor tempColour;
	Vector3 tempColourVector = colour;
	tempColour.red = (unsigned int) (GDK_FULL_INTENSITY * tempColourVector[0]);
	tempColour.green = (unsigned int) (GDK_FULL_INTENSITY * tempColourVector[1]);
	tempColour.blue = (unsigned int) (GDK_FULL_INTENSITY * tempColourVector[2]);

	// Create the colour button
	GtkWidget* button = gtk_color_button_new_with_color(&tempColour);
	gtk_color_button_set_title(GTK_COLOR_BUTTON(button), description.c_str());
	ColourItem* colourPtr = &colour;

	// Connect the signal, so that the ColourItem class is updated along with the colour button
	g_signal_connect(G_OBJECT(button), "color-set", G_CALLBACK(callbackColorChanged), colourPtr);
	gtk_widget_show(button);

	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

	GtkWidget* label = gtk_label_new(description.c_str());

	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	return hbox;
}

void ColourSchemeEditor::updateColourSelectors() {
	unsigned int i = 1;
	ColourItemMap::iterator it;

	gtk_widget_hide(_colourBox);
	gtk_widget_destroy(_colourBox);

	GtkWidget* curVbox = gtk_vbox_new(FALSE, 5);

	// Create the column container
	_colourBox = gtk_hbox_new(FALSE, 12);

	gtk_container_add(GTK_CONTAINER(_colourFrame), _colourBox);

	// Get the selected scheme
	ColourScheme& scheme = ColourSchemes().getScheme( getSelectedScheme() );

	// Retrieve the list with all the ColourItems of this scheme
	ColourItemMap& colourMap = scheme.getColourMap();

	// Cycle through all the ColourItems and save them into the registry
	for (it = colourMap.begin(), i = 1;
		 it != colourMap.end();
		 ++it, i++)
	{
		GtkWidget* colourSelector = constructColourSelector(it->second, it->first);
		gtk_box_pack_start(GTK_BOX(curVbox), colourSelector, FALSE, FALSE, 5);

		// Have we reached the maximum number of colours per column?
		if (i % COLOURS_PER_COLUMN == 0) {
			// yes, pack the current column into the _colourBox and create a new vbox
			gtk_box_pack_start(GTK_BOX(_colourBox), curVbox, FALSE, FALSE, 5);
			curVbox = gtk_vbox_new(FALSE, 5);
		}
	}

	// Pack the remaining items into the last column
	gtk_box_pack_start(GTK_BOX(_colourBox), curVbox, FALSE, FALSE, 0);

	gtk_widget_show_all(_colourBox);
}

void ColourSchemeEditor::updateWindows() {
	// Call the update, so all colours can be previewed
	XY_UpdateAllWindows();
	GlobalCamera_UpdateWindow();
	GlobalBrush()->clipperColourChanged();
	SceneChangeNotify();
}

void ColourSchemeEditor::selectionChanged() {
	std::string activeScheme = getSelectedScheme();

	// Update the colour selectors to reflect the newly selected scheme
	updateColourSelectors();

	// Check, if the currently selected scheme is read-only
	ColourScheme& scheme = ColourSchemes().getScheme(activeScheme);
	gtk_widget_set_sensitive(_deleteButton, (!scheme.isReadOnly()));

	// Set the active Scheme, so that the views are updated accordingly
	ColourSchemes().setActive(activeScheme);

	updateWindows();
}

void ColourSchemeEditor::deleteScheme() {
	std::string name = getSelectedScheme();
	// Get the selected scheme
	ColourScheme& scheme = ColourSchemes().getScheme(name);

	if (!scheme.isReadOnly()) {
		// Remove the actual scheme from the ColourSchemeManager
		ColourSchemes().deleteScheme(name);

		// Remove the selected item from the GtkListStore
		deleteSchemeFromList();
	}
}

std::string ColourSchemeEditor::inputDialog(const std::string& title, const std::string& label) {
	GtkWidget* dialog;
	GtkWidget* labelWidget;
	GtkWidget* entryWidget;
	std::string returnValue;

	dialog = gtk_dialog_new_with_buttons(title.c_str(), GTK_WINDOW(_editorWidget), GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);

	labelWidget = gtk_label_new(label.c_str());
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), labelWidget);

	entryWidget = gtk_entry_new();
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), entryWidget);

	gtk_widget_show_all(dialog);
	gint response = gtk_dialog_run(GTK_DIALOG(dialog));

	if (response == GTK_RESPONSE_OK) {
		returnValue = gtk_entry_get_text(GTK_ENTRY(entryWidget));
	}
	else {
		returnValue = "";
	}

	gtk_widget_destroy(dialog);

	return returnValue;
}

void ColourSchemeEditor::copyScheme() {
	GtkTreeIter iter;
	std::string name = getSelectedScheme();
	std::string newName = inputDialog(_("Copy Colour Scheme"), _("Enter a name for the new scheme:"));

	if (newName != "") {
		// Copy the scheme
		ColourSchemes().copyScheme(name, newName);
		ColourSchemes().setActive(newName);

		// Add the new list item to the ListStore
		gtk_list_store_append(_listStore, &iter);
		gtk_list_store_set(_listStore, &iter, 0, newName.c_str(), -1);

		// Highlight the copied scheme
		selectActiveScheme();
	}
}

// GTK Callback Routines

void ColourSchemeEditor::callbackCopy(GtkWidget* widget, ColourSchemeEditor* self) {
	self->copyScheme();
}

void ColourSchemeEditor::callbackDelete(GtkWidget* widget, ColourSchemeEditor* self) {
	self->deleteScheme();
}

void ColourSchemeEditor::callbackColorChanged(GtkWidget* widget, ColourItem* colourItem) {
	GdkColor colour;
	gtk_color_button_get_color(GTK_COLOR_BUTTON(widget), &colour);

	// Update the colourItem class
	colourItem->set(float(colour.red) / GDK_FULL_INTENSITY,
					float(colour.green) / GDK_FULL_INTENSITY,
					float(colour.blue) / GDK_FULL_INTENSITY);

	// Call the update, so all colours can be previewed
	updateWindows();
}

// This is called when the colourscheme selection is changed
void ColourSchemeEditor::callbackSelChanged(GtkWidget* widget, ColourSchemeEditor* self) {
	self->selectionChanged();
}

void ColourSchemeEditor::callbackOK(GtkWidget* widget, ColourSchemeEditor* self) {
	ColourSchemes().setActive(self->getSelectedScheme());
	ColourSchemes().saveColourSchemes();

	if (GTK_IS_WIDGET(self->_editorWidget)) {
		gtk_widget_hide(GTK_WIDGET(self->_editorWidget));
		gtk_widget_destroy(GTK_WIDGET(self->_editorWidget));
	}
	delete self;
}

// Destroy self function
void ColourSchemeEditor::doCancel() {
	// Restore all the colour settings from the XMLRegistry, changes get lost
	ColourSchemes().restoreColourSchemes();

	// Call the update, so all restored colours are displayed
	updateWindows();

	// Destroy GTK widgets and the object itself
	gtk_widget_hide(_editorWidget);
	gtk_widget_destroy(_editorWidget);
	delete this;
}

void ColourSchemeEditor::callbackCancel(GtkWidget* widget, ColourSchemeEditor* self) {
	self->doCancel();
}

// Window destroy callback
void ColourSchemeEditor::_onDeleteEvent(GtkWidget* w, GdkEvent* e, ColourSchemeEditor* self)
{
	self->doCancel();
}

} // namespace ui
