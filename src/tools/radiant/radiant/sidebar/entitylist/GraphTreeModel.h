#ifndef GRAPHTREEMODEL_H_
#define GRAPHTREEMODEL_H_

#include <gtk/gtktreestore.h>
#include <gtk/gtktreeselection.h>
#include <map>
#include "iscenegraph.h"
#include "GraphTreeNode.h"

namespace ui {

/**
 * greebo: This wraps around a GtkTreeModel which can be used
 *         in a GtkTreeView visualisation.
 *
 * The class provides basic routines to insert/remove scene::Instances
 * into the model (the lookup should be performed fast).
 */
class GraphTreeModel: public scene::Graph::Observer {
public:
	// The enumeration of GTK column names
	enum {
		COL_INSTANCE_POINTER, // this is scene::Instance*
		COL_NAME, // the name (caption)
		NUM_COLS
	};

private:
	// This maps scene::Nodes to TreeNode structures to allow fast lookups in the tree
	typedef std::map<scene::Node*, GraphTreeNode*> NodeMap;
	NodeMap _nodemap;

	// The actual GTK model
	GtkTreeStore* _model;
public:
	GraphTreeModel ();

	~GraphTreeModel ();

	// Inserts the instance into the tree, returns the GtkTreeIter*
	const GraphTreeNode* insert (const scene::Instance& instance);
	// Removes the given instance from the tree
	void erase (const scene::Instance& instance);

	// Tries to lookup the given instance in the tree, can return the NULL node
	GraphTreeNode* find (const scene::Instance& instance);

	// Remove everything from the TreeModel
	void clear ();

	// Rebuilds the entire tree using a scene::Graph::Walker
	void refresh ();

	// Updates the selection status of the entire tree
	void updateSelectionStatus (GtkTreeSelection* selection);

	// Updates the selection status of the given instance only
	void updateSelectionStatus (GtkTreeSelection* selection, scene::Instance& instance);

	// Operator-cast to GtkTreeModel to allow for implicit conversion
	operator GtkTreeModel* ();

	// scene::Graph::Observer implementation

	// Gets called when a new <instance> is inserted into the scenegraph
	virtual void onSceneNodeInsert (const scene::Instance& instance);

	// Gets called when <instance> is removed from the scenegraph
	void onSceneNodeErase (const scene::Instance& instance);

private:
	// Looks up the parent of the given instance, can return NULL
	GraphTreeNode* findParentNode (const scene::Instance& instance);

	// Tries to lookup the iterator to the parent item of the given instance,
	// returns NULL if not found
	GtkTreeIter* findParentIter (const scene::Instance& instance);

	// Get the caption string used to display the node in the tree
	std::string getNodeCaption (scene::Node& node);
};

} // namespace ui

#endif /*GRAPHTREEMODEL_H_*/
