#ifndef RENDERABLEPICOSURFACE_H_
#define RENDERABLEPICOSURFACE_H_

#include "picomodel.h"
#include "irender.h"
#include "render.h"
#include "cullable.h"
#include "selectable.h"
#include <string>
#include "math/aabb.h"
#include "VolumeIntersectionValue.h"

/* FORWARD DECLS */

namespace model
{
	/* Renderable class containing a series of polygons textured with the same
	 * material. RenderablePicoSurface objects are composited into a RenderablePicoModel
	 * object to create a renderable static mesh.
	 */
	class RenderablePicoSurface: public OpenGLRenderable, public Cullable
	{
			// Name of the material this surface is using, both originally and after a skin
			// remap.
			std::string _originalShaderName;
			std::string _mappedShaderName;

			// Shader object containing the material shader for this surface
			Shader* _shader;

			// Vector of ArbitraryMeshVertex structures, containing the coordinates,
			// normals, tangents and texture coordinates of the component vertices
			std::vector<ArbitraryMeshVertex> _vertices;

			// Vector of render indices, representing the groups of vertices to be
			// used to create triangles
			std::vector<unsigned int> _indices;

			// Keep track of the number of indices to iterate over, since vector::size()
			// may not be fast
			unsigned int _nIndices;

			// The AABB containing this surface, in local object space.
			AABB _localAABB;

		public:
			/** Constructor. Accepts a picoSurface_t struct and the file extension to determine
			 * how to assign materials.
			 */
			RenderablePicoSurface (picoSurface_t* surf);

			/** copy constructor
			 */
			RenderablePicoSurface (RenderablePicoSurface const& other);

			/** Destructor. Vectors will be automatically destructed, but we need to release the
			 * shader.
			 */
			~RenderablePicoSurface ();

			/** Render function from OpenGLRenderable
			 */
			void render (RenderStateFlags flags) const;

			void render (Renderer& renderer, const Matrix4& localToWorld, Shader* state) const;
			void render (Renderer& renderer, const Matrix4& localToWorld) const;

			/** Return the vertex count for this surface
			 */
			int getVertexCount () const
			{
				return _vertices.size();
			}

			/** Return the poly count for this surface
			 */
			int getPolyCount () const
			{
				return _indices.size() / 3; // 3 indices per triangle
			}

			/** Get the Shader for this surface.
			 */
			Shader* getShader () const
			{
				return _shader;
			}

			/** Get the containing AABB for this surface.
			 */
			const AABB& localAABB () const
			{
				return _localAABB;
			}

			VolumeIntersectionValue intersectVolume (const VolumeTest& test, const Matrix4& localToWorld) const
			{
				return test.TestAABB(_localAABB, localToWorld);
			}

			void testSelect (Selector& selector, SelectionTest& test, const Matrix4& localToWorld)
			{
				test.BeginMesh(localToWorld);
				SelectionIntersection result;

				test.TestTriangles(VertexPointer(&_vertices[0].vertex, sizeof(ArbitraryMeshVertex)), IndexPointer(
					&_indices[0], IndexPointer::index_type(_indices.size())), result);

				// Add the intersection to the selector if it is valid
				if(result.valid()) {
					selector.addIntersection(result);
				}
			}

			/** Apply the provided skin to this surface. If the skin has a remap for
			 * this surface's material, it will be applied, otherwise no action will
			 * occur.
			 *
			 * @param skin
			 * path of the skin to apply
			 */
			void applySkin (const std::string& skin);
	};
}

#endif /*RENDERABLEPICOSURFACE_H_*/
