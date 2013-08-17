#ifndef PRIMITIVES_H_
#define PRIMITIVES_H_

#include <string>
#include <stdexcept>
#include <vector>

class Face;
class Brush;

typedef std::vector<Brush*> BrushPtrVector;
typedef std::vector<Face*> FacePtrVector;

namespace selection {

	/** greebo: This is thrown if some of the routines
	 * below fail to retrieve the requested selection.
	 */
	class InvalidSelectionException :
		public std::runtime_error
	{
	public:
		// Constructor
		InvalidSelectionException(const std::string& what):
			std::runtime_error(what)
		{}
	};

	namespace algorithm {

	/** greebo: Returns the number of the selected face instances.
	 */
	int selectedFaceCount();

	/** greebo: Retrieves a list of selected Brushes, populated
	 * by a scene walker.
	 *
	 * @returns: the vector with the selected brush pointers.
	 */
	BrushPtrVector getSelectedBrushes();

	/** greebo: Retrieves a list of selected Faces (those selected
	 * 			with Ctrl-Shift-LMB by default).
	 *
	 * @returns: the vector with the selected face pointers.
	 */
	FacePtrVector getSelectedFaces();

	/** greebo: Retrieves the reference to the last selected face.
	 *
	 * Throws an selection::InvalidSelectionException on failure.
	 *
	 * @returns: the Face& reference of the last element.
	 */
	Face& getLastSelectedFace();

	void makeDetail ();

	void makeStructural ();

	} // namespace algorithm
} // namespace selection

#endif /*PRIMITIVES_H_*/
