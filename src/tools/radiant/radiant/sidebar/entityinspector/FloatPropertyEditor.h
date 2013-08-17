#ifndef FLOATPROPERTYEDITOR_H_
#define FLOATPROPERTYEDITOR_H_

#include "PropertyEditor.h"

#include <gtk/gtkwidget.h>

namespace ui {

/**
 * Property editor for a ranged floating point value. A slider widget is used
 * to adjust the value between the upper and lower limit in the range. The
 * range information is passed in via the options string.
 */
class FloatPropertyEditor: public PropertyEditor
{
		// Slider widget
		GtkWidget* _scale;

		// Entity to edit
		Entity* _entity;

		// Name of key
		std::string _key;

	private:

		/* GTK CALLBACKS */
		static void _onApply (GtkWidget*, FloatPropertyEditor*);

	public:

		/**
		 * Default constructor for creation in the map.
		 */
		FloatPropertyEditor ()
		{
		}

		/**
		 * Construct with Entity, key name and options.
		 */
		FloatPropertyEditor (Entity*, const std::string&, const std::string&);

		/**
		 * Virtual PropertyEditor clone method.
		 */
		PropertyEditorPtr createNew (Entity* entity, const std::string& name, const std::string& options)
		{
			return PropertyEditorPtr(new FloatPropertyEditor(entity, name, options));
		}

};

}

#endif /*FLOATPROPERTYEDITOR_H_*/
