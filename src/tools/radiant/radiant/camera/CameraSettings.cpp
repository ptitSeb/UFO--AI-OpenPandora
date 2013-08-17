#include "CameraSettings.h"

#include "ieventmanager.h"

#include "radiant_i18n.h"

#include "GlobalCamera.h"
#include "CamWnd.h"
#include "string/string.h"
#include <iostream>

CameraSettings::CameraSettings () :
	_callbackActive(false), _movementSpeed(GlobalRegistry().getInt(RKEY_MOVEMENT_SPEED)), _angleSpeed(
			GlobalRegistry().getInt(RKEY_ROTATION_SPEED)), _invertMouseVerticalAxis(GlobalRegistry().get(
			RKEY_INVERT_MOUSE_VERTICAL_AXIS) == "1"), _discreteMovement(GlobalRegistry().get(RKEY_DISCRETE_MOVEMENT)
			== "1"), _cubicScale(GlobalRegistry().getInt(RKEY_CUBIC_SCALE)), _farClipEnabled(GlobalRegistry().get(
			RKEY_ENABLE_FARCLIP) == "1"),
			_solidSelectionBoxes(GlobalRegistry().get(RKEY_SOLID_SELECTION_BOXES) == "1")
{
	// Constrain the cubic scale to a fixed value
	if (_cubicScale > MAX_CUBIC_SCALE) {
		_cubicScale = MAX_CUBIC_SCALE;
	}

	// Initialise the draw mode from the integer value stored in the registry
	importDrawMode(GlobalRegistry().getInt(RKEY_DRAWMODE));

	// Connect self to the according registry keys
	GlobalRegistry().addKeyObserver(this, RKEY_MOVEMENT_SPEED);
	GlobalRegistry().addKeyObserver(this, RKEY_ROTATION_SPEED);
	GlobalRegistry().addKeyObserver(this, RKEY_INVERT_MOUSE_VERTICAL_AXIS);
	GlobalRegistry().addKeyObserver(this, RKEY_DISCRETE_MOVEMENT);
	GlobalRegistry().addKeyObserver(this, RKEY_ENABLE_FARCLIP);
	GlobalRegistry().addKeyObserver(this, RKEY_DRAWMODE);
	GlobalRegistry().addKeyObserver(this, RKEY_SOLID_SELECTION_BOXES);

	// greebo: Register this class in the preference system so that the constructPreferencePage() gets called.
	GlobalPreferenceSystem().addConstructor(this);
}

void CameraSettings::constructPreferencePage(PreferenceGroup& group) {
	// Add a page to the given group
	PreferencesPage* page(group.createPage(_("Camera"), _("Camera View Preferences")));

	// Add the sliders for the movement and angle speed and connect them to the observer
	page->appendSlider(_("Movement Speed (game units)"), RKEY_MOVEMENT_SPEED, TRUE, 100, 50, 300, 1, 10, 10);
	page->appendSlider(_("Rotation Speed"), RKEY_ROTATION_SPEED, TRUE, 3, 1, 180, 1, 10, 10);

	// Add the checkboxes and connect them with the registry key and the according observer
	page->appendCheckBox("", _("Discrete movement (non-freelook mode)"), RKEY_DISCRETE_MOVEMENT);
	page->appendCheckBox("", _("Enable far-clip plane (hides distant objects)"), RKEY_ENABLE_FARCLIP);

	// Add the "inverse mouse vertical axis in free-look mode" preference
	page->appendCheckBox("", _("Invert mouse vertical axis (freelook mode)"), RKEY_INVERT_MOUSE_VERTICAL_AXIS);

	// States whether the selection boxes are stippled or not
	page->appendCheckBox("", _("Solid selection boxes"), RKEY_SOLID_SELECTION_BOXES);

	// Create the string list containing the render mode captions
	std::list<std::string> renderModeDescriptions;
	renderModeDescriptions.push_back(_("WireFrame"));
	renderModeDescriptions.push_back(_("Flatshade"));
	renderModeDescriptions.push_back(_("Textured"));

	page->appendCombo(_("Render Mode"), RKEY_DRAWMODE, renderModeDescriptions);
}

void CameraSettings::importDrawMode (const int mode)
{
	switch (mode) {
	case 0:
		_cameraDrawMode = drawWire;
		break;
	case 1:
		_cameraDrawMode = drawSolid;
		break;
	case 2:
		_cameraDrawMode = drawTexture;
		break;
	default:
		_cameraDrawMode = drawTexture;
	}
}

void CameraSettings::keyChanged (const std::string& changedKey, const std::string& newValue)
{
	// Check for iterative loops
	if (_callbackActive) {
		return;
	} else {
		_callbackActive = true;

		// Load the values from the registry
		_movementSpeed = GlobalRegistry().getInt(RKEY_MOVEMENT_SPEED);
		_angleSpeed = GlobalRegistry().getInt(RKEY_ROTATION_SPEED);
		_invertMouseVerticalAxis = (GlobalRegistry().get(RKEY_INVERT_MOUSE_VERTICAL_AXIS) == "1");
		_farClipEnabled = (GlobalRegistry().get(RKEY_ENABLE_FARCLIP) == "1");
		_solidSelectionBoxes = (GlobalRegistry().get(RKEY_SOLID_SELECTION_BOXES) == "1");

		GlobalEventManager().setToggled("ToggleCubicClip", _farClipEnabled);

		// Determine the draw mode represented by the integer registry value
		importDrawMode(GlobalRegistry().getInt(RKEY_DRAWMODE));

		// Check if a global camwindow is set
		if (GlobalCamera().getCamWnd() != 0) {
			 // Disconnect the handlers for the old state and re-connect after reading the registry value
			GlobalCamera().getCamWnd()->removeHandlersMove();

			// Check the value and take the according actions
			_discreteMovement = (GlobalRegistry().get(RKEY_DISCRETE_MOVEMENT) == "1");

			// Reconnect the new handlers
			GlobalCamera().getCamWnd()->addHandlersMove();

			GlobalCamera().getCamWnd()->getCamera().updateProjection();

			// Call the update method in case the render mode has changed
			GlobalCamera().getCamWnd()->update();
		}
	}
	_callbackActive = false;
}

CameraDrawMode CameraSettings::getMode () const
{
	return _cameraDrawMode;
}

void CameraSettings::setMode (const CameraDrawMode& mode)
{
	// Write the value into the registry, this should trigger the keyChanged() callback that in turn calls the update functions
	GlobalRegistry().setInt(RKEY_DRAWMODE, static_cast<int> (mode));
}

bool CameraSettings::farClipEnabled () const
{
	return _farClipEnabled;
}

bool CameraSettings::solidSelectionBoxes () const
{
	return _solidSelectionBoxes;
}

int CameraSettings::cubicScale () const
{
	return _cubicScale;
}

int CameraSettings::movementSpeed () const
{
	return _movementSpeed;
}

int CameraSettings::angleSpeed () const
{
	return _angleSpeed;
}

bool CameraSettings::invertMouseVerticalAxis () const
{
	return _invertMouseVerticalAxis;
}

bool CameraSettings::discreteMovement () const
{
	return _discreteMovement;
}

void CameraSettings::setCubicScale (const int& scale)
{
	// Update the internal value
	_cubicScale = scale;

	std::string scaleStr = string::toString(_cubicScale);

	GlobalRegistry().set(RKEY_CUBIC_SCALE, scaleStr);

	// Constrain the value to [1..MAX_CUBIC_SCALE]
	if (_cubicScale > MAX_CUBIC_SCALE) {
		_cubicScale = MAX_CUBIC_SCALE;
	}

	if (_cubicScale < 1) {
		_cubicScale = 1;
	}
}

void CameraSettings::setFarClip (bool farClipEnabled)
{
	// Write the value into the registry, the keyChanged() method is automatically triggered hereby
	GlobalRegistry().set(RKEY_ENABLE_FARCLIP, farClipEnabled ? "1" : "0");
}

void CameraSettings::toggleFarClip ()
{
	setFarClip(!_farClipEnabled);
}

CameraSettings* getCameraSettings ()
{
	static CameraSettings _cameraSettings;
	return &_cameraSettings;
}
