#ifndef CONTROLBUTTON_H_
#define CONTROLBUTTON_H_

#include "image.h"
#include "Timer.h"

#include <gtk/gtkbutton.h>
#include <gtk/gtkvbox.h>

/** greebo: A ControlButton is a button containing a single icon
 * that keeps periodically calling the "clicked" event as long as the user
 * keeps the mouse button pressed. (Used for Surface Inspector controls, for ex.)
 *
 * Specify the filename of the icon to be used in the constructor (must be located
 * in DarkRadiant's "bitmaps/" folder.
 */
namespace gtkutil {

	namespace {
		// The delay between the first "click" and the second "click" event
		const int DELAY_INITIAL = 200;
		// The delay between all following "click" events
		const int DELAY_PERIODIC = 20;
	}

class ControlButton
{
	// Icon pixbuf
	GdkPixbuf* _iconPixBuf;

	// The actual button widget
	GtkWidget* _button;

	// The timer object that periodically fires the onTimeOut() method
	Timer _timer;

public:

	ControlButton(const std::string& iconPath) :
		_iconPixBuf(gtkutil::getLocalPixbuf(iconPath)),
		_button(gtk_button_new()),
		_timer(DELAY_INITIAL, onTimeOut, this)
	{
		_timer.disable();

		// Add the icon to the button
		GtkWidget* icon = gtk_image_new_from_pixbuf(_iconPixBuf);
		GtkWidget* vbox = gtk_vbox_new(false, 3);
		gtk_box_pack_start(GTK_BOX(vbox), icon, true, false, 0);

		gtk_container_add(GTK_CONTAINER(_button), vbox);

		// Connect the pressed/released signals
		g_signal_connect(G_OBJECT(_button), "pressed", G_CALLBACK(onPress), this);
		g_signal_connect(G_OBJECT(_button), "released", G_CALLBACK(onRelease), this);
	}

	/** Operator cast to GtkWidget*
	 */
	operator GtkWidget* () {
		// Return the button
		return _button;
	}

	static gboolean onTimeOut(gpointer data) {
		ControlButton* self = reinterpret_cast<ControlButton*>(data);

		// Fire the "clicked" signal
		gtk_button_clicked(GTK_BUTTON(self->_button));

		// Set the interval to a smaller value
		self->_timer.setTimeout(DELAY_PERIODIC);
		self->_timer.enable();

		// Return true, so that the timer gets called again
		return true;
	}

	static void onPress(GtkButton* button, ControlButton* self) {
		// Connect the timing event
		self->_timer.enable();
	}

	static void onRelease(GtkButton* button, ControlButton* self) {
		// Disconnect the timing event
		self->_timer.disable();
		// Reset the interval to the initial value
		self->_timer.setTimeout(DELAY_INITIAL);
	}
};

} // namespace gtkutil

#endif /*CONTROLBUTTON_H_*/
