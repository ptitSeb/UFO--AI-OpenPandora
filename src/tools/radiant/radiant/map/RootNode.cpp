#include "RootNode.h"

#include "nameable.h"
#include "traverselib.h"
#include "transformlib.h"
#include "scenelib.h"
#include "string/string.h"
#include "instancelib.h"
#include "selectionlib.h"
#include "generic/callback.h"

namespace map {

RootNode::RootNode (const std::string& name) :
	_name(name)
{
	// Apply root status to this node
	setIsRoot(true);

	// Attach the InstanceSet as scene::Traversable::Observer
	// to the TraversableNodeset >> triggers instancing.
	m_traverse.attach(&m_instances);

	GlobalUndoSystem().trackerAttach(m_changeTracker);
}
RootNode::~RootNode ()
{
	// Override the default release() method
	GlobalUndoSystem().trackerDetach(m_changeTracker);

	// Remove the observer InstanceSet from the TraversableNodeSet
	m_traverse.detach(&m_instances);
}

// scene::Traversable Implementation
void RootNode::insert (Node& node)
{
	m_traverse.insert(node);
}
void RootNode::erase (Node& node)
{
	m_traverse.erase(node);
}
void RootNode::traverse (const Walker& walker)
{
	m_traverse.traverse(walker);
}
bool RootNode::empty () const
{
	return m_traverse.empty();
}

// TransformNode implementation
const Matrix4& RootNode::localToParent () const
{
	return m_transform.localToParent();
}

// MapFile implementation
void RootNode::save ()
{
	m_changeTracker.save();
}
bool RootNode::saved () const
{
	return m_changeTracker.saved();
}
void RootNode::changed ()
{
	m_changeTracker.changed();
}
void RootNode::setChangedCallback (const Callback& changed)
{
	m_changeTracker.setChangedCallback(changed);
}
std::size_t RootNode::changes () const
{
	return m_changeTracker.changes();
}

// Nameable implementation
std::string RootNode::name () const
{
	return _name;
}

InstanceCounter m_instanceCounter;
void RootNode::instanceAttach (const scene::Path& path)
{
	if (++m_instanceCounter.m_count == 1) {
		m_traverse.instanceAttach(path_find_mapfile(path.begin(), path.end()));
	}
}
void RootNode::instanceDetach (const scene::Path& path)
{
	if (--m_instanceCounter.m_count == 0) {
		m_traverse.instanceDetach(path_find_mapfile(path.begin(), path.end()));
	}
}

scene::Node& RootNode::clone () const
{
	return *(new RootNode(*this));
}

scene::Instance* RootNode::create (const scene::Path& path, scene::Instance* parent)
{
	return new SelectableInstance(path, parent);
}
void RootNode::forEachInstance (const scene::Instantiable::Visitor& visitor)
{
	m_instances.forEachInstance(visitor);
}
void RootNode::insert (scene::Instantiable::Observer* observer, const scene::Path& path, scene::Instance* instance)
{
	m_instances.insert(observer, path, instance);
	instanceAttach(path);
}
scene::Instance* RootNode::erase (scene::Instantiable::Observer* observer, const scene::Path& path)
{
	instanceDetach(path);
	return m_instances.erase(observer, path);
}

} // namespace map
