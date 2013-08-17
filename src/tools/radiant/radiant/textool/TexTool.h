#ifndef TEXTOOL_H_
#define TEXTOOL_H_

#include <gtk/gtkwidget.h>
#include "gtkutil/WindowPosition.h"
#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/glwidget.h"
#include "math/Vector3.h"
#include "math/aabb.h"
#include "ishader.h"
#include "iselection.h"
#include "iregistry.h"

#include "TexToolItem.h"

class Winding;
class Patch;

namespace ui {

namespace
{
	const std::string RKEY_TEXTOOL_ROOT = "user/ui/textures/texTool/";
	const std::string RKEY_FACE_VERTEX_SCALE_PIVOT_IS_CENTROID = RKEY_TEXTOOL_ROOT + "faceVertexScalePivotIsCentroid";
}

class TexTool :
	public gtkutil::PersistentTransientWindow,
	public RegistryKeyObserver,
	public SelectionSystem::Observer
{
	// The window position tracker
	gtkutil::WindowPosition _windowPosition;

	gtkutil::GLWidget glWidget;

	// GL widget
	GtkWidget* _glWidget;

	// The shader we're working with (shared ptr)
	IShader* _shader;

	// A reference to the SelectionInfo structure (with the counters)
	const SelectionInfo& _selectionInfo;

	// The extents of the selected objects in texture space
	AABB _selAABB;

	// The extents of the visible rectangle in texture space
	AABB _texSpaceAABB;

	// The dimensions of the GL widget in pixels.
	Vector2 _windowDims;

	// The zoomfactor of this window (default = 1.1)
	float _zoomFactor;

	// The currently active objects in the textool window
	textool::TexToolItemVec _items;

	// The draggable selection rectangle
	textool::Rectangle _selectionRectangle;

	// The rectangle defining the manipulation's start and end point
	textool::Rectangle _manipulateRectangle;

	// The rectangle defining the moveOrigin operation's start and end point
	textool::Rectangle _moveOriginRectangle;

	// TRUE if we are in selection mode
	bool _dragRectangle;

	// TRUE if a manipulation is currently ongoing
	bool _manipulatorMode;

	// TRUE if the texspace center is being dragged around
	bool _viewOriginMove;

	// The current grid size
	float _grid;

	// TRUE if the grid is active
	bool _gridActive;

public:
	TexTool();

	/** greebo: Some sort of "soft" destructor that de-registers
	 * this class from the SelectionSystem, saves the window state, etc.
	 */
	void shutdown();

	/** greebo: This is the static accessor method containing
	 * the static instance of the TexTool class. Use this to access
	 * the public member methods like toggle() and shutdown().
	 */
	static TexTool& Instance();

	/** greebo: SelectionSystem::Observer implementation. Gets called by
	 * the SelectionSystem upon selection change to allow updating.
	 */
	void selectionChanged(scene::Instance& instance, bool isComponent);

	/** greebo: Updates the GL window
	 */
	void draw();

	/** greebo: Increases/Decreases the grid size.
	 */
	void gridUp();
	void gridDown();

	/** greebo: Snaps the current TexTool selection to the active grid.
	 */
	void snapToGrid();

	/** greebo: Merges the selected items in terms of their UV coordinates.
	 * 			Mainly useful for stitching patch control vertices.
	 */
	void mergeSelectedItems();

	/** greebo: This flips the selection in texture space. This is the equivalent
	 * 			of the according buttons in the Surface Inspector, but it only affects
	 * 			the items selected in the TexTool.
	 *
	 * @param axis: 0 = s-Axis flip, 1 = t-axis flip
	 */
	void flipSelected(int axis);

	/** greebo: RegistryKeyObserver implementation, gets called upon key change
	 */
	void keyChanged(const std::string& changedKey, const std::string& newValue);

	/** greebo: Registers the commands in the EventManager
	 */
	static void registerCommands();

private:
	/* TransientWindow callbacks */
	virtual void _preHide();
	virtual void _preShow();

	/** greebo: Helper methods that start/end an undoable operation.
	 */
	void beginOperation();
	void endOperation(const std::string& commandName);

	// Creates, packs and connects the child widgets
	void populateWindow();

	/** greebo: Calculates the extents the selection has in
	 * texture space.
	 *
	 * @returns: the reference to the internal AABB with the z-component set to 0.
	 * 			 Can return an invalid AABB as well (if no selection was found).
	 */
	AABB& getExtents();

	/** greebo: Returns the AABB of the currently visible texture space.
	 */
	AABB& getVisibleTexSpace();

	/** greebo: Recalculates and relocates the visible texture space.
	 * 			It's basically centered onto the current selection
	 * 			with the extents multiplied by the default zoomFactor.
	 */
	void recalculateVisibleTexSpace();

	/** greebo: Visualises the U/V coordinates by drawing the points
	 * into the "texture space".
	 */
	void drawUVCoords();

	/** greebo: Draws the grid into the visible texture space.
	 */
	void drawGrid();

	/** greebo: Loads all the relevant data from the
	 * selectionsystem and prepares the member variables for drawing.
	 */
	void update();

	/** greebo: Removes all selectable items and rescans the scene
	 * 			for selected brushes/faces/patches.
	 */
	void rescanSelection();

	/** Deallocates the textool items
	 */
	void clearItemList();

	/** greebo: Passes the given visitor to every Item in the hierarchy.
	 */
	void foreachItem(textool::ItemVisitor& visitor);

	/** greebo: Returns the number of selected TexToolItems.
	 */
	int countSelected();

	/** greebo: Sets all selectables to <selected>
	 * 			If selected == false and no items are selected,
	 * 			the call is propagated to the main window to
	 * 			trigger a scene deselection.
	 */
	bool setAllSelected(bool selected);

	/** greebo: Returns a list of selectables for the given rectangle.
	 */
	textool::TexToolItemVec getSelectables(const textool::Rectangle& rectangle);

	/** greebo: Returns a list of selectables for the given point.
	 * (A small rectangle is constructed to perform the selection test)
	 */
	textool::TexToolItemVec getSelectables(const Vector2& coords);

	/** greebo: These two get called by the GTK callback and handle the event
	 *
	 * @param coords: this has already been converted into texture space.
	 */
	void doMouseUp(const Vector2& coords, GdkEventButton* event);
	void doMouseDown(const Vector2& coords, GdkEventButton* event);
	void doMouseMove(const Vector2& coords, GdkEventMotion* event);

	/** greebo: Converts the mouse/window coordinates into texture coords.
	 */
	Vector2 getTextureCoords(const double& x, const double& y);

	// The callback for the delete event (toggles the visibility)
	static gboolean onDelete(GtkWidget* widget, GdkEvent* event, TexTool* self);
	static gboolean onExpose(GtkWidget* widget, GdkEventExpose* event, TexTool* self);
	static gboolean triggerRedraw(GtkWidget* widget, GdkEventFocus* event, TexTool* self);
	static void onSizeAllocate(GtkWidget* widget, GtkAllocation* allocation, TexTool* self);

	// The callbacks for capturing the mouse events
	static gboolean onMouseUp(GtkWidget* widget, GdkEventButton* event, TexTool* self);
	static gboolean onMouseDown(GtkWidget* widget, GdkEventButton* event, TexTool* self);
	static gboolean onMouseMotion(GtkWidget* widget, GdkEventMotion* event, TexTool* self);

	// The callback for mouse scroll events
	static gboolean onMouseScroll(GtkWidget* widget, GdkEventScroll* event, TexTool* self);

	// The static keyboard callback to catch the ESC key
	static gboolean onKeyPress(GtkWindow* window, GdkEventKey* event, TexTool* self);

	/** greebo: Static command targets
	 */
	/** greebo: Toggles the visibility of this TexTool instance.
	 * The actual static instance is owned by the Instance() method.
	 */
	static void toggle();
	static void texToolGridUp();
	static void texToolGridDown();
	static void texToolSnapToGrid();
	static void texToolMergeItems();
	static void texToolFlipS();
	static void texToolFlipT();
}; // class TexTool

} // namespace ui

#endif /*TEXTOOL_H_*/
