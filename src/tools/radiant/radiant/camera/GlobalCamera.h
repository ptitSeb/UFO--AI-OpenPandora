#ifndef GLOBALCAMERA_H_
#define GLOBALCAMERA_H_

#include <list>
#include "icamera.h"
#include "gtkutil/widget.h"

#include "CamWnd.h"
#include "CameraObserver.h"

/* greebo: This is the gateway class to access the currently active CamWindow
 *
 * This class provides an interface for creating and deleting CamWnd instances
 * as well as some methods that are passed to the currently active CamWnd, like
 * resetCameraAngles() or lookThroughSelected().
 *
 * The active CamWnd class is referenced by the _camWnd member pointer. */

class GlobalCameraManager
{

		// The currently active camera window
		CamWnd* _camWnd;

		CameraModel* _cameraModel;

		ToggleShown _cameraShown;

		// The connected callbacks (get invoked when movedNotify() is called)
		CameraObserverList _cameraObservers;

	public:

		// Constructor
		GlobalCameraManager ();

		void construct();

		// Creates a new CamWnd class and returns the according pointer
		CamWnd* newCamWnd ();

		// Specifies the parent window of the given CamWnd
		void setParent (CamWnd* camwnd, GtkWindow* parent);

		// Frees the created CamWnd class
		void deleteCamWnd (CamWnd* camWnd);

		// Retrieves/Sets the pointer to the current CamWnd
		CamWnd* getCamWnd ();
		void setCamWnd (CamWnd* camWnd);

		// Resets the camera angles of the currently active Camera
		void resetCameraAngles ();

		// Increases/decreases the far clip plane distance (passes the call to CamWnd)
		void cubicScaleIn ();
		void cubicScaleOut ();

		// Change the floor up/down, passes the call on to the CamWnd class
		void changeFloorUp ();
		void changeFloorDown ();

		/* greebo: Tries to get a CameraModel from the most recently selected instance
		 * Note: Currently NO instances are supporting the InstanceTypeCast onto a
		 * CameraModel, so actually these functions don't do anything. I'll leave them
		 * where they are, they should work in principle... */
		void lookThroughSelected ();
		void lookThroughCamera ();

		void update ();

		// Add a "CameraMoved" callback to the signal member
		void addCameraObserver (CameraObserver* observer);
		void removeCameraObserver(CameraObserver* observer);

		// Notify the attached "CameraMoved" callbacks
		void movedNotify ();

		void destroy ();

		// Movement commands (the calls are passed on to the Camera class)
		void moveForwardDiscrete ();
		void moveBackDiscrete ();
		void moveUpDiscrete ();
		void moveDownDiscrete ();
		void moveLeftDiscrete ();
		void moveRightDiscrete ();
		void rotateLeftDiscrete ();
		void rotateRightDiscrete ();
		void pitchUpDiscrete ();
		void pitchDownDiscrete ();

	private:
		void freelookMoveForwardKeyUp();
		void freelookMoveForwardKeyDown();

		void freelookMoveBackKeyUp();
		void freelookMoveBackKeyDown();

		void freelookMoveLeftKeyUp();
		void freelookMoveLeftKeyDown();

		void freelookMoveRightKeyUp();
		void freelookMoveRightKeyDown();

		void freelookMoveUpKeyUp();
		void freelookMoveUpKeyDown();

		void freelookMoveDownKeyUp();
		void freelookMoveDownKeyDown();

}; // class GlobalCameraManager

// The accessor function that contains the static instance of the GlobalCameraManager class
GlobalCameraManager& GlobalCamera ();

#endif /*GLOBALCAMERA_H_*/
