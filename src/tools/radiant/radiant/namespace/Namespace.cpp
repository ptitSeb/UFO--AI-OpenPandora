#include "Namespace.h"

#include "scenelib.h"
#include <list>

namespace {
inline Namespaced* Node_getNamespaced (scene::Node& node)
{
	return dynamic_cast<Namespaced*> (&node);
}
}

void Namespace::attach (const NameCallback& setName, const NameCallbackCallback& attachObserver)
{
	std::pair<Names::iterator, bool> result = m_names.insert(Names::value_type(setName, m_uniqueNames));
	attachObserver(NameObserver::NameChangedCaller((*result.first).second));
}

void Namespace::detach (const NameCallback& setName, const NameCallbackCallback& detachObserver)
{
	Names::iterator i = m_names.find(setName);
	detachObserver(NameObserver::NameChangedCaller((*i).second));
	m_names.erase(i);
}

void Namespace::makeUnique (const std::string& name, const NameCallback& setName) const
{
	char buffer[1024];
	name_write(buffer, m_uniqueNames.make_unique(name_read(name)));
	setName(buffer);
}

void Namespace::mergeNames (const Namespace& other) const
{
	typedef std::list<NameCallback> SetNameCallbacks;
	typedef std::map<std::string, SetNameCallbacks> NameGroups;
	NameGroups groups;

	UniqueNames uniqueNames(other.m_uniqueNames);

	for (Names::const_iterator i = m_names.begin(); i != m_names.end(); ++i) {
		groups[(*i).second.getName()].push_back((*i).first);
	}

	for (NameGroups::iterator i = groups.begin(); i != groups.end(); ++i) {
		name_t uniqueName(uniqueNames.make_unique(name_read((*i).first)));
		uniqueNames.insert(uniqueName);

		char buffer[1024];
		name_write(buffer, uniqueName);

		SetNameCallbacks& setNameCallbacks = (*i).second;

		for (SetNameCallbacks::const_iterator j = setNameCallbacks.begin(); j != setNameCallbacks.end(); ++j) {
			(*j)(buffer);
		}
	}
}

void Namespace::gatherNamespaced (scene::Node& root)
{
	// Local helper class
	class GatherNamespacedWalker: public scene::Traversable::Walker
	{
			NamespacedList& _list;
		public:
			GatherNamespacedWalker (NamespacedList& list) :
				_list(list)
			{
			}

			bool pre (scene::Node& node) const
			{
				Namespaced* namespaced = Node_getNamespaced(node);
				if (namespaced != NULL) {
					_list.push_back(namespaced);
				}
				return true;
			}
	};

	Node_traverseSubgraph(root, GatherNamespacedWalker(_cloned));
}

void Namespace::mergeClonedNames ()
{
	// Temporarily allocate a new Namespace ("cloned")
	Namespace _clonedNamespace;

	// First, move them into the "cloned" Namespace
	for (unsigned int i = 0; i < _cloned.size(); i++) {
		_cloned[i]->setNamespace(_clonedNamespace);
	}

	// Merge them in
	_clonedNamespace.mergeNames(*this);

	// Now, move them (back) into this namespace (*this)
	for (unsigned int i = 0; i < _cloned.size(); i++) {
		_cloned[i]->setNamespace(*this);
	}

	// Remove the items from the list, we're done.
	_cloned.clear();
}
