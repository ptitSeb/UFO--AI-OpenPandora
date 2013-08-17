#include "Brush.h"

#include "math/frustum.h"
#include "renderable.h"
#include "ifilter.h"

#include "Face.h"
#include "generic/referencecounted.h"
#include "../plugin.h"
#include "../sidebar/surfaceinspector/surfaceinspector.h"

/// \brief Returns true if 'self' takes priority when building brush b-rep.
inline bool plane3_inside (const Plane3& self, const Plane3& other)
{
	if (vector3_equal_epsilon(self.normal(), other.normal(), 0.001)) {
		return self.dist() < other.dist();
	}
	return true;
}

Brush::Brush (scene::Node& node, const Callback& evaluateTransform, const Callback& boundsChanged) :
	m_node(&node), m_undoable_observer(0), m_map(0), m_render_faces(m_faceCentroidPoints, GL_POINTS),
	m_render_vertices(m_uniqueVertexPoints, GL_POINTS), m_render_edges(m_uniqueEdgePoints, GL_POINTS),
	m_evaluateTransform(evaluateTransform), m_boundsChanged(boundsChanged), m_planeChanged(false), m_transformChanged(
			false)
{
	planeChanged();
}
Brush::Brush (const Brush& other, scene::Node& node, const Callback& evaluateTransform, const Callback& boundsChanged) :
	m_node(&node), m_undoable_observer(0), m_map(0), m_render_faces(m_faceCentroidPoints, GL_POINTS),
	m_render_vertices(m_uniqueVertexPoints, GL_POINTS), m_render_edges(m_uniqueEdgePoints, GL_POINTS),
	m_evaluateTransform(evaluateTransform), m_boundsChanged(boundsChanged), m_planeChanged(false), m_transformChanged(
			false)
{
	copy(other);
}
Brush::Brush (const Brush& other) :
	TransformNode(other), Bounded(other), Cullable(other), Snappable(other), Undoable(other), FaceObserver(other),
			Nameable(other), m_node(0), m_undoable_observer(0), m_map(0), m_render_faces(m_faceCentroidPoints,
					GL_POINTS),
			m_render_vertices(m_uniqueVertexPoints, GL_POINTS), m_render_edges(m_uniqueEdgePoints, GL_POINTS),
			m_planeChanged(false), m_transformChanged(false)
{
	copy(other);
}
Brush::~Brush ()
{
	ASSERT_MESSAGE(m_observers.empty(), "Brush::~Brush: observers still attached");
}

void Brush::attach (BrushObserver& observer)
{
	for (Faces::iterator i = m_faces.begin(); i != m_faces.end(); ++i) {
		observer.push_back(*(*i));
	}

	for (SelectableEdges::iterator i = m_select_edges.begin(); i != m_select_edges.end(); ++i) {
		observer.edge_push_back(*i);
	}

	for (SelectableVertices::iterator i = m_select_vertices.begin(); i != m_select_vertices.end(); ++i) {
		observer.vertex_push_back(*i);
	}

	m_observers.insert(&observer);
}
void Brush::detach (BrushObserver& observer)
{
	m_observers.erase(&observer);
}

void Brush::forEachFace (const BrushVisitor& visitor) const
{
	for (Faces::const_iterator i = m_faces.begin(); i != m_faces.end(); ++i) {
		visitor.visit(*(*i));
	}
}

void Brush::forEachFace_instanceAttach (MapFile* map) const
{
	for (Faces::const_iterator i = m_faces.begin(); i != m_faces.end(); ++i) {
		(*i)->instanceAttach(map);
	}
}
void Brush::forEachFace_instanceDetach (MapFile* map) const
{
	for (Faces::const_iterator i = m_faces.begin(); i != m_faces.end(); ++i) {
		(*i)->instanceDetach(map);
	}
}

InstanceCounter m_instanceCounter;
void Brush::instanceAttach (const scene::Path& path)
{
	if (++m_instanceCounter.m_count == 1) {
		m_map = path_find_mapfile(path.begin(), path.end());
		m_undoable_observer = GlobalUndoSystem().observer(this);
		forEachFace_instanceAttach( m_map);
	} else {
		ASSERT_MESSAGE(path_find_mapfile(path.begin(), path.end()) == m_map, "node is instanced across more than one file");
	}
}
void Brush::instanceDetach (const scene::Path& path)
{
	if (--m_instanceCounter.m_count == 0) {
		forEachFace_instanceDetach( m_map);
		m_map = 0;
		m_undoable_observer = 0;
		GlobalUndoSystem().release(this);
	}
}

// nameable
std::string Brush::name () const
{
	return "brush";
}
void Brush::attach (const NameCallback& callback)
{
}
void Brush::detach (const NameCallback& callback)
{
}

// observer
void Brush::planeChanged ()
{
	m_planeChanged = true;
	aabbChanged();
}
void Brush::shaderChanged ()
{
	planeChanged();

	// Queue an UI update of the texture tools
	ui::SurfaceInspector::Instance().queueUpdate();
}

void Brush::evaluateBRep () const
{
	if (m_planeChanged) {
		m_planeChanged = false;
		const_cast<Brush*> (this)->buildBRep();
	}
}

void Brush::transformChanged ()
{
	m_transformChanged = true;
	planeChanged();
}

void Brush::evaluateTransform ()
{
	if (m_transformChanged) {
		m_transformChanged = false;
		revertTransform();
		m_evaluateTransform();
	}
}
const Matrix4& Brush::localToParent () const
{
	return Matrix4::getIdentity();
}
void Brush::aabbChanged ()
{
	m_boundsChanged();
}
const AABB& Brush::localAABB () const
{
	evaluateBRep();
	return m_aabb_local;
}

VolumeIntersectionValue Brush::intersectVolume (const VolumeTest& test, const Matrix4& localToWorld) const
{
	return test.TestAABB(m_aabb_local, localToWorld);
}

void Brush::renderComponents (SelectionSystem::EComponentMode mode, Renderer& renderer, const VolumeTest& volume,
		const Matrix4& localToWorld) const
{
	switch (mode) {
	case SelectionSystem::eVertex:
		renderer.addRenderable(m_render_vertices, localToWorld);
		break;
	case SelectionSystem::eEdge:
		renderer.addRenderable(m_render_edges, localToWorld);
		break;
	case SelectionSystem::eFace:
		renderer.addRenderable(m_render_faces, localToWorld);
		break;
	default:
		break;
	}
}

void Brush::transform (const Matrix4& matrix)
{
	bool mirror = matrix4_handedness(matrix) == MATRIX4_LEFTHANDED;

	for (Faces::iterator i = m_faces.begin(); i != m_faces.end(); ++i) {
		(*i)->transform(matrix, mirror);
	}
}
void Brush::snapto (float snap)
{
	for (Faces::iterator i = m_faces.begin(); i != m_faces.end(); ++i) {
		(*i)->snapto(snap);
	}
}
void Brush::revertTransform ()
{
	for (Faces::iterator i = m_faces.begin(); i != m_faces.end(); ++i) {
		(*i)->revertTransform();
	}
}
void Brush::freezeTransform ()
{
	for (Faces::iterator i = m_faces.begin(); i != m_faces.end(); ++i) {
		(*i)->freezeTransform();
	}
}

/// \brief Returns the absolute index of the \p faceVertex.
std::size_t Brush::absoluteIndex (FaceVertexId faceVertex)
{
	std::size_t index = 0;
	for (std::size_t i = 0; i < faceVertex.getFace(); ++i) {
		index += m_faces[i]->getWinding().size();
	}
	return index + faceVertex.getVertex();
}

void Brush::appendFaces (const Faces& other)
{
	clear();
	for (Faces::const_iterator i = other.begin(); i != other.end(); ++i) {
		push_back(*i);
	}
}

void Brush::undoSave ()
{
	if (m_map != 0) {
		m_map->changed();
	}
	if (m_undoable_observer != 0) {
		m_undoable_observer->save(this);
	}
}

UndoMemento* Brush::exportState () const
{
	return new BrushUndoMemento(m_faces);
}

void Brush::importState (const UndoMemento* state)
{
	undoSave();
	appendFaces(static_cast<const BrushUndoMemento*> (state)->m_faces);
	planeChanged();

	for (Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
		(*i)->DEBUG_verify();
	}
}

bool Brush::isDetail ()
{
	return !m_faces.empty() && m_faces.front()->isDetail();
}

/// \brief Appends a copy of \p face to the end of the face list.
Face* Brush::addFace (const Face& face)
{
	if (m_faces.size() == c_brush_maxFaces) {
		return 0;
	}
	undoSave();
	push_back(FaceSmartPointer(new Face(face, this)));
	m_faces.back()->setDetail(isDetail());
	planeChanged();
	return m_faces.back();
}

/// \brief Appends a new face constructed from the parameters to the end of the face list.
Face* Brush::addPlane (const Vector3& p0, const Vector3& p1, const Vector3& p2, const std::string& shader,
		const TextureProjection& projection)
{
	if (m_faces.size() == c_brush_maxFaces) {
		return 0;
	}
	undoSave();
	push_back(FaceSmartPointer(new Face(p0, p1, p2, shader, projection, this)));
	m_faces.back()->setDetail(isDetail());
	planeChanged();
	return m_faces.back();
}

/* works like addPlane, but cleans the brush from useless planes */
Face* Brush::chopWithPlane (const Vector3& p0, const Vector3& p1, const Vector3& p2, const std::string& shader,
		const TextureProjection& projection)
{
	if (m_faces.size() == c_brush_maxFaces) {
		return 0;
	}

	evaluateBRep();

	/* remove faces that will be cut away */
	Faces::iterator faceIt = m_faces.begin();
	while (faceIt < m_faces.end()) {
		Face *victim = *faceIt;

		BrushSplitType split = Winding_ClassifyPlane (victim->getWinding(), Plane3(p0, p1, p2));
		if (split.counts[ePlaneFront] != 0) {
			faceIt = ++faceIt;
			continue; /* this face contrubutes to the brush (at least if brush wasn't malformed before the chop), so keep it */
		} else {
			faceIt = m_faces.erase(faceIt);
		}
	}

	push_back(FaceSmartPointer(new Face(p0, p1, p2, shader, projection, this)));
	m_faces.back()->setDetail(isDetail());
	planeChanged();
	return m_faces.back();
}

/**
 * The assumption is that surfaceflags are the same for all faces
 */
ContentsFlagsValue Brush::getFlags ()
{
	if (m_faces.empty())
		return ContentsFlagsValue();
	return m_faces.back()->GetFlags();
}

void Brush::constructStatic ()
{
	Face::m_quantise = quantiseFloating;

	m_state_point = GlobalShaderCache().capture("$POINT");
}
void Brush::destroyStatic ()
{
	GlobalShaderCache().release("$POINT");
}

std::size_t Brush::DEBUG_size ()
{
	return m_faces.size();
}

Brush::const_iterator Brush::begin () const
{
	return m_faces.begin();
}
Brush::const_iterator Brush::end () const
{
	return m_faces.end();
}

Face* Brush::back ()
{
	return m_faces.back();
}
const Face* Brush::back () const
{
	return m_faces.back();
}
/**
 * Reserve space in the planes vector
 * @param count The amount of planes to reserve
 */
void Brush::reserve (std::size_t count)
{
	m_faces.reserve(count);
	for (Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
		(*i)->reserve(count);
	}
}
void Brush::push_back (Faces::value_type face)
{
	m_faces.push_back(face);
	if (m_instanceCounter.m_count != 0) {
		m_faces.back()->instanceAttach(m_map);
	}
	for (Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
		(*i)->push_back(*face);
		(*i)->DEBUG_verify();
	}
}
void Brush::pop_back ()
{
	if (m_instanceCounter.m_count != 0) {
		m_faces.back()->instanceDetach(m_map);
	}
	m_faces.pop_back();
	for (Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
		(*i)->pop_back();
		(*i)->DEBUG_verify();
	}
}
void Brush::erase (std::size_t index)
{
	if (m_instanceCounter.m_count != 0) {
		m_faces[index]->instanceDetach(m_map);
	}
	m_faces.erase(m_faces.begin() + index);
	for (Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
		(*i)->erase(index);
		(*i)->DEBUG_verify();
	}
}
void Brush::connectivityChanged ()
{
	for (Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
		(*i)->connectivityChanged();
	}
}

/**
 * Clears the planes vector
 */
void Brush::clear ()
{
	undoSave();
	if (m_instanceCounter.m_count != 0) {
		forEachFace_instanceDetach( m_map);
	}
	m_faces.clear();
	for (Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
		(*i)->clear();
		(*i)->DEBUG_verify();
	}
}
std::size_t Brush::size () const
{
	return m_faces.size();
}
bool Brush::empty () const
{
	return m_faces.empty();
}

/// \brief Returns true if any face of the brush contributes to the final B-Rep.
bool Brush::hasContributingFaces () const
{
	for (const_iterator i = begin(); i != end(); ++i) {
		if ((*i)->contributes()) {
			return true;
		}
	}
	return false;
}

/// \brief Removes faces that do not contribute to the brush. This is useful for cleaning up after CSG operations on the brush.
/// Note: removal of empty faces is not performed during direct brush manipulations, because it would make a manipulation irreversible if it created an empty face.
void Brush::removeEmptyFaces ()
{
	evaluateBRep();

	std::size_t i = 0;
	while (i < m_faces.size()) {
		if (!m_faces[i]->contributes()) {
			erase(i);
			planeChanged();
		} else {
			++i;
		}
	}
}

/// \brief Constructs \p winding from the intersection of \p plane with the other planes of the brush.
void Brush::windingForClipPlane (Winding& winding, const Plane3& plane) const
{
	FixedWinding buffer[2];
	bool swap = false;

	// get a poly that covers an effectively infinite area
	Winding_createInfinite(buffer[swap], plane, m_maxWorldCoord + 1);

	// chop the poly by all of the other faces
	{
		for (std::size_t i = 0; i < m_faces.size(); ++i) {
			const Face& clip = *m_faces[i];

			if (clip.plane3() == plane || !clip.plane3().isValid() || !plane_unique(i) || plane == -clip.plane3()) {
				continue;
			}

			buffer[!swap].clear();

			{
				// flip the plane, because we want to keep the back side
				Plane3 clipPlane(-clip.plane3().normal(), -clip.plane3().dist());
				Winding_Clip(buffer[swap], plane, clipPlane, i, buffer[!swap]);
			}

			swap = !swap;
		}
	}

	Winding_forFixedWinding(winding, buffer[swap]);
}

void Brush::update_wireframe (RenderableWireframe& wire, const bool* faces_visible) const
{
	wire.m_faceVertex.resize(m_edge_indices.size());
	wire.m_vertices = m_uniqueVertexPoints.data();
	wire.m_size = 0;
	for (std::size_t i = 0; i < m_edge_faces.size(); ++i) {
		if (faces_visible[m_edge_faces[i].first] || faces_visible[m_edge_faces[i].second]) {
			wire.m_faceVertex[wire.m_size++] = m_edge_indices[i];
		}
	}
}

void Brush::update_faces_wireframe (Array<PointVertex>& wire, const bool* faces_visible) const
{
	std::size_t count = 0;
	for (std::size_t i = 0; i < m_faceCentroidPoints.size(); ++i) {
		if (faces_visible[i]) {
			++count;
		}
	}

	wire.resize(count);
	Array<PointVertex>::iterator p = wire.begin();
	for (std::size_t i = 0; i < m_faceCentroidPoints.size(); ++i) {
		if (faces_visible[i]) {
			*p++ = m_faceCentroidPoints[i];
		}
	}
}

/// \brief Makes this brush a deep-copy of the \p other.
void Brush::copy (const Brush& other)
{
	for (Faces::const_iterator i = other.m_faces.begin(); i != other.m_faces.end(); ++i) {
		addFace(*(*i));
	}
	planeChanged();
}

void Brush::edge_push_back (FaceVertexId faceVertex)
{
	m_select_edges.push_back(SelectableEdge(m_faces, faceVertex));
	for (Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
		(*i)->edge_push_back(m_select_edges.back());
	}
}
void Brush::edge_clear ()
{
	m_select_edges.clear();
	for (Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
		(*i)->edge_clear();
	}
}
void Brush::vertex_push_back (FaceVertexId faceVertex)
{
	m_select_vertices.push_back(SelectableVertex(m_faces, faceVertex));
	for (Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
		(*i)->vertex_push_back(m_select_vertices.back());
	}
}
void Brush::vertex_clear ()
{
	m_select_vertices.clear();
	for (Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
		(*i)->vertex_clear();
	}
}

/// \brief Returns true if the face identified by \p index is preceded by another plane that takes priority over it.
bool Brush::plane_unique (std::size_t index) const
{
	// duplicate plane
	for (std::size_t i = 0; i < m_faces.size(); ++i) {
		if (index != i && !plane3_inside(m_faces[index]->plane3(), m_faces[i]->plane3())) {
			return false;
		}
	}
	return true;
}

/// \brief Removes edges that are smaller than the tolerance used when generating brush windings.
void Brush::removeDegenerateEdges ()
{
	for (std::size_t i = 0; i < m_faces.size(); ++i) {
		Winding& winding = m_faces[i]->getWinding();
		for (Winding::iterator j = winding.begin(); j != winding.end();) {
			std::size_t index = std::distance(winding.begin(), j);
			std::size_t next = winding.next(index);
			if (Edge_isDegenerate(winding[index].vertex, winding[next].vertex)) {
				Winding& other = m_faces[winding[index].adjacent]->getWinding();
				std::size_t adjacent = Winding_FindAdjacent(other, i);
				if (adjacent != c_brush_maxFaces) {
					other.erase(other.begin() + adjacent);
				}
				winding.erase(j);
			} else {
				++j;
			}
		}
	}
}

/// \brief Invalidates faces that have only two vertices in their winding, while preserving edge-connectivity information.
void Brush::removeDegenerateFaces ()
{
	// save adjacency info for degenerate faces
	for (std::size_t i = 0; i < m_faces.size(); ++i) {
		Winding& degen = m_faces[i]->getWinding();

		if (degen.size() == 2) {
			// this is an "edge" face, where the plane touches the edge of the brush
			{
				Winding& winding = m_faces[degen[0].adjacent]->getWinding();
				std::size_t index = Winding_FindAdjacent(winding, i);
				if (index != c_brush_maxFaces) {
					winding[index].adjacent = degen[1].adjacent;
				}
			}
			{
				Winding& winding = m_faces[degen[1].adjacent]->getWinding();
				std::size_t index = Winding_FindAdjacent(winding, i);
				if (index != c_brush_maxFaces) {
					winding[index].adjacent = degen[0].adjacent;
				}
			}

			degen.resize(0);
		}
	}
}

/// \brief Removes edges that have the same adjacent-face as their immediate neighbour.
void Brush::removeDuplicateEdges ()
{
	// verify face connectivity graph
	for (std::size_t i = 0; i < m_faces.size(); ++i) {
		Winding& winding = m_faces[i]->getWinding();
		for (std::size_t j = 0; j != winding.size();) {
			std::size_t next = winding.next(j);
			if (winding[j].adjacent == winding[next].adjacent) {
				winding.erase(winding.begin() + next);
			} else {
				++j;
			}
		}
	}
}

/// \brief Removes edges that do not have a matching pair in their adjacent-face.
void Brush::verifyConnectivityGraph ()
{
	// verify face connectivity graph
	for (std::size_t i = 0; i < m_faces.size(); ++i) {
		Winding& winding = m_faces[i]->getWinding();
		for (Winding::iterator j = winding.begin(); j != winding.end();) {
			// remove unidirectional graph edges
			if ((*j).adjacent == c_brush_maxFaces || Winding_FindAdjacent(m_faces[(*j).adjacent]->getWinding(), i)
					== c_brush_maxFaces) {
				winding.erase(j);
			} else {
				++j;
			}
		}
	}
}

/// \brief Returns true if the brush is a finite volume. A brush without a finite volume extends past the maximum world bounds and is not valid.
bool Brush::isBounded ()
{
	for (const_iterator i = begin(); i != end(); ++i) {
		if (!(*i)->is_bounded()) {
			return false;
		}
	}
	return true;
}

/// \brief Constructs the polygon windings for each face of the brush. Also updates the brush bounding-box and face texture-coordinates.
bool Brush::buildWindings ()
{
	m_aabb_local = AABB();

	for (std::size_t i = 0; i < m_faces.size(); ++i) {
		Face& f = *m_faces[i];

		if (!f.plane3().isValid() || !plane_unique(i)) {
			f.getWinding().resize(0);
		} else {
			windingForClipPlane(f.getWinding(), f.plane3());

			// update brush bounds
			const Winding& winding = f.getWinding();
			for (Winding::const_iterator i = winding.begin(); i != winding.end(); ++i) {
				aabb_extend_by_point_safe(m_aabb_local, (*i).vertex);
			}

			// update texture coordinates
			f.EmitTextureCoordinates();
		}
	}

	const bool degenerate = !isBounded();

	if (!degenerate) {
		// clean up connectivity information.
		// these cleanups must be applied in a specific order.
		removeDegenerateEdges();
		removeDegenerateFaces();
		removeDuplicateEdges();
		verifyConnectivityGraph();
	}

	return degenerate;
}

// Returns TRUE if any of the brush's faces has a visible material, FALSE if all faces are effectively hidden
bool Brush::hasVisibleMaterial () const
{
	// Traverse the faces
	for (Faces::const_iterator i = m_faces.begin(); i != m_faces.end(); ++i)
	{
		if (GlobalFilterSystem().isVisible(FilterRule::TYPE_TEXTURE, (*i)->getShader().m_shader))
		{
			return true; // return true on first visible material
		}
	}

	// no visible material
	return false;
}

/** @todo replace this garbage by code that calculates real centroid (current code just calculates mean of face centroids) */
Vector3 Brush::getCentroid () const
{
	Vector3 mean(0,0,0);
	int faceCount = m_faceCentroidPoints.size();

	for (std::size_t i = 0; i < faceCount; ++i)
		mean +=  m_faceCentroidPoints[i].vertex;

	mean /= faceCount;

	return mean;
}


struct SListNode
{
		SListNode* m_next;
};

class ProximalVertex
{
	public:
		const SListNode* m_vertices;

		ProximalVertex (const SListNode* next) :
			m_vertices(next)
		{
		}

		bool operator< (const ProximalVertex& other) const
		{
			if (!(operator==(other))) {
				return m_vertices < other.m_vertices;
			}
			return false;
		}
		bool operator== (const ProximalVertex& other) const
		{
			const SListNode* v = m_vertices;
			std::size_t DEBUG_LOOP = 0;
			do {
				if (v == other.m_vertices)
					return true;
				v = v->m_next;
				//ASSERT_MESSAGE(DEBUG_LOOP < c_brush_maxFaces, "infinite loop");
				if (!(DEBUG_LOOP < c_brush_maxFaces)) {
					break;
				}
				++DEBUG_LOOP;
			} while (v != m_vertices);
			return false;
		}
};

typedef Array<SListNode> ProximalVertexArray;
inline std::size_t ProximalVertexArray_index (const ProximalVertexArray& array, const ProximalVertex& vertex)
{
	return vertex.m_vertices - array.data();
}

/// \brief Constructs the face windings and updates anything that depends on them.
void Brush::buildBRep ()
{
	bool degenerate = buildWindings();

	static Vector3 colourVertexVec = ColourSchemes().getColourVector3("brush_vertices");
	static const Colour4b colour_vertex(int(colourVertexVec[0] * 255), int(colourVertexVec[1] * 255), int(colourVertexVec[2]
			* 255), 255);

	std::size_t faces_size = 0;
	std::size_t faceVerticesCount = 0;
	for (Faces::const_iterator i = m_faces.begin(); i != m_faces.end(); ++i) {
		if ((*i)->contributes()) {
			++faces_size;
		}
		faceVerticesCount += (*i)->getWinding().size();
	}

	if (degenerate || faces_size < 4 || faceVerticesCount != (faceVerticesCount >> 1) << 1) { // sum of vertices for each face of a valid polyhedron is always even
		m_uniqueVertexPoints.resize(0);

		vertex_clear();
		edge_clear();

		m_edge_indices.resize(0);
		m_edge_faces.resize(0);

		m_faceCentroidPoints.resize(0);
		m_uniqueEdgePoints.resize(0);
		m_uniqueVertexPoints.resize(0);

		for (Faces::iterator i = m_faces.begin(); i != m_faces.end(); ++i) {
			(*i)->getWinding().resize(0);
		}
	} else {
		{
			typedef std::vector<FaceVertexId> FaceVertices;
			FaceVertices faceVertices;
			faceVertices.reserve(faceVerticesCount);

			{
				for (std::size_t i = 0; i != m_faces.size(); ++i) {
					for (std::size_t j = 0; j < m_faces[i]->getWinding().size(); ++j) {
						faceVertices.push_back(FaceVertexId(i, j));
					}
				}
			}

			IndexBuffer uniqueEdgeIndices;
			typedef VertexBuffer<ProximalVertex> UniqueEdges;
			UniqueEdges uniqueEdges;

			uniqueEdgeIndices.reserve(faceVertices.size());
			uniqueEdges.reserve(faceVertices.size());

			{
				ProximalVertexArray edgePairs;
				edgePairs.resize(faceVertices.size());

				{
					for (std::size_t i = 0; i < faceVertices.size(); ++i) {
						edgePairs[i].m_next = edgePairs.data() + absoluteIndex(next_edge(m_faces, faceVertices[i]));
					}
				}

				{
					UniqueVertexBuffer<ProximalVertex> inserter(uniqueEdges);
					for (ProximalVertexArray::iterator i = edgePairs.begin(); i != edgePairs.end(); ++i) {
						uniqueEdgeIndices.insert(inserter.insert(ProximalVertex(&(*i))));
					}
				}

				{
					edge_clear();
					m_select_edges.reserve(uniqueEdges.size());
					for (UniqueEdges::iterator i = uniqueEdges.begin(); i != uniqueEdges.end(); ++i) {
						edge_push_back(faceVertices[ProximalVertexArray_index(edgePairs, *i)]);
					}
				}

				{
					m_edge_faces.resize(uniqueEdges.size());
					for (std::size_t i = 0; i < uniqueEdges.size(); ++i) {
						FaceVertexId faceVertex = faceVertices[ProximalVertexArray_index(edgePairs, uniqueEdges[i])];
						m_edge_faces[i] = EdgeFaces(faceVertex.getFace(),
								m_faces[faceVertex.getFace()]->getWinding()[faceVertex.getVertex()].adjacent);
					}
				}

				{
					m_uniqueEdgePoints.resize(uniqueEdges.size());
					for (std::size_t i = 0; i < uniqueEdges.size(); ++i) {
						FaceVertexId faceVertex = faceVertices[ProximalVertexArray_index(edgePairs, uniqueEdges[i])];

						const Winding& w = m_faces[faceVertex.getFace()]->getWinding();
						Vector3 edge = vector3_mid(w[faceVertex.getVertex()].vertex, w[w.next(faceVertex.getVertex())].vertex);
						m_uniqueEdgePoints[i] = PointVertex(edge, colour_vertex);
					}
				}

			}

			IndexBuffer uniqueVertexIndices;
			typedef VertexBuffer<ProximalVertex> UniqueVertices;
			UniqueVertices uniqueVertices;

			uniqueVertexIndices.reserve(faceVertices.size());
			uniqueVertices.reserve(faceVertices.size());

			{
				ProximalVertexArray vertexRings;
				vertexRings.resize(faceVertices.size());

				{
					for (std::size_t i = 0; i < faceVertices.size(); ++i) {
						vertexRings[i].m_next = vertexRings.data() + absoluteIndex(
								next_vertex(m_faces, faceVertices[i]));
					}
				}

				{
					UniqueVertexBuffer<ProximalVertex> inserter(uniqueVertices);
					for (ProximalVertexArray::iterator i = vertexRings.begin(); i != vertexRings.end(); ++i) {
						uniqueVertexIndices.insert(inserter.insert(ProximalVertex(&(*i))));
					}
				}

				{
					vertex_clear();
					m_select_vertices.reserve(uniqueVertices.size());
					for (UniqueVertices::iterator i = uniqueVertices.begin(); i != uniqueVertices.end(); ++i) {
						vertex_push_back(faceVertices[ProximalVertexArray_index(vertexRings, (*i))]);
					}
				}

				{
					m_uniqueVertexPoints.resize(uniqueVertices.size());
					for (std::size_t i = 0; i < uniqueVertices.size(); ++i) {
						FaceVertexId faceVertex =
								faceVertices[ProximalVertexArray_index(vertexRings, uniqueVertices[i])];

						const Winding& winding = m_faces[faceVertex.getFace()]->getWinding();
						m_uniqueVertexPoints[i] = PointVertex(winding[faceVertex.getVertex()].vertex, colour_vertex);
					}
				}
			}

			if ((uniqueVertices.size() + faces_size) - uniqueEdges.size() != 2) {
				globalErrorStream() << "Final B-Rep: inconsistent vertex count\n";
			}

#if BRUSH_CONNECTIVITY_DEBUG
			if ((uniqueVertices.size() + faces_size) - uniqueEdges.size() != 2) {
				for (Faces::iterator i = m_faces.begin(); i != m_faces.end(); ++i) {
					std::size_t faceIndex = std::distance(m_faces.begin(), i);

					if (!(*i)->contributes()) {
						globalOutputStream() << "face: " << string::toString(faceIndex) << " does not contribute\n";
					}

					Winding_printConnectivity((*i)->getWinding());
				}
			}
#endif

			// edge-index list for wireframe rendering
			{
				m_edge_indices.resize(uniqueEdgeIndices.size());

				for (std::size_t i = 0, count = 0; i < m_faces.size(); ++i) {
					const Winding& winding = m_faces[i]->getWinding();
					for (std::size_t j = 0; j < winding.size(); ++j) {
						const RenderIndex edge_index = uniqueEdgeIndices[count + j];

						m_edge_indices[edge_index].first = uniqueVertexIndices[count + j];
						m_edge_indices[edge_index].second = uniqueVertexIndices[count + winding.next(j)];
					}
					count += winding.size();
				}
			}
		}

		{
			m_faceCentroidPoints.resize(m_faces.size());
			for (std::size_t i = 0; i < m_faces.size(); ++i) {
				m_faces[i]->construct_centroid();
				m_faceCentroidPoints[i] = PointVertex(m_faces[i]->centroid(), colour_vertex);
			}
		}
	}
}

double Brush::m_maxWorldCoord = 0;
Shader* Brush::m_state_point;
