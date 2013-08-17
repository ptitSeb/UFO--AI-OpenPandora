#ifndef CULLINGWALKER_H_
#define CULLINGWALKER_H_

/**
 * Walker wrapper class which invokes its child walker for instances which pass
 * a Cullable::intersectVolume test.
 */
template<typename _Walker>
class CullingWalker
{
		const VolumeTest& m_volume;
		const _Walker& m_walker;
	public:
		// Constructor
		CullingWalker (const VolumeTest& volume, const _Walker& walker) :
			m_volume(volume), m_walker(walker)
		{
		}

		// Pre-descent function
		bool pre (const scene::Path& path, scene::Instance& instance, VolumeIntersectionValue parentVisible) const
		{
			VolumeIntersectionValue visible = Cullable_testVisible(instance, m_volume, parentVisible);

			if (visible != VOLUME_OUTSIDE) {
				return m_walker.pre(path, instance);
			}
			return true;
		}

		// Post-descent function
		void post (const scene::Path& path, scene::Instance& instance, VolumeIntersectionValue parentVisible) const
		{
			m_walker.post(path, instance);
		}
};

#endif /*CULLINGWALKER_H_*/
