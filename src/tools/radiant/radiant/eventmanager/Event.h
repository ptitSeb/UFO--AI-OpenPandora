#ifndef EVENT_H_
#define EVENT_H_

/* The base class for an Event.
 *
 * Provides methods to enable/disable the event and to connect GtkWidgets
 */

class Event :
	public IEvent
{
protected:
	// If false, the command is ignored and not executed.
	bool _enabled;

public:
	// Constructor
	Event() :
		_enabled(true)
	{}

	virtual ~Event() {}

	// Enables/disables this command according to the passed bool <enabled>
	virtual void setEnabled(const bool enabled) {
		_enabled = enabled;
	}

	virtual bool isToggle() const {
		return false;
	}

	virtual bool setToggled(const bool toggled) {
		return false;
	}

	virtual void keyUp() {}
	virtual void keyDown() {}

	virtual void updateWidgets() {}

	// Empty standard implementation
	// (implement this in the derived classes to support the various GTKWidget types)
	virtual void connectWidget(GtkWidget* widget) {}

}; // class Event

#endif /*EVENT_H_*/
