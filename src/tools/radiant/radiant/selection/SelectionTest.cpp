#include "SelectionTest.h"

#include "../renderer.h"

inline SelectionIntersection select_point_from_clipped(Vector4& clipped) {
  return SelectionIntersection(clipped[2] / clipped[3], static_cast<float>(Vector3(clipped[0] / clipped[3], clipped[1] / clipped[3], 0).getLengthSquared()));
}

void SelectionVolume::BeginMesh(const Matrix4& localToWorld, bool twoSided) {
    _local2view = matrix4_multiplied_by_matrix4(_view.GetViewMatrix(), localToWorld);

    // Cull back-facing polygons based on winding being clockwise or counter-clockwise.
    // Don't cull if the view is wireframe and the polygons are two-sided.
    _cull = twoSided && !_view.fill() ? eClipCullNone : (matrix4_handedness(localToWorld) == MATRIX4_RIGHTHANDED) ? eClipCullCW : eClipCullCCW;

    {
      Matrix4 screen2world(matrix4_full_inverse(_local2view));

      _near = matrix4_transformed_vector4( screen2world, Vector4(0, 0, -1, 1) ).getProjected();

      _far = matrix4_transformed_vector4( screen2world, Vector4(0, 0, 1, 1) ).getProjected();
    }

#if defined(DEBUG_SELECTION)
    g_render_clipped.construct(_view.GetViewMatrix());
#endif
  }

void SelectionVolume::TestPoint(const Vector3& point, SelectionIntersection& best) {
    Vector4 clipped;
    if(matrix4_clip_point(_local2view, point, clipped) == c_CLIP_PASS)
    {
      best = select_point_from_clipped(clipped);
    }
}

void SelectionVolume::TestPolygon(const VertexPointer& vertices, std::size_t count, SelectionIntersection& best) {
    Vector4 clipped[9];
    for(std::size_t i=0; i+2<count; ++i)
    {
      BestPoint(
        matrix4_clip_triangle(
          _local2view,
          reinterpret_cast<const Vector3&>(vertices[0]),
          reinterpret_cast<const Vector3&>(vertices[i+1]),
          reinterpret_cast<const Vector3&>(vertices[i+2]),
          clipped
        ),
        clipped,
        best,
        _cull
      );
    }
}

void SelectionVolume::TestLineLoop(const VertexPointer& vertices, std::size_t count, SelectionIntersection& best) {
    if(count == 0)
      return;
    Vector4 clipped[9];
    for(VertexPointer::iterator i = vertices.begin(), end = i + count, prev = i + (count-1); i != end; prev = i, ++i)
    {
      BestPoint(
        matrix4_clip_line(
          _local2view,
          reinterpret_cast<const Vector3&>((*prev)),
          reinterpret_cast<const Vector3&>((*i)),
          clipped
        ),
        clipped,
        best,
        _cull
      );
    }
}

void SelectionVolume::TestLineStrip(const VertexPointer& vertices, std::size_t count, SelectionIntersection& best) {
    if(count == 0)
      return;
    Vector4 clipped[9];
    for(VertexPointer::iterator i = vertices.begin(), end = i + count, next = i + 1; next != end; i = next, ++next)
    {
      BestPoint(
        matrix4_clip_line(
          _local2view,
          reinterpret_cast<const Vector3&>((*i)),
          reinterpret_cast<const Vector3&>((*next)),
          clipped
        ),
        clipped,
        best,
        _cull
      );
    }
}

void SelectionVolume::TestLines(const VertexPointer& vertices, std::size_t count, SelectionIntersection& best) {
    if(count == 0)
      return;
    Vector4 clipped[9];
    for(VertexPointer::iterator i = vertices.begin(), end = i + count; i != end; i += 2)
    {
      BestPoint(
        matrix4_clip_line(
          _local2view,
          reinterpret_cast<const Vector3&>((*i)),
          reinterpret_cast<const Vector3&>((*(i+1))),
          clipped
        ),
        clipped,
        best,
        _cull
      );
    }
}

void SelectionVolume::TestTriangles(const VertexPointer& vertices, const IndexPointer& indices, SelectionIntersection& best) {
    Vector4 clipped[9];
    for(IndexPointer::iterator i(indices.begin()); i != indices.end(); i += 3)
    {
      BestPoint(
        matrix4_clip_triangle(
          _local2view,
          reinterpret_cast<const Vector3&>(vertices[*i]),
          reinterpret_cast<const Vector3&>(vertices[*(i+1)]),
          reinterpret_cast<const Vector3&>(vertices[*(i+2)]),
          clipped
        ),
        clipped,
        best,
        _cull
      );
    }
}

void SelectionVolume::TestQuads(const VertexPointer& vertices, const IndexPointer& indices, SelectionIntersection& best) {
    Vector4 clipped[9];
    for(IndexPointer::iterator i(indices.begin()); i != indices.end(); i += 4)
    {
      BestPoint(
        matrix4_clip_triangle(
          _local2view,
          reinterpret_cast<const Vector3&>(vertices[*i]),
          reinterpret_cast<const Vector3&>(vertices[*(i+1)]),
          reinterpret_cast<const Vector3&>(vertices[*(i+3)]),
          clipped
        ),
        clipped,
        best,
        _cull
      );
	    BestPoint(
        matrix4_clip_triangle(
          _local2view,
          reinterpret_cast<const Vector3&>(vertices[*(i+1)]),
          reinterpret_cast<const Vector3&>(vertices[*(i+2)]),
          reinterpret_cast<const Vector3&>(vertices[*(i+3)]),
          clipped
        ),
        clipped,
        best,
        _cull
      );
    }
}

void SelectionVolume::TestQuadStrip(const VertexPointer& vertices, const IndexPointer& indices, SelectionIntersection& best) {
    Vector4 clipped[9];
    for(IndexPointer::iterator i(indices.begin()); i+2 != indices.end(); i += 2)
    {
      BestPoint(
        matrix4_clip_triangle(
          _local2view,
          reinterpret_cast<const Vector3&>(vertices[*i]),
          reinterpret_cast<const Vector3&>(vertices[*(i+1)]),
          reinterpret_cast<const Vector3&>(vertices[*(i+2)]),
          clipped
        ),
        clipped,
        best,
        _cull
      );
      BestPoint(
        matrix4_clip_triangle(
          _local2view,
          reinterpret_cast<const Vector3&>(vertices[*(i+2)]),
          reinterpret_cast<const Vector3&>(vertices[*(i+1)]),
          reinterpret_cast<const Vector3&>(vertices[*(i+3)]),
          clipped
        ),
        clipped,
        best,
        _cull
      );
    }
}

// ==================================================================================

bool testselect_entity_visible::pre(const scene::Path& path, scene::Instance& instance) const {
    Selectable* selectable = Instance_getSelectable(instance);
    if(selectable != 0 && Node_isEntity(path.top())) {
      _selector.pushSelectable(*selectable);
    }

    SelectionTestable* selectionTestable = Instance_getSelectionTestable(instance);
    if(selectionTestable) {
      selectionTestable->testSelect(_selector, _test);
    }

    return true;
}

void testselect_entity_visible::post(const scene::Path& path, scene::Instance& instance) const {
	Selectable* selectable = Instance_getSelectable(instance);
	if (selectable == 0)
		return;
	// Get the Entity from this node
	Entity* entity = Node_getEntity(path.top());
	// Don't select the worldspawn entity
	if (entity != NULL && entity->getKeyValue("classname") != "worldspawn") {
		_selector.popSelectable();
	}
}

bool testselect_primitive_visible::pre(const scene::Path& path, scene::Instance& instance) const {
    Selectable* selectable = Instance_getSelectable(instance);
    if(selectable != 0) {
      _selector.pushSelectable(*selectable);
    }

    SelectionTestable* selectionTestable = Instance_getSelectionTestable(instance);
    if(selectionTestable) {
      selectionTestable->testSelect(_selector, _test);
    }

    return true;
}

void testselect_primitive_visible::post(const scene::Path& path, scene::Instance& instance) const {
    Selectable* selectable = Instance_getSelectable(instance);
    if(selectable != 0)
    {
      _selector.popSelectable();
    }
}

bool testselect_component_visible::pre(const scene::Path& path, scene::Instance& instance) const {
    ComponentSelectionTestable* componentSelectionTestable = Instance_getComponentSelectionTestable(instance);
	if(componentSelectionTestable) {
      componentSelectionTestable->testSelectComponents(_selector, _test, _mode);
    }

    return true;
}

bool testselect_component_visible_selected::pre(const scene::Path& path, scene::Instance& instance) const {
    Selectable* selectable = Instance_getSelectable(instance);
    if (selectable != 0 && selectable->isSelected()) {
      ComponentSelectionTestable* componentSelectionTestable = Instance_getComponentSelectionTestable(instance);
      if (componentSelectionTestable) {
        componentSelectionTestable->testSelectComponents(_selector, _test, _mode);
      }
    }

    return true;
}

// ==================================================================================

void Scene_TestSelect_Primitive(Selector& selector, SelectionTest& test, const VolumeTest& volume) {
  Scene_forEachVisible(GlobalSceneGraph(), volume, testselect_primitive_visible(selector, test));
}

void Scene_TestSelect_Component_Selected(Selector& selector, SelectionTest& test, const VolumeTest& volume, SelectionSystem::EComponentMode componentMode) {
  Scene_forEachVisible(GlobalSceneGraph(), volume, testselect_component_visible_selected(selector, test, componentMode));
}

void Scene_TestSelect_Component(Selector& selector, SelectionTest& test, const VolumeTest& volume, SelectionSystem::EComponentMode componentMode) {
  Scene_forEachVisible(GlobalSceneGraph(), volume, testselect_component_visible(selector, test, componentMode));
}
