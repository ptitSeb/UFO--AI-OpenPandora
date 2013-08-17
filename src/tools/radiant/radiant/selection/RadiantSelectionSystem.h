#ifndef RADIANTSELECTIONSYSTEM_H_
#define RADIANTSELECTIONSYSTEM_H_

#include "iselection.h"
#include "math/matrix.h"
#include "gtkutil/event/SingleIdleCallback.h"
#include "signal/signal.h"
#include "Manipulators.h"

/* greebo: This can be tricky to understand (and I don't know if I do :D), but I'll try:
 *
 * This is the implementation of the Abstract Base Class SelectionSystem, so this is the point where
 * the selected instances are kept track of. It provides functions to select points and areas, and makes
 * sure that the according instances are being selected on mouse release. The mouse calls are handled by
 * the RadiantWindowObserver (or its helper classes SelectObserver and ManipulateObserver). The observers
 * call the routines here like SelectPoint() and SelectArea().
 *
 * Basically, this class responds to the calls that come from RadiantWindowObserver. If the user clicks
 * anything, the mouse callback function of the WindowObserverSystem is invoked. These observer routines
 * call the methods of the selectionsystem.
 *
 * For example, if the left mouse button is pressed, the Observer onMouseDown calls the SelectManipulator
 * method of this system, which itself makes use of the Manipulator classes and their testSelect routines.
 *
 * On mouse release, the according manipulator methods are invoked and these in turn call back here. The
 * actual transformations are performed using TransformationVisitors like TranslateSelected or such.
 *
 * Note that the RadiantSelectionSystem class derives from all the possible Manipulatables (Translatable,
 * Rotatable, Scalable). This way the system can pass _itself_ to the Manipulator constructors, which call
 * their Manipulatable's Transform() method, which in turn calls the RadiantSelectionSystem's
 * method translate() (in the example of translation). So basically the dog seems to be hunting
 * its own tail here, but then again, it doesn't.
 *
 */

// RadiantSelectionSystem
class RadiantSelectionSystem :
	public SelectionSystem,
	public Translatable,
	public Rotatable,
	public Scalable,
	public Renderable,
	protected gtkutil::SingleIdleCallback
{
	mutable Matrix4 _pivot2world;
	Matrix4 _pivot2worldStart;
	Matrix4 _manip2pivotStart;

	typedef std::list<Observer*> ObserverList;
	ObserverList _observers;

	Translation _translation;	// This is a vector3
	Rotation _rotation;			// This is a quaternion
	Scale _scale;				// This is a vector3
public:
	static Shader* _state;
private:

	selection::WorkZone _workZone;
	SelectionInfo _selectionInfo;

	EManipulatorMode _manipulatorMode;
	// The currently active manipulator
	Manipulator* _manipulator;

	// state
	bool _requestWorkZoneRecalculation;
	bool _undoBegun;
	EMode _mode;
	EComponentMode _componentMode;

	std::size_t _countPrimitive;
	std::size_t _countComponent;

	// The possible manipulators
	TranslateManipulator _translateManipulator;
	RotateManipulator _rotateManipulator;
	ScaleManipulator _scaleManipulator;
	DragManipulator _dragManipulator;
	ClipManipulator _clipManipulator;

	// The internal list to keep track of the selected instances (components and primitives)
	typedef SelectionList<scene::Instance> SelectionListType;
	SelectionListType _selection;
	SelectionListType _componentSelection;

	Signal1<const Selectable&> _selectionChangedCallbacks;

	void ConstructPivot() const;
	mutable bool _pivotChanged;
	bool _pivotMoving;

	void Scene_TestSelect(Selector& selector, SelectionTest& test, const View& view, SelectionSystem::EMode mode, SelectionSystem::EComponentMode componentMode);
	bool nothingSelected() const;

public:

	RadiantSelectionSystem();

	/** greebo: Returns a structure with all the related
	 * information about the current selection (brush count,
	 * entity count, etc.)
	 */
	const SelectionInfo& getSelectionInfo ();

	void pivotChanged() const;
	typedef ConstMemberCaller<RadiantSelectionSystem, &RadiantSelectionSystem::pivotChanged> PivotChangedCaller;

	void pivotChangedSelection(const Selectable& selectable);
	typedef MemberCaller1<RadiantSelectionSystem, const Selectable&, &RadiantSelectionSystem::pivotChangedSelection> PivotChangedSelectionCaller;

	void addObserver(Observer* observer);
	void removeObserver(Observer* observer);

	void SetMode(EMode mode);
	EMode Mode() const;

	void SetComponentMode(EComponentMode mode);
	EComponentMode ComponentMode() const;

	void SetManipulatorMode(EManipulatorMode mode);
	EManipulatorMode ManipulatorMode() const;

	std::size_t countSelected() const;
	std::size_t countSelectedEntities() const;
	std::size_t countSelectedComponents() const;

	std::size_t countSelectedFaces () const;
	bool areFacesSelected () const;

	void onSelectedChanged(scene::Instance& instance, const Selectable& selectable);
	void onComponentSelection(scene::Instance& instance, const Selectable& selectable);

	scene::Instance& ultimateSelected() const;
	scene::Instance& penultimateSelected() const;

	void setSelectedAll(bool selected);
	void setSelectedAllComponents(bool selected);

	void foreachSelected(const Visitor& visitor) const;
	void foreachSelectedComponent(const Visitor& visitor) const;

	void addSelectionChangeCallback(const SelectionChangeHandler& handler);

	void startMove();

	bool SelectManipulator(const View& view, const float device_point[2], const float device_epsilon[2]);

	void deselectAll();

	void SelectPoint(const View& view, const float device_point[2], const float device_epsilon[2], EModifier modifier, bool face);
	void SelectArea(const View& view, const float device_point[2], const float device_delta[2], EModifier modifier, bool face);

	// These are the "callbacks" that are used by the Manipulatables
	void translate(const Vector3& translation);
	void rotate(const Quaternion& rotation);
	void scale(const Vector3& scaling);

	void outputTranslation(TextOutputStream& ostream);
	void outputRotation(TextOutputStream& ostream);
	void outputScale(TextOutputStream& ostream);

	void rotateSelected(const Quaternion& rotation);
	void translateSelected(const Vector3& translation);
	void scaleSelected(const Vector3& scaling);

	void MoveSelected(const View& view, const float device_point[2]);

	/// \todo Support view-dependent nudge.
	void NudgeManipulator(const Vector3& nudge, const Vector3& view);

	void endMove();
	void freezeTransforms();

	void onBoundsChanged();

	const selection::WorkZone& getWorkZone();

	void renderSolid(Renderer& renderer, const VolumeTest& volume) const;
	void renderWireframe(Renderer& renderer, const VolumeTest& volume) const;

	const Matrix4& GetPivot2World() const;

	static void constructStatic();
	static void destroyStatic();

private:

	void notifyObservers(scene::Instance& instance, bool isComponent);

protected:
	// Called when GTK is idle to recalculate the workzone (if necessary)
	virtual void onGtkIdle();
};

#endif /*RADIANTSELECTIONSYSTEM_H_*/
