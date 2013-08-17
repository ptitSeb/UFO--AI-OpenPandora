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

#include "dialog.h"
#include "../radiant/ui/mainframe/mainframe.h"

#include "gtkutil/dialog.h"
#include <gtk/gtk.h>
#include <string>

#include "radiant_i18n.h"
#include "button.h"
#include "window.h"

GtkVBox* create_dialog_vbox (int spacing, int border)
{
	GtkVBox* vbox = GTK_VBOX(gtk_vbox_new(FALSE, spacing));
	gtk_widget_show(GTK_WIDGET(vbox));
	gtk_container_set_border_width(GTK_CONTAINER(vbox), border);
	return vbox;
}

GtkHBox* create_dialog_hbox (int spacing, int border)
{
	GtkHBox* hbox = GTK_HBOX(gtk_hbox_new(FALSE, spacing));
	gtk_widget_show(GTK_WIDGET(hbox));
	gtk_container_set_border_width(GTK_CONTAINER(hbox), border);
	return hbox;
}

GtkFrame* create_dialog_frame (const std::string& frameHeadline, GtkShadowType shadow)
{
	GtkFrame* frame = GTK_FRAME(gtk_frame_new(NULL));
	if (!frameHeadline.empty()) {
		GtkLabel* label = GTK_LABEL(gtk_label_new(NULL));
		gtk_label_set_markup(label, frameHeadline.c_str());
		gtk_frame_set_label_widget(frame, GTK_WIDGET(label));
	}
	gtk_widget_show_all(GTK_WIDGET(frame));
	gtk_frame_set_shadow_type(frame, shadow);
	return frame;
}

GtkButton* create_dialog_button (const std::string& label, GCallback func, gpointer data)
{
	GtkButton* button = GTK_BUTTON(gtk_button_new_with_label(label.c_str()));
	gtk_widget_show(GTK_WIDGET(button));
	widget_set_size(GTK_WIDGET(button), 64, 0);
	g_signal_connect(G_OBJECT(button), "clicked", func, data);
	return button;
}

inline GtkWindow* create_dialog_window (GtkWindow* parent, const std::string& title, GCallback func, gpointer data,
		int default_w, int default_h)
{
	GtkWindow* window = create_floating_window(title, parent);
	gtk_window_set_default_size(window, default_w, default_h);
	gtk_window_set_position(window, GTK_WIN_POS_CENTER_ON_PARENT);
	g_signal_connect(G_OBJECT(window), "delete_event", func, data);

	return window;
}

gboolean modal_dialog_button_clicked (GtkWidget *widget, ModalDialogButton* button)
{
	button->m_dialog.loop = false;
	button->m_dialog.ret = button->m_value;
	return TRUE;
}

gboolean modal_dialog_delete (GtkWidget *widget, GdkEvent* event, ModalDialog* dialog)
{
	dialog->loop = 0;
	dialog->ret = eIDCANCEL;
	return TRUE;
}

EMessageBoxReturn modal_dialog_show (GtkWindow* window, ModalDialog& dialog)
{
	gtk_widget_show(GTK_WIDGET(window));
	gtk_grab_add(GTK_WIDGET(window));

	dialog.loop = true;
	while (dialog.loop) {
		gtk_main_iteration();
	}

	gtk_grab_remove(GTK_WIDGET(window));
	gtk_widget_hide(GTK_WIDGET(window));

	return dialog.ret;
}

GtkButton* create_modal_dialog_button (const std::string& label, ModalDialogButton& button)
{
	return create_dialog_button(label, G_CALLBACK(modal_dialog_button_clicked), &button);
}

GtkWindow* create_fixedsize_modal_dialog_window (GtkWindow* parent, const std::string& title, ModalDialog& dialog,
		int width, int height)
{
	GtkWindow* window = create_dialog_window(parent, title, G_CALLBACK(modal_dialog_delete), &dialog, width, height);

	gtk_window_set_resizable(window, FALSE);
	gtk_window_set_modal(window, TRUE);
	gtk_window_set_position(window, GTK_WIN_POS_CENTER);

	window_remove_minmax(window);

	return window;
}

gboolean dialog_button_ok (GtkWidget *widget, ModalDialog* data)
{
	data->loop = false;
	data->ret = eIDOK;
	return TRUE;
}

gboolean dialog_button_cancel (GtkWidget *widget, ModalDialog* data)
{
	data->loop = false;
	data->ret = eIDCANCEL;
	return TRUE;
}

gboolean dialog_delete_callback (GtkWidget *widget, GdkEventAny* event, ModalDialog* data)
{
	gtk_widget_hide(widget);
	data->loop = false;
	return TRUE;
}

GtkWindow* create_simple_modal_dialog_window (const std::string& title, ModalDialog& dialog, GtkWidget* contents)
{
	GtkWindow* window = create_fixedsize_modal_dialog_window(0, title, dialog);

	GtkVBox* vbox1 = create_dialog_vbox(8, 4);
	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(vbox1));

	gtk_container_add(GTK_CONTAINER(vbox1), contents);

	GtkAlignment* alignment = GTK_ALIGNMENT(gtk_alignment_new(0.5, 0.0, 0.0, 0.0));
	gtk_widget_show(GTK_WIDGET(alignment));
	gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(alignment), FALSE, FALSE, 0);

	GtkButton* button = create_dialog_button(_("OK"), G_CALLBACK(dialog_button_ok), &dialog);
	gtk_container_add(GTK_CONTAINER(alignment), GTK_WIDGET(button));

	return window;
}

PathEntry PathEntry_new ()
{
	GtkFrame* frame = GTK_FRAME(gtk_frame_new(NULL));
	gtk_widget_show(GTK_WIDGET(frame));
	gtk_frame_set_shadow_type(frame, GTK_SHADOW_IN);

	// path entry
	GtkHBox* hbox = GTK_HBOX(gtk_hbox_new(FALSE, 0));
	gtk_widget_show(GTK_WIDGET(hbox));

	GtkEntry* entry = GTK_ENTRY(gtk_entry_new());
	gtk_entry_set_has_frame(entry, FALSE);
	gtk_widget_show(GTK_WIDGET(entry));
	gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(entry), TRUE, TRUE, 0);

	// browse button
	GtkButton* button = GTK_BUTTON(gtk_button_new());
	button_set_icon(button, "ellipsis.png");
	gtk_widget_show(GTK_WIDGET(button));
	gtk_box_pack_end(GTK_BOX(hbox), GTK_WIDGET(button), FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(hbox));

	return PathEntry(frame, entry, button);
}

GtkLabel* DialogLabel_new (const std::string& name)
{
	GtkLabel* label = GTK_LABEL(gtk_label_new(name.c_str()));
	gtk_widget_show(GTK_WIDGET(label));
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	gtk_label_set_justify(label, GTK_JUSTIFY_LEFT);

	return label;
}

GtkTable* DialogRow_new (const std::string& name, GtkWidget* widget)
{
	GtkTable* table = GTK_TABLE(gtk_table_new(1, 3, TRUE));
	gtk_widget_show(GTK_WIDGET(table));

	gtk_table_set_col_spacings(table, 4);
	gtk_table_set_row_spacings(table, 0);

	gtk_table_attach(table, GTK_WIDGET(DialogLabel_new(name)), 0, 1, 0, 1, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions) (0), 0, 0);

	gtk_table_attach(table, widget, 1, 3, 0, 1, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0,
			0);

	return table;
}

void DialogVBox_packRow (GtkVBox* vbox, GtkWidget* row)
{
	gtk_box_pack_start(GTK_BOX(vbox), row, FALSE, FALSE, 0);
}

/* gtkutil namespace */

namespace gtkutil
{
	// Display a Gtk Error dialog
	void errorDialog (const std::string& errorText)
	{
		GtkWindow* window = MainFrame_getWindow();
		GtkWidget* dialog = gtk_message_dialog_new_with_markup(window, GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", errorText.c_str());
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}

	// Display a Gtk Info dialog
	void infoDialog (const std::string& infoText)
	{
		GtkWindow* window = MainFrame_getWindow();
		GtkWidget* dialog = gtk_message_dialog_new_with_markup(window, GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "%s", infoText.c_str());
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}

	// Display a fatal Gtk Error dialog
	void fatalErrorDialog (const std::string& errorText)
	{
		errorDialog(errorText);
		abort();
	}
} // namespace gtkutil
