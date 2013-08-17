#ifndef MODELPREVIEW_H_
#define MODELPREVIEW_H_

#include "imodel.h"
#include "math/matrix.h"

#include <gtk/gtk.h>
#include <igl.h>
#include <string>

#include "gtkutil/glwidget.h"

namespace ui
{
	/** Preview widget for models and skins. This class encapsulates the GTK widgets to
	 * render a specified model and skin using OpenGL, and return a single GTK widget
	 * to be packed into the parent window. The model and skin is changed with setModel()
	 * and setSkin(). This class handles zooming and rotating the model itself.
	 */
	class ModelPreview
	{
			// Top-level widget
			GtkWidget* _widget;

			// GL widget
			gtkutil::GLWidget _glWidget;

			// Toolbar buttons
			GtkToolItem* _drawBBox;

			// Current model to display
			model::IModelPtr _model;

			// Current distance between camera and preview
			GLfloat _camDist;

			// Current rotation matrix
			Matrix4 _rotation;

		private:

			/* GTK CALLBACKS */

			static void callbackGLDraw (GtkWidget*, GdkEventExpose*, ModelPreview*);
			static void callbackGLMotion (GtkWidget*, GdkEventMotion*, ModelPreview*);
			static void callbackGLScroll (GtkWidget*, GdkEventScroll*, ModelPreview*);
			static void callbackToggleBBox (GtkToggleToolButton*, ModelPreview*);

		public:

			/** Construct a ModelPreview widget.
			 */
			ModelPreview ();

			/** Destructor for destroying loaded models
			 */
			~ModelPreview ();

			/** Set the pixel size of the ModelPreview widget. The widget is always square.
			 *
			 * @param size
			 * The pixel size of the square widget.
			 */
			void setSize (int size);

			/** Initialise the GL preview. This clears the window and sets up the initial
			 * matrices and lights.
			 */
			void initialisePreview ();

			/** Set the widget to display the given model. If the model name is the empty string,
			 * the widget will release the currently displayed model.
			 *
			 * @param model
			 * String name of the model to display.
			 */
			void setModel (const std::string& model);

			/** Set the skin to apply on the currently-displayed model.
			 *
			 * @param skin
			 * Name of the skin to apply.
			 */
			void setSkin (const std::string& skin);

			/** Operator cast to GtkWidget*, for packing into the parent window.
			 */
			operator GtkWidget* ()
			{
				return _widget;
			}

			/** Get the model from the widget, in order to display properties about it.
			 */
			model::IModel* getModel ()
			{
				return _model.get();
			}
	};

}

#endif /*MODELPREVIEW_H_*/
