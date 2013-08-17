#include "CamWnd.h"

#include "iscenegraph.h"
#include "ieventmanager.h"
#include "iclipper.h"

#include <gdk/gdkkeysyms.h>

#include "gtkutil/glwidget.h"
#include "gtkutil/widget.h"
#include "gtkutil/GLWidgetSentry.h"

#include "../windowobservers.h"
#include "../plugin.h"
#include "../ui/mainframe/mainframe.h"
#include "../renderer.h"
#include "../render/RenderStatistics.h"

#include "CamRenderer.h"
#include "CameraSettings.h"
#include "GlobalCamera.h"

class ObjectFinder: public scene::Graph::Walker
{
		scene::Instance*& _instance;
		SelectionTest& _selectionTest;

		// To store the best intersection candidate
		mutable SelectionIntersection _bestIntersection;
	public:
		// Constructor
		ObjectFinder (SelectionTest& test, scene::Instance*& instance) :
			_instance(instance), _selectionTest(test)
		{
			_instance = NULL;
		}

		// The visitor function
		bool pre (const scene::Path& path, scene::Instance& instance) const
		{
			// Check if the node is filtered
			if (path.top().get().visible()) {
				SelectionTestable* selectionTestable = Instance_getSelectionTestable(instance);

				if (selectionTestable != NULL) {
					bool occluded;
					OccludeSelector selector(_bestIntersection, occluded);
					selectionTestable->testSelect(selector, _selectionTest);

					if (occluded) {
						_instance = &instance;
					}
				}
			}

			return true;
		}
};


inline WindowVector windowvector_for_widget_centre(GtkWidget* widget) {
	return WindowVector(static_cast<float>(widget->allocation.width / 2), static_cast<float>(widget->allocation.height / 2));
}

class FloorHeightWalker : public scene::Graph::Walker
{
	float m_current;
	float& m_bestUp;
	float& m_bestDown;

public:
	FloorHeightWalker(float current, float& bestUp, float& bestDown) :
			m_current(current), m_bestUp(bestUp), m_bestDown(bestDown) {
		bestUp = GlobalRegistry().getFloat("game/defaults/maxWorldCoord");
		bestDown = -GlobalRegistry().getFloat("game/defaults/maxWorldCoord");
	}

	bool pre(const scene::Path& path, scene::Instance& instance) const {
		if (path.top().get().visible() && Node_isBrush(path.top())) // this node is a floor
		{

			const AABB& aabb = instance.worldAABB();

			float floorHeight = aabb.origin.z() + aabb.extents.z();

			if (floorHeight > m_current && floorHeight < m_bestUp) {
				m_bestUp = floorHeight;
			}

			if (floorHeight < m_current && floorHeight > m_bestDown) {
				m_bestDown = floorHeight;
			}
		}

		return true;
	}
};

// --------------- Callbacks ---------------------------------------------------------

void selection_motion(gdouble x, gdouble y, guint state, void* data) {
	//globalOutputStream() << "motion... ";
	reinterpret_cast<WindowObserver*>(data)->onMouseMotion(WindowVector(x, y), state);
}

void camwnd_update_xor_rectangle(CamWnd& self, Rectangle area) {
	if (GTK_WIDGET_VISIBLE(self.m_gl_widget)) {
		self.m_XORRectangle.set(rectangle_from_area(area.min, area.max, self.getCamera().width, self.getCamera().height));
	}
}

gboolean CamWnd::camera_size_allocate(GtkWidget* widget, GtkAllocation* allocation, CamWnd* camwnd) {
	camwnd->getCamera().width = allocation->width;
	camwnd->getCamera().height = allocation->height;
	camwnd->getCamera().updateProjection();
	camwnd->m_window_observer->onSizeChanged(camwnd->getCamera().width, camwnd->getCamera().height);
	camwnd->queueDraw();
	return FALSE;
}

gboolean CamWnd::camera_expose(GtkWidget* widget, GdkEventExpose* event, gpointer data) {
	reinterpret_cast<CamWnd*>(data)->draw();
	return FALSE;
}

void Camera_motionDelta(int x, int y, unsigned int state, void* data) {
	Camera* cam = reinterpret_cast<Camera*>(data);

	cam->m_mouseMove.motion_delta(x, y, state);
	cam->m_strafe = GlobalEventManager().MouseEvents().strafeActive(state);

	if (cam->m_strafe) {
		cam->m_strafe_forward = GlobalEventManager().MouseEvents().strafeForwardActive(state);
	} else {
		cam->m_strafe_forward = false;
	}
}

// greebo: The GTK Callback during freemove mode for mouseDown. Passes the call on to the Windowobserver
gboolean CamWnd::selection_button_press_freemove(GtkWidget* widget, GdkEventButton* event, WindowObserver* observer) {
	// Check for the correct event type
	if (event->type == GDK_BUTTON_PRESS) {
		observer->onMouseDown(windowvector_for_widget_centre(widget), event);
	}
	return FALSE;
}

// greebo: The GTK Callback during freemove mode for mouseUp. Passes the call on to the Windowobserver
gboolean CamWnd::selection_button_release_freemove(GtkWidget* widget, GdkEventButton* event, WindowObserver* observer) {
	if (event->type == GDK_BUTTON_RELEASE) {
		observer->onMouseUp(windowvector_for_widget_centre(widget), event);
	}
	return FALSE;
}

// greebo: The GTK Callback during freemove mode for mouseMoved. Passes the call on to the Windowobserver
gboolean CamWnd::selection_motion_freemove(GtkWidget *widget, GdkEventMotion *event, WindowObserver* observer) {
	observer->onMouseMotion(windowvector_for_widget_centre(widget), event->state);
	return FALSE;
}

// greebo: The GTK Callback during freemove mode for scroll events.
gboolean CamWnd::wheelmove_scroll(GtkWidget* widget, GdkEventScroll* event, CamWnd* camwnd) {

	// Set the GTK focus to this widget
	gtk_widget_grab_focus(widget);

	// Determine the direction we are moving.
	if (event->direction == GDK_SCROLL_UP) {
		camwnd->getCamera().freemoveUpdateAxes();
		camwnd->setCameraOrigin(camwnd->getCameraOrigin() + camwnd->getCamera().forward * static_cast<float>(getCameraSettings()->movementSpeed()));
	}
	else if (event->direction == GDK_SCROLL_DOWN) {
		camwnd->getCamera().freemoveUpdateAxes();
		camwnd->setCameraOrigin(camwnd->getCameraOrigin() + camwnd->getCamera().forward * (-static_cast<float>(getCameraSettings()->movementSpeed())));
	}

	return FALSE;
}

/* greebo: GTK Callback: This gets called on "button_press_event" and basically just passes the call on
 * to the according window observer. */
gboolean CamWnd::selection_button_press(GtkWidget* widget, GdkEventButton* event, WindowObserver* observer) {

	// Set the GTK focus to this widget
	gtk_widget_grab_focus(widget);

	// Check for the correct event type
	if (event->type == GDK_BUTTON_PRESS) {
		observer->onMouseDown(WindowVector(event->x, event->y), event);
	}
	return FALSE;
}

/* greebo: GTK Callback: This gets called on "button_release_event" and basically just passes the call on
 * to the according window observer. */
gboolean CamWnd::selection_button_release(GtkWidget* widget, GdkEventButton* event, WindowObserver* observer) {
	if (event->type == GDK_BUTTON_RELEASE) {
		observer->onMouseUp(WindowVector(event->x, event->y), event);
	}
	return FALSE;
}

gboolean CamWnd::enable_freelook_button_press(GtkWidget* widget, GdkEventButton* event, CamWnd* camwnd) {
	if (event->type == GDK_BUTTON_PRESS) {

		if (GlobalEventManager().MouseEvents().stateMatchesCameraViewEvent(ui::camEnableFreeLookMode, event)) {
			camwnd->enableFreeMove();
			return TRUE;
		}
	}
	return FALSE;
}

gboolean CamWnd::disable_freelook_button_press(GtkWidget* widget, GdkEventButton* event, CamWnd* camwnd) {
	if (event->type == GDK_BUTTON_PRESS) {
		if (GlobalEventManager().MouseEvents().stateMatchesCameraViewEvent(ui::camDisableFreeLookMode, event)) {
			camwnd->disableFreeMove();
			return TRUE;
		}
	}
	return FALSE;
}

// ---------- CamWnd Implementation --------------------------------------------------

CamWnd::CamWnd() :
		m_view(true),
		m_Camera(&m_view, CamWndQueueDraw(*this)),
		m_cameraview(m_Camera, &m_view, CamWndUpdate(*this)),
		m_drawing(false),
		_glWidget(true), m_gl_widget(static_cast<GtkWidget*>(_glWidget)),
		m_window_observer(NewWindowObserver()),
		m_XORRectangle(m_gl_widget),
		m_deferredDraw(WidgetQueueDrawCaller(*m_gl_widget)),
		m_deferred_motion(selection_motion, m_window_observer),
		m_selection_button_press_handler(0),
		m_selection_button_release_handler(0),
		m_selection_motion_handler(0),
		m_freelook_button_press_handler(0) {
	m_bFreeMove = false;

	GlobalWindowObservers_add(m_window_observer);
	GlobalWindowObservers_connectWidget(m_gl_widget);

	m_window_observer->setRectangleDrawCallback(ReferenceCaller1<CamWnd, Rectangle, camwnd_update_xor_rectangle>(*this));
	m_window_observer->setView(m_view);

	gtk_widget_ref(m_gl_widget);

	gtk_widget_set_events(m_gl_widget, GDK_DESTROY | GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK);
	GTK_WIDGET_SET_FLAGS (m_gl_widget, GTK_CAN_FOCUS);
	gtk_widget_set_size_request(m_gl_widget, CAMWND_MINSIZE_X, CAMWND_MINSIZE_Y);
	g_object_set(m_gl_widget, "can-focus", TRUE, NULL);

	m_sizeHandler = g_signal_connect(G_OBJECT(m_gl_widget), "size_allocate", G_CALLBACK(camera_size_allocate), this);
	m_exposeHandler = g_signal_connect(G_OBJECT(m_gl_widget), "expose_event", G_CALLBACK(camera_expose), this);

	GlobalMap().addValidCallback(DeferredDrawOnMapValidChangedCaller(m_deferredDraw));

	// Deactivate all commands, just to make sure
	disableDiscreteMoveEvents();
	disableFreeMoveEvents();

	// Now add the handlers for the non-freelook mode, the events are activated by this
	addHandlersMove();

	g_signal_connect(G_OBJECT(m_gl_widget), "scroll_event", G_CALLBACK(wheelmove_scroll), this);

	GlobalSceneGraph().addSceneObserver(this);

	GlobalEventManager().connect(GTK_OBJECT(m_gl_widget));
}

void CamWnd::onSceneGraphChange ()
{
	update();
}

void CamWnd::jumpToObject(SelectionTest& selectionTest) {
	// Find a suitable target Instance
	scene::Instance* instance;
	GlobalSceneGraph().traverse(ObjectFinder(selectionTest, instance));

	if (instance != NULL) {
		// An instance has been found, get the bounding box
		AABB found = instance->worldAABB();

		// Focuse the view at the center of the found AABB
		GlobalMap().FocusViews(found.origin, getCameraAngles()[CAMERA_YAW]);
	}
}

void CamWnd::changeFloor(bool up) {
	float current = m_Camera.getOrigin()[2] - 48;
	float bestUp;
	float bestDown;
	GlobalSceneGraph().traverse(FloorHeightWalker(current, bestUp, bestDown));

	if (up && bestUp != GlobalRegistry().getFloat("game/defaults/maxWorldCoord")) {
		current = bestUp;
	}

	if (!up && bestDown != -GlobalRegistry().getFloat("game/defaults/maxWorldCoord")) {
		current = bestDown;
	}

	const Vector3& org = m_Camera.getOrigin();
	m_Camera.setOrigin(Vector3(org[0], org[1], current + 48));

	m_Camera.updateModelview();
	update();
	GlobalCamera().movedNotify();
}

// NOTE TTimo if there's an OS-level focus out of the application
//   then we can release the camera cursor grab
static gboolean camwindow_freemove_focusout(GtkWidget* widget, GdkEventFocus* event, gpointer data) {
	reinterpret_cast<CamWnd*>(data)->disableFreeMove();
	return FALSE;
}

void CamWnd::enableFreeMove() {
	ASSERT_MESSAGE(!m_bFreeMove, "EnableFreeMove: free-move was already enabled");
	m_bFreeMove = true;
	m_Camera.clearMovementFlags(MOVE_ALL);

	removeHandlersMove();

	m_selection_button_press_handler = g_signal_connect(G_OBJECT(m_gl_widget), "button_press_event", G_CALLBACK(selection_button_press_freemove), m_window_observer);
	m_selection_button_release_handler = g_signal_connect(G_OBJECT(m_gl_widget), "button_release_event", G_CALLBACK(selection_button_release_freemove), m_window_observer);
	m_selection_motion_handler = g_signal_connect(G_OBJECT(m_gl_widget), "motion_notify_event", G_CALLBACK(selection_motion_freemove), m_window_observer);
	m_freelook_button_press_handler = g_signal_connect(G_OBJECT(m_gl_widget), "button_press_event", G_CALLBACK(disable_freelook_button_press), this);

	enableFreeMoveEvents();

	gtk_window_set_focus(m_parent, m_gl_widget);
	m_freemove_handle_focusout = g_signal_connect(G_OBJECT(m_gl_widget), "focus_out_event", G_CALLBACK(camwindow_freemove_focusout), this);
	m_freezePointer.freeze_pointer(m_parent, Camera_motionDelta, &m_Camera);

	update();
}

void CamWnd::disableFreeMove() {
	ASSERT_MESSAGE(m_bFreeMove, "DisableFreeMove: free-move was not enabled");
	m_bFreeMove = false;
	m_Camera.clearMovementFlags(MOVE_ALL);

	disableFreeMoveEvents();

	g_signal_handler_disconnect(G_OBJECT(m_gl_widget), m_selection_button_press_handler);
	g_signal_handler_disconnect(G_OBJECT(m_gl_widget), m_selection_button_release_handler);
	g_signal_handler_disconnect(G_OBJECT(m_gl_widget), m_selection_motion_handler);
	g_signal_handler_disconnect(G_OBJECT(m_gl_widget), m_freelook_button_press_handler);

	addHandlersMove();

	m_freezePointer.unfreeze_pointer(m_parent);
	g_signal_handler_disconnect(G_OBJECT(m_gl_widget), m_freemove_handle_focusout);

	update();
}

void CamWnd::Cam_Draw() {
	glViewport(0, 0, m_Camera.width, m_Camera.height);

	// enable depth buffer writes
	glDepthMask(GL_TRUE);

	Vector3 clearColour(0, 0, 0);
	clearColour = ColourSchemes().getColourVector3("camera_background");

	glClearColor(clearColour[0], clearColour[1], clearColour[2], 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	render::RenderStatistics::Instance().resetStats();

	extern void Cull_ResetStats ();
	Cull_ResetStats();

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(m_Camera.projection);

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(m_Camera.modelview);

	// one directional light source directly behind the viewer
	{
		GLfloat inverse_cam_dir[4], ambient[4], diffuse[4];//, material[4];

		ambient[0] = ambient[1] = ambient[2] = 0.4f;
		ambient[3] = 1.0f;
		diffuse[0] = diffuse[1] = diffuse[2] = 0.4f;
		diffuse[3] = 1.0f;

		inverse_cam_dir[0] = m_Camera.vpn[0];
		inverse_cam_dir[1] = m_Camera.vpn[1];
		inverse_cam_dir[2] = m_Camera.vpn[2];
		inverse_cam_dir[3] = 0;

		glLightfv(GL_LIGHT0, GL_POSITION, inverse_cam_dir);

		glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

		glEnable(GL_LIGHT0);
	}

	unsigned int globalstate = RENDER_DEPTHTEST | RENDER_COLOURWRITE | RENDER_DEPTHWRITE | RENDER_ALPHATEST
			| RENDER_BLEND | RENDER_CULLFACE | RENDER_COLOURARRAY | RENDER_COLOURCHANGE;
	switch (getCameraSettings()->getMode()) {
	case drawWire:
		break;
	case drawSolid:
		globalstate |= (RENDER_FILL | RENDER_LIGHTING | RENDER_SMOOTH | RENDER_SCALED);
		break;
	case drawTexture:
		globalstate |= (RENDER_FILL | RENDER_LIGHTING | RENDER_TEXTURE_2D | RENDER_SMOOTH | RENDER_SCALED);
		break;
	default:
		globalstate = 0;
		break;
	}

	if (!getCameraSettings()->solidSelectionBoxes()) {
		globalstate |= RENDER_LINESTIPPLE;
	}

	{
		CamRenderer renderer(globalstate, m_state_select2, m_state_select1, m_view.getViewer());

		Scene_Render(renderer, m_view);

		renderer.render(m_Camera.modelview, m_Camera.projection);
	}

	// greebo: Draw the clipper's points (skipping the depth-test)
	{
		glDisable(GL_DEPTH_TEST);

		glColor4f(1, 1, 1, 1);

		glPointSize(5);

		if (GlobalClipper().clipMode()) {
			GlobalClipper().draw(1.0f);
		}

		glPointSize(1);
	}

	// prepare for 2d stuff
	glColor4f(1, 1, 1, 1);
	glDisable(GL_BLEND);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, (float) m_Camera.width, 0, (float) m_Camera.height, -100, 100);
	glScalef(1, -1, 1);
	glTranslatef(0, -(float) m_Camera.height, 0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if (GlobalOpenGL().GL_1_3()) {
		glClientActiveTexture(GL_TEXTURE0);
		glActiveTexture(GL_TEXTURE0);
	}

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_COLOR_MATERIAL);
	glDisable(GL_DEPTH_TEST);
	glLineWidth(1);

	// draw the crosshair
	if (m_bFreeMove) {
		glBegin(GL_LINES);
		glVertex2f((float) m_Camera.width / 2.f, (float) m_Camera.height / 2.f + 6);
		glVertex2f((float) m_Camera.width / 2.f, (float) m_Camera.height / 2.f + 2);
		glVertex2f((float) m_Camera.width / 2.f, (float) m_Camera.height / 2.f - 6);
		glVertex2f((float) m_Camera.width / 2.f, (float) m_Camera.height / 2.f - 2);
		glVertex2f((float) m_Camera.width / 2.f + 6, (float) m_Camera.height / 2.f);
		glVertex2f((float) m_Camera.width / 2.f + 2, (float) m_Camera.height / 2.f);
		glVertex2f((float) m_Camera.width / 2.f - 6, (float) m_Camera.height / 2.f);
		glVertex2f((float) m_Camera.width / 2.f - 2, (float) m_Camera.height / 2.f);
		glEnd();
	}

	glRasterPos3f(1.0f, static_cast<float> (m_Camera.height) - 1.0f, 0.0f);
	GlobalOpenGL().drawString(render::RenderStatistics::Instance().getStatString());

	glRasterPos3f(1.0f, static_cast<float> (m_Camera.height) - 11.0f, 0.0f);
	extern const char* Cull_GetStats ();
	GlobalOpenGL().drawString(Cull_GetStats());

	// bind back to the default texture so that we don't have problems
	// elsewhere using/modifying texture maps between contexts
	glBindTexture(GL_TEXTURE_2D, 0);
}

void CamWnd::draw() {
	m_drawing = true;

	gtkutil::GLWidgetSentry sentry(m_gl_widget);
	if (GlobalMap().isValid() && ScreenUpdates_Enabled()) {
		Cam_Draw();

		m_XORRectangle.set(rectangle_t());
	}

	m_drawing = false;
}

CamWnd::~CamWnd() {
	// Subscribe to the global scene graph update
	GlobalSceneGraph().removeSceneObserver(this);

	if (m_bFreeMove) {
		disableFreeMove();
	}

	removeHandlersMove();

	g_signal_handler_disconnect(G_OBJECT(m_gl_widget), m_sizeHandler);
	g_signal_handler_disconnect(G_OBJECT(m_gl_widget), m_exposeHandler);

	gtk_widget_unref(m_gl_widget);

	// Disconnect self from EventManager\r
	GlobalEventManager().disconnect(GTK_OBJECT(m_gl_widget));
	GlobalEventManager().disconnect(GTK_OBJECT(m_parent));

	delete m_window_observer;
}

// ----------------------------------------------------------

void CamWnd::enableFreeMoveEvents() {
	GlobalEventManager().enableEvent("CameraFreeMoveForward");
	GlobalEventManager().enableEvent("CameraFreeMoveBack");
	GlobalEventManager().enableEvent("CameraFreeMoveLeft");
	GlobalEventManager().enableEvent("CameraFreeMoveRight");
	GlobalEventManager().enableEvent("CameraFreeMoveUp");
	GlobalEventManager().enableEvent("CameraFreeMoveDown");
}

void CamWnd::disableFreeMoveEvents() {
	GlobalEventManager().disableEvent("CameraFreeMoveForward");
	GlobalEventManager().disableEvent("CameraFreeMoveBack");
	GlobalEventManager().disableEvent("CameraFreeMoveLeft");
	GlobalEventManager().disableEvent("CameraFreeMoveRight");
	GlobalEventManager().disableEvent("CameraFreeMoveUp");
	GlobalEventManager().disableEvent("CameraFreeMoveDown");
}

void CamWnd::enableDiscreteMoveEvents() {
	GlobalEventManager().enableEvent("CameraForward");
	GlobalEventManager().enableEvent("CameraBack");
	GlobalEventManager().enableEvent("CameraLeft");
	GlobalEventManager().enableEvent("CameraRight");
	GlobalEventManager().enableEvent("CameraStrafeRight");
	GlobalEventManager().enableEvent("CameraStrafeLeft");
	GlobalEventManager().enableEvent("CameraUp");
	GlobalEventManager().enableEvent("CameraDown");
	GlobalEventManager().enableEvent("CameraAngleUp");
	GlobalEventManager().enableEvent("CameraAngleDown");
}

void CamWnd::disableDiscreteMoveEvents() {
	GlobalEventManager().disableEvent("CameraForward");
	GlobalEventManager().disableEvent("CameraBack");
	GlobalEventManager().disableEvent("CameraLeft");
	GlobalEventManager().disableEvent("CameraRight");
	GlobalEventManager().disableEvent("CameraStrafeRight");
	GlobalEventManager().disableEvent("CameraStrafeLeft");
	GlobalEventManager().disableEvent("CameraUp");
	GlobalEventManager().disableEvent("CameraDown");
	GlobalEventManager().disableEvent("CameraAngleUp");
	GlobalEventManager().disableEvent("CameraAngleDown");
}

void CamWnd::addHandlersMove() {
	m_selection_button_press_handler = g_signal_connect(G_OBJECT(m_gl_widget), "button_press_event", G_CALLBACK(selection_button_press), m_window_observer);
	m_selection_button_release_handler = g_signal_connect(G_OBJECT(m_gl_widget), "button_release_event", G_CALLBACK(selection_button_release), m_window_observer);
	m_selection_motion_handler = g_signal_connect(G_OBJECT(m_gl_widget), "motion_notify_event", G_CALLBACK(DeferredMotion::gtk_motion), &m_deferred_motion);

	m_freelook_button_press_handler = g_signal_connect(G_OBJECT(m_gl_widget), "button_press_event", G_CALLBACK(enable_freelook_button_press), this);

	// Enable either the free-look movement commands or the discrete ones, depending on the selection
	if (getCameraSettings()->discreteMovement()) {
		enableDiscreteMoveEvents();
	} else {
		enableFreeMoveEvents();
	}
}

void CamWnd::removeHandlersMove() {
	g_signal_handler_disconnect(G_OBJECT(m_gl_widget), m_selection_button_press_handler);
	g_signal_handler_disconnect(G_OBJECT(m_gl_widget), m_selection_button_release_handler);
	g_signal_handler_disconnect(G_OBJECT(m_gl_widget), m_selection_motion_handler);

	g_signal_handler_disconnect(G_OBJECT(m_gl_widget), m_freelook_button_press_handler);

	// Disable either the free-look movement commands or the discrete ones, depending on the selection
	if (getCameraSettings()->discreteMovement()) {
		disableDiscreteMoveEvents();
	} else {
		disableFreeMoveEvents();
	}
}

void CamWnd::update() {
	queueDraw();
}

Camera& CamWnd::getCamera() {
	return m_Camera;
}

void CamWnd::captureStates() {
	m_state_select1 = GlobalShaderCache().capture("$CAM_HIGHLIGHT");
	m_state_select2 = GlobalShaderCache().capture("$CAM_OVERLAY");
}

void CamWnd::releaseStates() {
	GlobalShaderCache().release("$CAM_HIGHLIGHT");
	GlobalShaderCache().release("$CAM_OVERLAY");
}

void CamWnd::queueDraw() {
	if (m_drawing) {
		return;
	}

	m_deferredDraw.draw();
}

const Vector3& CamWnd::getCameraOrigin () const {
	return m_Camera.getOrigin();
}

void CamWnd::setCameraOrigin (const Vector3& origin) {
	m_Camera.setOrigin(origin);
}

const Vector3& CamWnd::getCameraAngles () const {
	return m_Camera.getAngles();
}

void CamWnd::setCameraAngles (const Vector3& angles) {
	m_Camera.setAngles(angles);
}

void CamWnd::cubicScaleIn ()
{
	getCameraSettings()->setCubicScale( getCameraSettings()->cubicScale() - 1 );
	m_Camera.updateProjection();
	update();
}

void CamWnd::cubicScaleOut ()
{
	getCameraSettings()->setCubicScale( getCameraSettings()->cubicScale() + 1 );
	m_Camera.updateProjection();
	update();
}

CameraView* CamWnd::getCameraView() {
	return &m_cameraview;
}

GtkWidget* CamWnd::getWidget()
{
	return m_gl_widget;
}

void CamWnd::setParent(GtkWindow* parent) {
	m_parent = parent;
	GlobalEventManager().connect(GTK_OBJECT(m_parent));
}

GtkWindow* CamWnd::getParent() {
	return m_parent;
}

Shader* CamWnd::m_state_select1 = 0;
Shader* CamWnd::m_state_select2 = 0;
