#include "ioverlay.h"

#include "iregistry.h"
#include "preferencesystem.h"
#include "igl.h"
#include "itextures.h"

#include "math/Vector3.h"
#include "math/matrix.h"
#include "texturelib.h"
#include "archivelib.h"
#include "radiant_i18n.h"


/* greebo: The Overlay module adds the ability to use an arbitrary image as
 * background image in the orthogonal views.
 *
 * The draw() method gets invoked by the XYView render function and displays
 * the texture with the specified <scale> and <translation>.
 *
 * All the settings are stored in the XMLRegistry and can be accessed via the
 * preferences dialog.
 */

namespace {
	const std::string RKEY_OVERLAY_VISIBLE = "user/ui/xyview/overlay/visible";
	const std::string RKEY_OVERLAY_TRANSPARENCY = "user/ui/xyview/overlay/transparency";
	const std::string RKEY_OVERLAY_IMAGE = "user/ui/xyview/overlay/image";
	const std::string RKEY_OVERLAY_SCALE = "user/ui/xyview/overlay/scale";
	const std::string RKEY_OVERLAY_TRANSLATIONX = "user/ui/xyview/overlay/translationX";
	const std::string RKEY_OVERLAY_TRANSLATIONY = "user/ui/xyview/overlay/translationY";
	const std::string RKEY_OVERLAY_PROPORTIONAL = "user/ui/xyview/overlay/proportional";
	const std::string RKEY_OVERLAY_SCALE_WITH_XY = "user/ui/xyview/overlay/scaleWithOrthoView";
	const std::string RKEY_OVERLAY_PAN_WITH_XY = "user/ui/xyview/overlay/panWithOrthoView";

	const float MIN_SCALE = 0.001f;
	const float MAX_SCALE = 20.0f;
}

class Overlay :
	public IOverlay,
	public RegistryKeyObserver,
	public PreferenceConstructor
{
public:
	// Radiant Module stuff
	typedef IOverlay Type;
	STRING_CONSTANT(Name, "*");

	// Return the static instance
	IOverlay* getTable() {
		return this;
	}

private:

	// The imagename to be drawn
	std::string _imageName;

	// TRUE, if the overlay is visible
	bool _visible;

	// The transparency of the image (0 = completely transparent, 1 = opaque)
	float _transparency;

	// The image scale
	float _scale;

	// TRUE, if the image should be rescaled along with the orthoview scale
	bool _scaleWithXYView;

	// TRUE, if the image should be panned along
	bool _panWithXYView;

	// Set to TRUE, if the image should be displayed proportionally.
	bool _keepProportions;

	// The x,y translation of the texture center
	float _translationX;
	float _translationY;

	// The loaded texture
	GLTexture* _texture;

	// The instance of the GDKModule loader
	ImageModuleRef _imageGDKModule;

public:

	Overlay() :
		_imageName(GlobalRegistry().get(RKEY_OVERLAY_IMAGE)),
		_visible(GlobalRegistry().get(RKEY_OVERLAY_VISIBLE) == "1"),
		_transparency(GlobalRegistry().getFloat(RKEY_OVERLAY_TRANSPARENCY)),
		_scale(GlobalRegistry().getFloat(RKEY_OVERLAY_SCALE)),
		_scaleWithXYView(GlobalRegistry().get(RKEY_OVERLAY_SCALE_WITH_XY) == "1"),
		_panWithXYView(GlobalRegistry().get(RKEY_OVERLAY_PAN_WITH_XY) == "1"),
		_keepProportions(GlobalRegistry().get(RKEY_OVERLAY_PROPORTIONAL) == "1"),
		_translationX(GlobalRegistry().getFloat(RKEY_OVERLAY_TRANSLATIONX)),
		_translationY(GlobalRegistry().getFloat(RKEY_OVERLAY_TRANSLATIONY)),
		_texture(NULL),
		_imageGDKModule("GDK")
	{
		// Watch the relevant registry keys
		GlobalRegistry().addKeyObserver(this, RKEY_OVERLAY_VISIBLE);
		GlobalRegistry().addKeyObserver(this, RKEY_OVERLAY_TRANSPARENCY);
		GlobalRegistry().addKeyObserver(this, RKEY_OVERLAY_IMAGE);
		GlobalRegistry().addKeyObserver(this, RKEY_OVERLAY_SCALE);
		GlobalRegistry().addKeyObserver(this, RKEY_OVERLAY_TRANSLATIONX);
		GlobalRegistry().addKeyObserver(this, RKEY_OVERLAY_TRANSLATIONY);
		GlobalRegistry().addKeyObserver(this, RKEY_OVERLAY_PROPORTIONAL);
		GlobalRegistry().addKeyObserver(this, RKEY_OVERLAY_SCALE_WITH_XY);
		GlobalRegistry().addKeyObserver(this, RKEY_OVERLAY_PAN_WITH_XY);

		// greebo: Register this class in the preference system so that the constructPreferencePage() gets called.
		GlobalPreferenceSystem().addConstructor(this);
	}

	~Overlay() {
		releaseTexture();
	}

	void show(bool shown) {
		_visible = shown;
	}

	// Sets the name of the image that should be loaded
	void setImage(const std::string& imageName) {
		// Do nothing, if the extension removal failed or if the current image is the same
		if (imageName == _imageName) {
			return;
		}

		releaseTexture();

		_imageName = imageName;

		// Set the visibility flag to zero, if no imageName is specified
		if (_imageName == "") {
			_visible = false;
		}

		captureTexture();
	}

	void setTransparency(const float& transparency) {
		_transparency = transparency;

		// Check for valid bounds (0.0f ... 1.0f)
		if (_transparency > 1.0f) {
			_transparency = 1.0f;
		} else if (_transparency < 0.0f) {
			_transparency = 0.0f;
		}
	}

	// Helper method, constrains the <input> float to the given min/max values
	float constrainFloat(const float& input, const float& min, const float& max) {
		if (input < min) {
			return min;
		} else if (input > max) {
			return max;
		}
		return input;
	}

	// Sets the image scale to the given float (1.0f is no scaling)
	void setImageScale(const float& scale) {
		_scale = constrainFloat(scale, MIN_SCALE, MAX_SCALE);
	}

	// Sets the image position in quasi texture coordinates (-0.5f .. 0.5f)
	void setImagePosition(const float& x, const float& y) {
		_translationX = constrainFloat(x, -1.0f, 1.0f);
		_translationY = constrainFloat(y, -1.0f, 1.0f);
	}

	// RegistryKeyObserver implementation, gets called upon key change
	void keyChanged(const std::string& changedKey, const std::string& newValue) {
		show(GlobalRegistry().getBool(RKEY_OVERLAY_VISIBLE));
		_keepProportions = GlobalRegistry().getBool(RKEY_OVERLAY_PROPORTIONAL);
		_scaleWithXYView = GlobalRegistry().getBool(RKEY_OVERLAY_SCALE_WITH_XY);
		_panWithXYView = GlobalRegistry().getBool(RKEY_OVERLAY_PAN_WITH_XY);
		setImage(GlobalRegistry().get(RKEY_OVERLAY_IMAGE));
		setTransparency(GlobalRegistry().getFloat(RKEY_OVERLAY_TRANSPARENCY));
		setImageScale(GlobalRegistry().getFloat(RKEY_OVERLAY_SCALE));
		setImagePosition(GlobalRegistry().getFloat(RKEY_OVERLAY_TRANSLATIONX),
						GlobalRegistry().getFloat(RKEY_OVERLAY_TRANSLATIONY));
	}

	// PreferenceConstructor implementation, add the preference settings
	void constructPreferencePage(PreferenceGroup& group) {
		PreferencesPage* page(group.createPage(_("Overlay"), _("Orthoview Background Overlay")));

		page->appendCheckBox("", _("Show Background Image"), RKEY_OVERLAY_VISIBLE);
		page->appendPathEntry(_("Background Image"), RKEY_OVERLAY_IMAGE, false);
		page->appendSlider(_("Image Transparency"), RKEY_OVERLAY_TRANSPARENCY, true, 0.3f, 0, 1, 0.01f, 0.20f, 0.0f);
		page->appendSlider(_("Image Position X"), RKEY_OVERLAY_TRANSLATIONX, true, 0.0f, -1.0f, 1.0f, 0.01f, 0.20f, 0.0f);
		page->appendSlider(_("Image Position Y"), RKEY_OVERLAY_TRANSLATIONY, true, 0.0f, -1.0f, 1.0f, 0.01f, 0.20f, 0.0f);
		page->appendSlider(_("Image Scale"), RKEY_OVERLAY_SCALE, true, 1.0f, MIN_SCALE, MAX_SCALE, 0.05f, 0.20f, 0.0f);
		page->appendCheckBox("", _("Keep Proportions"), RKEY_OVERLAY_PROPORTIONAL);
		page->appendCheckBox("", _("Scale Image together with Orthoview"), RKEY_OVERLAY_SCALE_WITH_XY);
		page->appendCheckBox("", _("Pan image with viewport"), RKEY_OVERLAY_PAN_WITH_XY);
	}

	void draw(float xbegin, float xend, float ybegin, float yend, float xyviewscale) {
		// Check if we should display the overlay in the first place
		if (!_visible) {
			return;
		}

		// Check if the texture is realised
		if (_texture == NULL) {
			// Try to realise it
			captureTexture();

			// If it's still not realised >> error
			if (_texture == NULL) {
				return;
			}
		}

		// The two corners of the window (default: stretches to window borders)
		Vector3 windowUpperLeft(xbegin, ybegin, 0);
		Vector3 windowLowerRight(xend, yend, 0);

		if (_keepProportions) {
			const float aspectRatio = static_cast<float>(_texture->width) / _texture->height;

			// Calculate the proportionally stretched yEnd coordinate
			const float newYend = ybegin + (xend - xbegin) / aspectRatio;
			windowLowerRight = Vector3(xend, newYend, 0);

			// Now calculate how far the center went off due to this stretch
			const float deltaCenter = (newYend - yend) / 2;

			// Correct the y coordinates with the delta, so that the image gets centered again
			windowLowerRight.y() -= deltaCenter;
			windowUpperLeft.y() -= deltaCenter;
		}

		// Calculate the (virtual) window center
		Vector3 windowOrigin((xend + xbegin) / 2, (yend + ybegin) / 2, 0);

		windowUpperLeft -= windowOrigin;
		windowLowerRight -= windowOrigin;

		// The translation vector
		Vector3 translation(_translationX * (xend - xbegin) * xyviewscale, _translationY * (yend - ybegin) * xyviewscale, 0);

		// Create a translation matrix
		Matrix4 scaleTranslation = Matrix4::getTranslation(translation);

		// Store the scale into the matrix
		scaleTranslation.xx() = _scale;
		scaleTranslation.yy() = _scale;

		if (_scaleWithXYView) {
			// Scale once again with the xyviewscale, if enabled
			scaleTranslation.xx() *= xyviewscale;
			scaleTranslation.yy() *= xyviewscale;
		}

		// Apply the transformations onto the window corners
		windowUpperLeft = scaleTranslation.transform(windowUpperLeft).getProjected();
		windowLowerRight = scaleTranslation.transform(windowLowerRight).getProjected();

		if (!_panWithXYView) {
			windowUpperLeft += windowOrigin;
			windowLowerRight += windowOrigin;
		}

		// Enable the blend functions and textures
		glEnable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		// Define the blend function for transparency
		glBlendColor(0, 0, 0, _transparency);
		glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE_MINUS_CONSTANT_ALPHA_EXT);

		// Define the texture (get the ID from the texture object)
		glBindTexture(GL_TEXTURE_2D, _texture->texture_number);
		glColor3f(1, 1, 1);

		// Draw the rectangle with the texture on it
		glBegin(GL_QUADS);
		glTexCoord2i(0, 1);
		glVertex3f(windowUpperLeft.x(), windowUpperLeft.y(), 0.0f);

		glTexCoord2i(1, 1);
		glVertex3f(windowLowerRight.x(), windowUpperLeft.y(), 0.0f);

		glTexCoord2i(1, 0);
		glVertex3f(windowLowerRight.x(), windowLowerRight.y(), 0.0f);

		glTexCoord2i(0, 0);
		glVertex3f(windowUpperLeft.x(), windowLowerRight.y(), 0.0f);
		glEnd();

		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
	}

private:

	void captureTexture() {
		if (_texture == NULL && _imageName != "") {
			_texture = GlobalTexturesCache().capture(LoadImageCallback(this, &loadImageGDK), _imageName);

			if (_texture->texture_number == 0) {
				// Image load seemed to have failed
				GlobalTexturesCache().release(_texture);
				_texture = NULL;
			}
		}
		else {
			globalErrorStream() << "Texture already captured!\n";
		}
	}

	void releaseTexture() {
		if (_texture != NULL) {
			GlobalTexturesCache().release(_texture);
			_texture = NULL;
		}
	}

	/* greebo: The custom loader that delivers the loaded Image* to the
	 * GlobalTexturesCache() system.
	 */
	static Image* loadImageGDK (void* environment, const std::string& name)
	{
		Overlay* self = reinterpret_cast<Overlay*> (environment);

		DirectoryArchiveFile file(name, name);

		if (!file.failed()) {
			// Invoke the imagefile loader
			return self->_imageGDKModule.getTable()->loadImage(file);
		}

		return NULL;
	}

}; // class Overlay

/* Overlay dependencies class.
 */
class OverlayDependencies :
	public GlobalRegistryModuleRef,
	public GlobalTexturesModuleRef,
	public GlobalPreferenceSystemModuleRef
{};

/* Required code to register the module with the ModuleServer.
 */
#include "modulesystem/singletonmodule.h"

typedef SingletonModule<Overlay, OverlayDependencies> OverlayModule;

typedef Static<OverlayModule> StaticOverlaySystemModule;
StaticRegisterModule staticRegisterOverlaySystem(StaticOverlaySystemModule::instance());
