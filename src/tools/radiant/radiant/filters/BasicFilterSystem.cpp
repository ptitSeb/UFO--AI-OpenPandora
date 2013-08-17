#include "BasicFilterSystem.h"
#include "InstanceUpdateWalker.h"

#include "generic/callback.h"

#include "iradiant.h"
#include "itextstream.h"
#include "iscenegraph.h"
#include "iregistry.h"
#include "ieventmanager.h"
namespace filters
{

namespace {

	// Registry key for .game-defined filters
	const std::string RKEY_GAME_FILTERS = "game/filtersystem//filter";

	const std::string RKEY_USER_FILTER_BASE = "user/ui/filtersystem";

	// Registry key for user-defined filters
	const std::string RKEY_USER_FILTERS = RKEY_USER_FILTER_BASE + "/filters//filter";

	// Registry key for persistent filter setting
	const std::string RKEY_USER_ACTIVE_FILTERS = RKEY_USER_FILTER_BASE + "//activeFilter";
}


void BasicFilterSystem::onRadiantStartup()
{
	// Ask the XML Registry for filter nodes (from .game file and from user's filters.xml)
	xml::NodeList filters = GlobalRegistry().findXPath(RKEY_GAME_FILTERS);
	xml::NodeList userFilters = GlobalRegistry().findXPath(RKEY_USER_FILTERS);

	std::cout << "[filters] Loaded " << (filters.size() + userFilters.size())
			  << " filters from registry." << "\n";

	// Read-only filters
	addFiltersFromXML(filters, true);

	// user-defined filters
	addFiltersFromXML(userFilters, false);
}

void BasicFilterSystem::onRadiantShutdown()
{
	shutdown();
}

// Initialise the filter system
void BasicFilterSystem::init()
{
	GlobalRadiant().addEventListener(this);
}

void BasicFilterSystem::addFiltersFromXML(const xml::NodeList& nodes, bool readOnly) {
	// Load the list of active filter names from the user tree. There is no
	// guarantee that these are actually valid filters in the .game file
	std::set<std::string> activeFilterNames;
	xml::NodeList activeFilters = GlobalRegistry().findXPath(RKEY_USER_ACTIVE_FILTERS);

	for (xml::NodeList::const_iterator i = activeFilters.begin();
		 i != activeFilters.end();
		 ++i)
	{
		// Add the name of this filter to the set
		activeFilterNames.insert(i->getAttributeValue("name"));
	}

	// Iterate over the list of nodes, adding filter objects onto the list
	for (xml::NodeList::const_iterator iter = nodes.begin();
		 iter != nodes.end();
		 ++iter)
	{
		// Initialise the XMLFilter object
		std::string filterName = iter->getAttributeValue("name");
		XMLFilter filter(filterName, readOnly);

		// Get all of the filterCriterion children of this node
		xml::NodeList critNodes = iter->getNamedChildren("filterCriterion");

		// Create XMLFilterRule objects for each criterion
		for (xml::NodeList::iterator critIter = critNodes.begin();
			 critIter != critNodes.end();
			 ++critIter)
		{

			std::string typeStr = critIter->getAttributeValue("type");
			bool show = critIter->getAttributeValue("action") == "show";
			std::string match = critIter->getAttributeValue("match");

			if (typeStr == "texture") {
				filter.addRule(FilterRule::TYPE_TEXTURE, match, show);
			} else if (typeStr == "entityclass") {
				filter.addRule(FilterRule::TYPE_ENTITYCLASS, match, show);
			} else if (typeStr == "surfaceflags") {
				filter.addRule(FilterRule::TYPE_SURFACEFLAGS, match, show);
			} else if (typeStr == "contentflags") {
				filter.addRule(FilterRule::TYPE_CONTENTFLAGS, match, show);
			} else if (typeStr == "entitykeyvalue") {
				filter.addEntityKeyValueRule(critIter->getAttributeValue("key"),
						match, show);
			}
		}

		// Add this XMLFilter to the list of available filters
		XMLFilter& inserted = _availableFilters.insert(
			FilterTable::value_type(filterName, filter)
		).first->second;

		// Add the according toggle command to the eventmanager
		IEvent* fEvent = GlobalEventManager().addToggle(
			filter.getEventName(),
			MemberCaller<XMLFilter, &XMLFilter::onToggle>(inserted)
		);

		// If this filter is in our active set, enable it
		if (activeFilterNames.find(filterName) != activeFilterNames.end()) {
			fEvent->setToggled(true);
			_activeFilters.insert(
				FilterTable::value_type(filterName, inserted)
			);
		}
	}
}

// Shut down the Filters module, saving active filters to registry
void BasicFilterSystem::shutdown() {

	// Remove the existing set of active filter nodes
	GlobalRegistry().deleteXPath(RKEY_USER_ACTIVE_FILTERS);

	// Add a node for each active filter
	for (FilterTable::const_iterator i = _activeFilters.begin();
		 i != _activeFilters.end();
		 ++i)
	{
		GlobalRegistry().createKeyWithName(
			RKEY_USER_FILTER_BASE, "activeFilter", i->first
		);
	}

	// Save user-defined filters too (delete all first)
	GlobalRegistry().deleteXPath(RKEY_USER_FILTER_BASE + "/filters");

	// Create the new top-level node
	xml::Node filterParent = GlobalRegistry().createKey(RKEY_USER_FILTER_BASE + "/filters");

	for (FilterTable::iterator i = _availableFilters.begin();
		 i != _availableFilters.end();
		 ++i)
	{
		// Don't save stock filters
		if (i->second.isReadOnly()) continue;

		// Create a new filter node with a name
		xml::Node filter = filterParent.createChild("filter");
		filter.setAttributeValue("name", i->first);

		// Save all the rules as children to that node
		FilterRules ruleSet = i->second.getRuleSet();

		for (FilterRules::const_iterator r = ruleSet.begin(); r != ruleSet.end(); ++r) {
			// Create a new criterion tag
			xml::Node criterion = filter.createChild("filterCriterion");
			std::string typeStr;

			switch (r->type) {
			case FilterRule::TYPE_TEXTURE:
				typeStr = "texture";
				break;
			case FilterRule::TYPE_CONTENTFLAGS:
				typeStr = "contentflags";
				break;
			case FilterRule::TYPE_SURFACEFLAGS:
				typeStr = "surfaceflags";
				break;
			case FilterRule::TYPE_ENTITYCLASS:
				typeStr = "entityclass";
				break;
			case FilterRule::TYPE_ENTITYKEYVALUE:
				typeStr = "entitykeyvalue";
				criterion.setAttributeValue("key", r->entityKey);
				break;
			default:
				continue;
			};

			criterion.setAttributeValue("type", typeStr);
			criterion.setAttributeValue("match", r->match);
			criterion.setAttributeValue("action", r->show ? "show" : "hide");
		}
	}
}

void BasicFilterSystem::addObserver(const Observer* observer) {
	_observers.insert(observer);
}

void BasicFilterSystem::removeObserver(const Observer* observer) {
	_observers.erase(observer);
}

void BasicFilterSystem::forEachFilter(IFilterVisitor& visitor) {

	// Visit each filter on the list, passing the name to the visitor
	for (FilterTable::iterator iter = _availableFilters.begin();
		 iter != _availableFilters.end();
		 ++iter)
	{
		visitor.visit(iter->first);
	}
}

std::string BasicFilterSystem::getFilterEventName(const std::string& filter) {
	FilterTable::iterator f = _availableFilters.find(filter);

	if (f != _availableFilters.end()) {
		return f->second.getEventName();
	}
	else {
		return "";
	}
}

void BasicFilterSystem::notifyObservers() {
	// Traverse the set
	for (ObserverList::const_iterator i = _observers.begin(); i != _observers.end(); ++i) {
		// Call each observer
		(*i)->onFiltersChanged();
	}
}

// Change the state of a named filter
void BasicFilterSystem::setFilterState(const std::string& filter, bool state) {

	if (state) {
		// Copy the filter to the active filters list
		_activeFilters.insert(
			FilterTable::value_type(
				filter, _availableFilters.find(filter)->second));
	}
	else {
		// Remove filter from active filters list
		_activeFilters.erase(filter);
	}

	// Invalidate the visibility cache to force new values to be
	// loaded from the filters themselves
	_visibilityCache.clear();

	// Update the scenegraph instances
	updateInstances();

	notifyObservers();

	// Trigger an immediate scene redraw
	GlobalSceneGraph().sceneChanged();
}

void BasicFilterSystem::updateEvents() {
	for (FilterTable::const_iterator iter = _availableFilters.begin();
		 iter != _availableFilters.end();
		 ++iter)
	{
		IEvent* toggle = GlobalEventManager().findEvent(iter->second.getEventName());

		if (toggle == NULL)
			continue;

		toggle->setToggled(getFilterState(iter->first));
	}
}

bool BasicFilterSystem::filterIsReadOnly(const std::string& filter) {
	FilterTable::const_iterator f = _availableFilters.find(filter);

	if (f != _availableFilters.end()) {
		return f->second.isReadOnly();
	}
	else {
		// Filter not found, return "read-only" just in case
		return true;
	}
}

bool BasicFilterSystem::addFilter(const std::string& filterName, const FilterRules& ruleSet) {
	FilterTable::iterator f = _availableFilters.find(filterName);

	if (f != _availableFilters.end()) {
		return false; // already exists
	}

	std::pair<FilterTable::iterator, bool> result = _availableFilters.insert(
		FilterTable::value_type(filterName, XMLFilter(filterName, false))
	);

	// Apply the ruleset
	result.first->second.setRules(ruleSet);

	// Add the according toggle command to the eventmanager
	GlobalEventManager().addToggle(
		result.first->second.getEventName(),
		MemberCaller<XMLFilter, &XMLFilter::onToggle>(result.first->second)
	);

	// Clear the cache, the rules have changed
	_visibilityCache.clear();

	notifyObservers();

	return true;
}

bool BasicFilterSystem::removeFilter(const std::string& filter) {
	FilterTable::iterator f = _availableFilters.find(filter);

	if (f != _availableFilters.end()) {
		if (f->second.isReadOnly()) {
			return false;
		}

		// Remove all accelerators from that event before removal
		GlobalEventManager().disconnectAccelerator(f->second.getEventName());

		// Disable the event in the EventManager, to avoid crashes when calling the menu items
		GlobalEventManager().disableEvent(f->second.getEventName());

		// Check if the filter was active
		FilterTable::iterator found = _activeFilters.find(f->first);

		if (found != _activeFilters.end()) {
			_activeFilters.erase(found);
		}

		// Now remove the object from the available filters too
		_availableFilters.erase(f);

		// Clear the cache, the rules have changed
		_visibilityCache.clear();

		notifyObservers();

		return true;
	}
	else {
		// Filter not found
		return false;
	}
}

bool BasicFilterSystem::renameFilter(const std::string& oldFilterName, const std::string& newFilterName) {
	// Check if the new name is already used
	FilterTable::iterator c = _availableFilters.find(newFilterName);

	if (c != _availableFilters.end()) {
		// Can't rename, name is already in use
		return false;
	}

	FilterTable::iterator f = _availableFilters.find(oldFilterName);

	if (f != _availableFilters.end()) {
		// Check for read-only filters
		if (f->second.isReadOnly()) {
			return false;
		}

		// Check if the filter was active
		FilterTable::iterator found = _activeFilters.find(f->first);

		bool wasActive = (found != _activeFilters.end());

		if (wasActive) {
			_activeFilters.erase(found);
		}

		std::string oldEventName = f->second.getEventName();

		IEvent* oldEvent = GlobalEventManager().findEvent(oldEventName);

		// Get the accelerator associated to the old event, if appropriate
		IAccelerator* oldAccel = GlobalEventManager().findAccelerator(oldEvent);

		// Perform the actual rename procedure
		f->second.setName(newFilterName);

		// Insert the new filter into the table
		std::pair<FilterTable::iterator, bool> result = _availableFilters.insert(
			FilterTable::value_type(newFilterName, f->second)
		);

		// Add the according toggle command to the eventmanager
		IEvent* fEvent = GlobalEventManager().addToggle(
			result.first->second.getEventName(),
			MemberCaller<XMLFilter, &XMLFilter::onToggle>(result.first->second)
		);

		if (fEvent != NULL) {
			GlobalEventManager().connectAccelerator(oldAccel, f->second.getEventName());

			// If this filter is in our active set, enable it
			if (wasActive) {
				fEvent->setToggled(true);

				_activeFilters.insert(
					FilterTable::value_type(newFilterName, f->second)
				);
			}
			else {
				fEvent->setToggled(false);
			}
		}
		else {
			globalWarningStream()
				<< "Can't register event after rename, the new event name is already registered!"
				<< "\n";
		}

		// Remove the old filter from the filtertable
		_availableFilters.erase(oldFilterName);

		// Remove the old event from the EventManager
		GlobalEventManager().removeEvent(oldEventName);

		return true;
	}
	else {
		// Filter not found
		return false;
	}
}

// Query whether an item is visible or filtered out
bool BasicFilterSystem::isVisible(const FilterRule::Type type,
								  const std::string& name)
{
	// Check if this item is in the visibility cache, returning
	// its cached value if found
	StringFlagCache::iterator cacheIter = _visibilityCache.find(name);
	if (cacheIter != _visibilityCache.end())
		return cacheIter->second;

	// Otherwise, walk the list of active filters to find a value for
	// this item.
	bool visFlag = true; // default if no filters modify it

	for (FilterTable::iterator activeIter = _activeFilters.begin();
		 activeIter != _activeFilters.end();
		 ++activeIter)
	{
		// Delegate the check to the filter object. If a filter returns
		// false for the visibility check, then the item is filtered
		// and we don't need any more checks.
		if (!activeIter->second.isVisible(type, name)) {
			visFlag = false;
			break;
		}
	}

	// Cache the result and return to caller
	_visibilityCache.insert(StringFlagCache::value_type(name, visFlag));
	return visFlag;
}

bool BasicFilterSystem::isVisible (const FilterRule::Type type, int flags)
{
	return isVisible(type, string::toString(flags));
}

bool BasicFilterSystem::isEntityVisible(const FilterRule::Type type, const Entity& entity)
{
       // Otherwise, walk the list of active filters to find a value for
	// this item.
	bool visFlag = true; // default if no filters modify it

	for (FilterTable::iterator activeIter = _activeFilters.begin();
			activeIter != _activeFilters.end(); ++activeIter) {
		// Delegate the check to the filter object. If a filter returns
		// false for the visibility check, then the item is filtered
		// and we don't need any more checks.
		if (!activeIter->second.isEntityVisible(type, entity)) {
			visFlag = false;
			break;
		}
	}

	return visFlag;
}

FilterRules BasicFilterSystem::getRuleSet(const std::string& filter) {
	FilterTable::iterator f = _availableFilters.find(filter);

	if (f != _availableFilters.end()) {
		return f->second.getRuleSet();
	}

	return FilterRules();
}

bool BasicFilterSystem::setFilterRules(const std::string& filter, const FilterRules& ruleSet) {
	FilterTable::iterator f = _availableFilters.find(filter);

	if (f != _availableFilters.end() && !f->second.isReadOnly()) {
		// Apply the ruleset
		f->second.setRules(ruleSet);

		// Clear the cache, the ruleset has changed
		_visibilityCache.clear();

		notifyObservers();

		return true;
	}

	return false; // not found or readonly
}

// Update scenegraph instances with filtered status
void BasicFilterSystem::updateInstances() {

	// Construct an InstanceUpdateWalker and traverse the scenegraph to update
	// all instances
	InstanceUpdateWalker walker;
	GlobalSceneGraph().traverse(walker);
}

}
