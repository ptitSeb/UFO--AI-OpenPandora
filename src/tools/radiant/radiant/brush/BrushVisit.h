#ifndef BRUSHVISIT_H_
#define BRUSHVISIT_H_

#include "iselection.h"
#include "BrushInstance.h"

template<typename Functor>
class BrushSelectedVisitor: public SelectionSystem::Visitor
{
		const Functor& m_functor;
	public:
		BrushSelectedVisitor (const Functor& functor) :
			m_functor(functor)
		{
		}
		void visit (scene::Instance& instance) const
		{
			BrushInstance* brush = Instance_getBrush(instance);
			if (brush != 0) {
				m_functor(*brush);
			}
		}
};

template<typename Functor>
inline const Functor& Scene_forEachSelectedBrush (const Functor& functor)
{
	GlobalSelectionSystem().foreachSelected(BrushSelectedVisitor<Functor> (functor));
	return functor;
}

class BrushForEachFace
{
		const BrushInstanceVisitor& m_visitor;
	public:
		BrushForEachFace (const BrushInstanceVisitor& visitor) :
			m_visitor(visitor)
		{
		}
		void operator() (BrushInstance& brush) const
		{
			brush.forEachFaceInstance(m_visitor);
		}
};

template<class Functor>
class FaceInstanceVisitFace: public BrushInstanceVisitor
{
		const Functor& functor;
	public:
		FaceInstanceVisitFace (const Functor& functor) :
			functor(functor)
		{
		}
		void visit (FaceInstance& face) const
		{
			functor(face.getFace());
		}
};

template<typename Functor>
inline const Functor& Brush_forEachFace (BrushInstance& brush, const Functor& functor)
{
	brush.forEachFaceInstance(FaceInstanceVisitFace<Functor> (functor));
	return functor;
}

template<class Functor>
class FaceVisitAll: public BrushVisitor
{
		const Functor& functor;
	public:
		FaceVisitAll (const Functor& functor) :
			functor(functor)
		{
		}
		void visit (Face& face) const
		{
			functor(face);
		}
};

template<typename Functor>
inline const Functor& Brush_forEachFace (const Brush& brush, const Functor& functor)
{
	brush.forEachFace(FaceVisitAll<Functor> (functor));
	return functor;
}

template<typename Functor>
inline const Functor& Brush_forEachFace (Brush& brush, const Functor& functor)
{
	brush.forEachFace(FaceVisitAll<Functor> (functor));
	return functor;
}

template<class Functor>
class FaceInstanceVisitAll: public BrushInstanceVisitor
{
		const Functor& functor;
	public:
		FaceInstanceVisitAll (const Functor& functor) :
			functor(functor)
		{
		}
		void visit (FaceInstance& face) const
		{
			functor(face);
		}
};

template<typename Functor>
inline const Functor& Brush_ForEachFaceInstance (BrushInstance& brush, const Functor& functor)
{
	brush.forEachFaceInstance(FaceInstanceVisitAll<Functor> (functor));
	return functor;
}

template<typename Functor>
inline const Functor& Scene_forEachBrush (scene::Graph& graph, const Functor& functor)
{
	graph.traverse(InstanceWalker<InstanceApply<BrushInstance, Functor> > (functor));
	return functor;
}

template<typename Type, typename Functor>
class InstanceIfVisible: public Functor
{
	public:
		InstanceIfVisible (const Functor& functor) :
			Functor(functor)
		{
		}
		void operator() (scene::Instance& instance)
		{
			if (instance.path().top().get().visible()) {
				Functor::operator()(instance);
			}
		}
};

template<typename Functor>
inline const Functor& Scene_ForEachBrush_ForEachFace (scene::Graph& graph, const Functor& functor)
{
	Scene_forEachBrush(graph, BrushForEachFace(FaceInstanceVisitFace<Functor> (functor)));
	return functor;
}

// d1223m
template<typename Functor>
inline const Functor& Scene_ForEachBrush_ForEachFaceInstance (scene::Graph& graph, const Functor& functor)
{
	Scene_forEachBrush(graph, BrushForEachFace(FaceInstanceVisitAll<Functor> (functor)));
	return functor;
}

template<typename Functor>
inline const Functor& Scene_ForEachSelectedBrush_ForEachFace (scene::Graph& graph, const Functor& functor)
{
	Scene_forEachSelectedBrush(BrushForEachFace(FaceInstanceVisitFace<Functor> (functor)));
	return functor;
}

template<typename Functor>
inline const Functor& Scene_ForEachSelectedBrush_ForEachFaceInstance (scene::Graph& graph, const Functor& functor)
{
	Scene_forEachSelectedBrush(BrushForEachFace(FaceInstanceVisitAll<Functor> (functor)));
	return functor;
}

template<typename Functor>
class FaceVisitorWrapper
{
		const Functor& functor;
	public:
		FaceVisitorWrapper (const Functor& functor) :
			functor(functor)
		{
		}

		void operator() (FaceInstance& faceInstance) const
		{
			functor(faceInstance.getFace());
		}
};

template<typename Functor>
inline const Functor& Scene_ForEachSelectedBrushFace (const Functor& functor)
{
	g_SelectedFaceInstances.foreach(FaceVisitorWrapper<Functor> (functor));
	return functor;
}

#endif /*BRUSHVISIT_H_*/
