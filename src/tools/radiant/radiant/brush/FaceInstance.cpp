#include "FaceInstance.h"

#include "renderable.h"
#include "math/frustum.h"

template<typename Element>
inline Vector3 triangle_cross (const BasicVector3<Element>& x, const BasicVector3<Element> y, const BasicVector3<
		Element>& z)
{
	return vector3_cross(y - x, z - x);
}
template<typename Element>
inline bool triangles_same_winding (const BasicVector3<Element>& x1, const BasicVector3<Element> y1,
		const BasicVector3<Element>& z1, const BasicVector3<Element>& x2, const BasicVector3<Element> y2,
		const BasicVector3<Element>& z2)
{
	return triangle_cross(x1, y1, z1).dot(triangle_cross(x2, y2, z2)) > 0;
}

inline bool triangle_reversed (std::size_t x, std::size_t y, std::size_t z)
{
	return !((x < y && y < z) || (z < x && x < y) || (y < z && z < x));
}

FaceInstance::FaceInstance (Face& face, const SelectionChangeCallback& observer) :
	m_face(&face), m_selectable(SelectedChangedCaller(*this)), m_selectableVertices(observer), m_selectableEdges(
			observer), m_selectionChanged(observer)
{
}
FaceInstance::FaceInstance (const FaceInstance& other) :
	m_face(other.m_face), m_selectable(SelectedChangedCaller(*this)), m_selectableVertices(other.m_selectableVertices),
			m_selectableEdges(other.m_selectableEdges), m_selectionChanged(other.m_selectionChanged)
{
}
FaceInstance& FaceInstance::operator= (const FaceInstance& other)
{
	m_face = other.m_face;
	return *this;
}

Face& FaceInstance::getFace ()
{
	return *m_face;
}
const Face& FaceInstance::getFace () const
{
	return *m_face;
}
Face* FaceInstance::getFacePtr () const
{
	return m_face;
}

void FaceInstance::selectedChanged (const Selectable& selectable)
{
	if (selectable.isSelected()) {
		g_SelectedFaceInstances.insert(*this);
	} else {
		g_SelectedFaceInstances.erase(*this);
	}
	m_selectionChanged(selectable);
}
typedef MemberCaller1<FaceInstance, const Selectable&, &FaceInstance::selectedChanged> SelectedChangedCaller;

bool FaceInstance::selectedVertices () const
{
	return !m_vertexSelection.empty();
}
bool FaceInstance::selectedEdges () const
{
	return !m_edgeSelection.empty();
}
bool FaceInstance::isSelected () const
{
	return m_selectable.isSelected();
}

bool FaceInstance::selectedComponents () const
{
	return selectedVertices() || selectedEdges() || isSelected();
}
bool FaceInstance::selectedComponents (SelectionSystem::EComponentMode mode) const
{
	switch (mode) {
	case SelectionSystem::eVertex:
		return selectedVertices();
	case SelectionSystem::eEdge:
		return selectedEdges();
	case SelectionSystem::eFace:
		return isSelected();
	default:
		return false;
	}
}

void FaceInstance::setSelected (SelectionSystem::EComponentMode mode, bool select)
{
	switch (mode) {
	case SelectionSystem::eFace:
		m_selectable.setSelected(select);
		break;
	case SelectionSystem::eVertex:
		ASSERT_MESSAGE(!select, "select-all not supported");

		m_vertexSelection.clear();
		m_selectableVertices.setSelected(false);
		break;
	case SelectionSystem::eEdge:
		ASSERT_MESSAGE(!select, "select-all not supported");

		m_edgeSelection.clear();
		m_selectableEdges.setSelected(false);
		break;
	default:
		break;
	}
}

void FaceInstance::invertSelected ()
{
	switch (GlobalSelectionSystem().ComponentMode()) {
	case SelectionSystem::eFace:
		m_selectable.invertSelected();
		break;
	case SelectionSystem::eVertex:
		break;
	case SelectionSystem::eEdge:
		break;
	default:
		break;
	}
}

template<typename Functor>
void FaceInstance::SelectedVertices_foreach (Functor functor) const
{
	for (VertexSelection::const_iterator i = m_vertexSelection.begin(); i != m_vertexSelection.end(); ++i) {
		const Winding& winding = m_face->getWinding();
		const std::size_t index = Winding_FindAdjacent(winding, *i);
		if (index != c_brush_maxFaces) {
			functor(winding[index].vertex);
		}
	}
}
template<typename Functor>
void FaceInstance::SelectedEdges_foreach (Functor functor) const
{
	for (VertexSelection::const_iterator i = m_edgeSelection.begin(); i != m_edgeSelection.end(); ++i) {
		const Winding& winding = m_face->getWinding();
		const std::size_t index = Winding_FindAdjacent(winding, *i);
		if (index != c_brush_maxFaces) {
			std::size_t adjacent = winding.next(index);
			functor(vector3_mid(winding[index].vertex, winding[adjacent].vertex));
		}
	}
}
template<typename Functor>
void FaceInstance::SelectedFaces_foreach (Functor functor) const
{
	if (isSelected()) {
		functor(centroid());
	}
}

template<typename Functor>
void FaceInstance::SelectedComponents_foreach (Functor functor) const
{
	SelectedVertices_foreach(functor);
	SelectedEdges_foreach(functor);
	SelectedFaces_foreach(functor);
}

void FaceInstance::iterate_selected (AABB& aabb) const
{
	SelectedComponents_foreach(AABBExtendByPoint(aabb));
}

class RenderablePointVectorPushBack
{
		RenderablePointVector& m_points;
	public:
		RenderablePointVectorPushBack (RenderablePointVector& points) :
			m_points(points)
		{
		}
		void operator() (const Vector3& point) const
		{
			const Colour4b colour_selected(0, 0, 255, 255);
			m_points.push_back(PointVertex(point, colour_selected));
		}
};

void FaceInstance::iterate_selected (RenderablePointVector& points) const
{
	SelectedComponents_foreach(RenderablePointVectorPushBack(points));
}

bool FaceInstance::intersectVolume (const VolumeTest& volume, const Matrix4& localToWorld) const
{
	return m_face->intersectVolume(volume, localToWorld);
}

void FaceInstance::render (Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const
{
	if (m_face->contributes() && intersectVolume(volume, localToWorld)) {
		renderer.PushState();
		if (selectedComponents()) {
			renderer.Highlight(Renderer::eFace);
		}
		m_face->render(renderer, localToWorld);
		renderer.PopState();
	}
}

void FaceInstance::testSelect (SelectionTest& test, SelectionIntersection& best)
{
	m_face->testSelect(test, best);
}
void FaceInstance::testSelect (Selector& selector, SelectionTest& test)
{
	SelectionIntersection best;
	testSelect(test, best);
	if (best.valid()) {
		Selector_add(selector, m_selectable, best);
	}
}
void FaceInstance::testSelect_centroid (Selector& selector, SelectionTest& test)
{
	if (m_face->contributes()) {
		SelectionIntersection best;
		m_face->testSelect_centroid(test, best);
		if (best.valid()) {
			Selector_add(selector, m_selectable, best);
		}
	}
}

void FaceInstance::selectPlane (Selector& selector, const Line& line, PlanesIterator first, PlanesIterator last,
		const PlaneCallback& selectedPlaneCallback)
{
	const Winding& winding = m_face->getWinding();
	for (Winding::const_iterator i =winding.begin(); i != winding.end(); ++i) {
		Vector3 v(line_closest_point(line, (*i).vertex) - (*i).vertex);
		double dot = m_face->plane3().normal().dot(v);
		if (dot <= 0) {
			return;
		}
	}

	Selector_add(selector, m_selectable);

	selectedPlaneCallback(m_face->plane3());
}
void FaceInstance::selectReversedPlane (Selector& selector, const SelectedPlanes& selectedPlanes)
{
	if (selectedPlanes.contains(-m_face->plane3())) {
		Selector_add(selector, m_selectable);
	}
}

void FaceInstance::transformComponents (const Matrix4& matrix)
{
	if (isSelected()) {
		m_face->transform(matrix, false);
	}
	if (selectedVertices()) {
		if (m_vertexSelection.size() == 1) {
			matrix4_transform_point(matrix, m_face->m_move_planeptsTransformed[1]);
			m_face->assign_planepts(m_face->m_move_planeptsTransformed);
		} else if (m_vertexSelection.size() == 2) {
			matrix4_transform_point(matrix, m_face->m_move_planeptsTransformed[1]);
			matrix4_transform_point(matrix, m_face->m_move_planeptsTransformed[2]);
			m_face->assign_planepts(m_face->m_move_planeptsTransformed);
		} else if (m_vertexSelection.size() >= 3) {
			matrix4_transform_point(matrix, m_face->m_move_planeptsTransformed[0]);
			matrix4_transform_point(matrix, m_face->m_move_planeptsTransformed[1]);
			matrix4_transform_point(matrix, m_face->m_move_planeptsTransformed[2]);
			m_face->assign_planepts(m_face->m_move_planeptsTransformed);
		}
	}
	if (selectedEdges()) {
		if (m_edgeSelection.size() == 1) {
			matrix4_transform_point(matrix, m_face->m_move_planeptsTransformed[0]);
			matrix4_transform_point(matrix, m_face->m_move_planeptsTransformed[1]);
			m_face->assign_planepts(m_face->m_move_planeptsTransformed);
		} else if (m_edgeSelection.size() >= 2) {
			matrix4_transform_point(matrix, m_face->m_move_planeptsTransformed[0]);
			matrix4_transform_point(matrix, m_face->m_move_planeptsTransformed[1]);
			matrix4_transform_point(matrix, m_face->m_move_planeptsTransformed[2]);
			m_face->assign_planepts(m_face->m_move_planeptsTransformed);
		}
	}
}

void FaceInstance::snapto (float snap)
{
	m_face->snapto(snap);
}

void FaceInstance::snapComponents (float snap)
{
	if (isSelected()) {
		snapto(snap);
	}
	if (selectedVertices()) {
		vector3_snap(m_face->m_move_planepts[0], snap);
		vector3_snap(m_face->m_move_planepts[1], snap);
		vector3_snap(m_face->m_move_planepts[2], snap);
		m_face->assign_planepts(m_face->m_move_planepts);
		planepts_assign(m_face->m_move_planeptsTransformed, m_face->m_move_planepts);
		m_face->freezeTransform();
	}
	if (selectedEdges()) {
		vector3_snap(m_face->m_move_planepts[0], snap);
		vector3_snap(m_face->m_move_planepts[1], snap);
		vector3_snap(m_face->m_move_planepts[2], snap);
		m_face->assign_planepts(m_face->m_move_planepts);
		planepts_assign(m_face->m_move_planeptsTransformed, m_face->m_move_planepts);
		m_face->freezeTransform();
	}
}
void FaceInstance::update_move_planepts_vertex (std::size_t index)
{
	m_face->update_move_planepts_vertex(index, m_face->m_move_planepts);
}
void FaceInstance::update_move_planepts_vertex2 (std::size_t index, std::size_t other)
{
	const std::size_t numpoints = m_face->getWinding().size();
	ASSERT_MESSAGE(index < numpoints, "select_vertex: invalid index");

	const std::size_t opposite = Winding_Opposite(m_face->getWinding(), index, other);

	if (triangle_reversed(index, other, opposite)) {
		std::swap(index, other);
	}

	ASSERT_MESSAGE(
			triangles_same_winding(
					m_face->getWinding()[opposite].vertex,
					m_face->getWinding()[index].vertex,
					m_face->getWinding()[other].vertex,
					m_face->getWinding()[0].vertex,
					m_face->getWinding()[1].vertex,
					m_face->getWinding()[2].vertex
			),
			"update_move_planepts_vertex2: error"
	);

	m_face->m_move_planepts[0] = m_face->getWinding()[opposite].vertex;
	m_face->m_move_planepts[1] = m_face->getWinding()[index].vertex;
	m_face->m_move_planepts[2] = m_face->getWinding()[other].vertex;
	planepts_quantise(m_face->m_move_planepts, GRID_MIN); // winding points are very inaccurate
}
void FaceInstance::update_selection_vertex ()
{
	if (m_vertexSelection.empty()) {
		m_selectableVertices.setSelected(false);
	} else {
		m_selectableVertices.setSelected(true);

		const Winding& winding = m_face->getWinding();
		if (m_vertexSelection.size() == 1) {
			std::size_t index = Winding_FindAdjacent(winding, *m_vertexSelection.begin());

			if (index != c_brush_maxFaces) {
				update_move_planepts_vertex(index);
			}
		} else if (m_vertexSelection.size() == 2) {
			std::size_t index = Winding_FindAdjacent(winding, *m_vertexSelection.begin());
			std::size_t other = Winding_FindAdjacent(winding, *(++m_vertexSelection.begin()));

			if (index != c_brush_maxFaces && other != c_brush_maxFaces) {
				update_move_planepts_vertex2(index, other);
			}
		}
	}
}
void FaceInstance::select_vertex (std::size_t index, bool select)
{
	const Winding& winding = m_face->getWinding();
	if (select) {
		VertexSelection_insert(m_vertexSelection, winding[index].adjacent);
	} else {
		VertexSelection_erase(m_vertexSelection, winding[index].adjacent);
	}

	SceneChangeNotify();
	update_selection_vertex();
}

bool FaceInstance::selected_vertex (std::size_t index) const
{
	const Winding& winding = m_face->getWinding();
	return VertexSelection_find(m_vertexSelection, winding[index].adjacent) != m_vertexSelection.end();
}

void FaceInstance::update_move_planepts_edge (std::size_t index)
{
	const Winding& winding = m_face->getWinding();
	const std::size_t numpoints = winding.size();
	ASSERT_MESSAGE(index < numpoints, "select_edge: invalid index");

	const std::size_t adjacent = winding.next(index);
	const std::size_t opposite = Winding_Opposite(winding, index);
	m_face->m_move_planepts[0] = winding[index].vertex;
	m_face->m_move_planepts[1] = winding[adjacent].vertex;
	m_face->m_move_planepts[2] = winding[opposite].vertex;
	planepts_quantise(m_face->m_move_planepts, GRID_MIN); // winding points are very inaccurate
}
void FaceInstance::update_selection_edge ()
{
	if (m_edgeSelection.empty()) {
		m_selectableEdges.setSelected(false);
	} else {
		m_selectableEdges.setSelected(true);

		if (m_edgeSelection.size() == 1) {
			std::size_t index = Winding_FindAdjacent(m_face->getWinding(), *m_edgeSelection.begin());

			if (index != c_brush_maxFaces) {
				update_move_planepts_edge(index);
			}
		}
	}
}
void FaceInstance::select_edge (std::size_t index, bool select)
{
	if (select) {
		VertexSelection_insert(m_edgeSelection, m_face->getWinding()[index].adjacent);
	} else {
		VertexSelection_erase(m_edgeSelection, m_face->getWinding()[index].adjacent);
	}

	SceneChangeNotify();
	update_selection_edge();
}

bool FaceInstance::selected_edge (std::size_t index) const
{
	return VertexSelection_find(m_edgeSelection, m_face->getWinding()[index].adjacent) != m_edgeSelection.end();
}

const Vector3& FaceInstance::centroid () const
{
	return m_face->centroid();
}

void FaceInstance::connectivityChanged ()
{
	// This occurs when a face is added or removed.
	// The current vertex and edge selections no longer valid and must be cleared.
	m_vertexSelection.clear();
	m_selectableVertices.setSelected(false);
	m_edgeSelection.clear();
	m_selectableEdges.setSelected(false);
}

FaceInstanceSet g_SelectedFaceInstances;
