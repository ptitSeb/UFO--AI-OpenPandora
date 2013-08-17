#ifndef TEXDEF_H_
#define TEXDEF_H_

#include "itexdef.h"
#include "math/matrix.h"

class TexDef: public GenericTextureDefinition
{
	public:
		// Constructor
		TexDef ();

		// copy constructor
		TexDef (const TexDef& other);

		// Constructs a TexDef out of the given transformation matrix plus width/height
		TexDef (float width, float height, const Matrix4& transform);

		virtual ~TexDef() {}

		void shift (float s, float t);

		void scale (float s, float t);

		void rotate (float angle);

		bool isSane () const;

		// All texture-projection translation (shift) values are congruent modulo the dimensions of the texture.
		// This function normalises shift values to the smallest positive congruent values.
		void normalise (float width, float height);

		/* Construct a transform in ST space from the texdef.
		 * Transforms constructed from quake's texdef format
		 * are (-shift)*(1/scale)*(-rotate) with x translation sign flipped.
		 * This would really make more sense if it was inverseof(shift*rotate*scale).. oh well.*/
		Matrix4 getTransform (float width, float height) const;
};

#endif /*TEXDEF_H_*/
