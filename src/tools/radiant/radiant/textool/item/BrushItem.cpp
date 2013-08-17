#include "BrushItem.h"

#include "FaceItem.h"

namespace textool {

class FaceItemCreator :
	public BrushVisitor
{
	// The target vector
	TexToolItemVec& _vector;
public:
	FaceItemCreator(TexToolItemVec& vector) :
		_vector(vector)
	{}

	void visit(Face& face) const {
		TexToolItem* faceItem(
			new FaceItem(face)
		);

		_vector.push_back(faceItem);
	}
};

// Constructor
BrushItem::BrushItem(Brush& sourceBrush) :
	_sourceBrush(sourceBrush)
{
	// Visit all the brush faces with the FaceItemCreator
	// that populates the _children vector
	_sourceBrush.forEachFace(FaceItemCreator(_children));
}

BrushItem::~BrushItem() {
	for (TexToolItemVec::iterator i = _children.begin(); i != _children.end(); ++i) {
		delete *i;
	}

	_children.clear();
}

void BrushItem::beginTransformation() {
	_sourceBrush.undoSave();
}

} // namespace TexTool
