#ifndef ITRAVERSABLE_H_
#define ITRAVERSABLE_H_

#include "generic/constant.h"

namespace scene {

// Forward Declaration
class Node;

/** greebo: A Traversable is basically an element which can
 * 			have children and provides some basic methods
 * 			to insert/erase and traverse them.
 *
 * 			Currently, the standard implementation
 * 			of this is TraversableNodeSet (see traverselib.h)
 */
class Traversable
{
	public:
		STRING_CONSTANT(Name, "scene::Traversable");

		class Observer
		{
			public:
				virtual ~Observer ()
				{
				}

				/** @brief Called when a node is added to the container. */
				virtual void insertChild (Node& node) = 0;
				/** @brief Called when a node is removed from the container. */
				virtual void eraseChild (Node& node) = 0;
		};

		class Walker
		{
			public:
				virtual ~Walker ()
				{
				}

				/** @brief Called before traversing the first child-node of 'node'. If the return value is false, the children of the current node are not traversed. */
				virtual bool pre (Node& node) const = 0;
				/** @brief Called after traversing the last child-node of 'node'. */
				virtual void post (Node& node) const
				{
				}
		};

		virtual ~Traversable ()
		{
		}

		/** @brief Adds a node to the container. */
		virtual void insert (Node& node) = 0;
		/** @brief Removes a node from the container. */
		virtual void erase (Node& node) = 0;
		/** @brief Traverses the subgraphs rooted at each node in the container, depth-first. */
		virtual void traverse (const Walker& walker) = 0;
		/** @brief Returns true if the container contains no nodes. */
		virtual bool empty () const = 0;
};

} // namespace scene

#endif /*ITRAVERSABLE_H_*/
