/**
 * @file aabb.h
 * @brief Axis-aligned bounding-box data types and related operations.
 */

/*
 Copyright (C) 2001-2006, William Joseph.
 All Rights Reserved.

 This file is part of GtkRadiant.

 GtkRadiant is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 GtkRadiant is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with GtkRadiant; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#if !defined(INCLUDED_MATH_AABB_H)
#define INCLUDED_MATH_AABB_H

#include "math/matrix.h"
#include "math/Plane3.h"
#include <algorithm>

/** An Axis Aligned Bounding Box is a simple cuboid which encloses a given set of points,
 * such as the vertices of a model. It is defined by an origin, located at the centre of
 * the AABB, and symmetrical extents in 3 dimension which determine its size.
 */

class AABB
{
	public:
		/// The origin of the AABB, which is always located at the centre.
		Vector3 origin;

		/// The symmetrical extents in 3 dimensions.
		Vector3 extents;

		/** Construct an AABB with default origin and invalid extents.
		 */
		AABB () :
			origin(0, 0, 0), extents(-1, -1, -1)
		{
		}

		/** Construct an AABB with the provided origin and extents
		 * vectors.
		 */
		AABB (const Vector3& origin_, const Vector3& extents_) :
			origin(origin_), extents(extents_)
		{
		}

		/** Get the origin of this AABB.
		 *
		 * @returns
		 * A const reference to a Vector3 containing the AABB's origin.
		 */
		const Vector3& getOrigin () const
		{
			return origin;
		}

		const Vector3 getMins() const {
			return origin - extents;
		}

		const Vector3 getMaxs() const {
			return origin + extents;
		}

		/** Get the radius of the smallest sphere which encloses this
		 * bounding box.
		 */
		float getRadius () const
		{
			return extents.getLength(); // Pythagorean length of extents vector
		}

		/** Expand this AABB in-place to include the given point in
		 * world space.
		 *
		 * @param point
		 * Vector3 representing the point to include.
		 */
		void includePoint (const Vector3& point)
		{
			// Process each axis separately
			for (int i = 0; i < 3; ++i) {
				float axisDisp = point[i] - origin[i]; // axis displacement from origin to point
				float halfDif = 0.5 * (std::abs(axisDisp) - extents[i]); // half of extent increase needed (maybe negative if point inside)
				if (halfDif > 0) {
					origin[i] += (axisDisp > 0) ? halfDif : -halfDif;
					extents[i] += halfDif;
				}
			}
		}

		static AABB createFromMinMax (const Vector3& min, const Vector3& max)
		{
			AABB aabb;
			aabb.origin = vector3_mid(min, max);
			aabb.extents = max - aabb.origin;
			return aabb;
		}

		// Expand this AABB to include another AABB
		void includeAABB (const AABB& other)
		{
			// Validity check. If both this and other are valid, use the extension
			// algorithm. If only the other AABB is valid, set this AABB equal to it.
			// If neither are valid we do nothing.

			if (isValid() && other.isValid()) {
				// Extend each axis separately
				for (int i = 0; i < 3; ++i) {
					float displacement = other.origin[i] - origin[i];
					float difference = other.extents[i] - extents[i];
					if (fabs(displacement) > fabs(difference)) {
						float half_difference = static_cast<float> (0.5 * (fabs(displacement) + difference));
						if (half_difference > 0.0f) {
							origin[i] += (displacement >= 0.0f) ? half_difference : -half_difference;
							extents[i] += half_difference;
						}
					} else if (difference > 0.0f) {
						origin[i] = other.origin[i];
						extents[i] = other.extents[i];
					}
				}
			} else if (other.isValid()) {
				origin = other.origin;
				extents = other.extents;
			}
		}

		// Returns true if this AABB contains the other AABB (all dimensions)
		bool contains (const AABB& other) const
		{
			// Return true if all coordinates of <other> are contained within these bounds
			return (origin[0] + extents[0] >= other.origin[0] + other.extents[0]) && (origin[0] - extents[0]
					<= other.origin[0] - other.extents[0]) && (origin[1] + extents[1] >= other.origin[1]
					+ other.extents[1]) && (origin[1] - extents[1] <= other.origin[1] - other.extents[1]) && (origin[2]
					+ extents[2] >= other.origin[2] + other.extents[2]) && (origin[2] - extents[2] <= other.origin[2]
					- other.extents[2]);
		}

		// Check whether the AABB is valid, or if the extents are still uninitialised
		bool isValid() const {
			bool valid = true;

			// Check each origin and extents value. The origins must be between
			// +/- FLT_MAX, and the extents between 0 and FLT_MAX.
			for (int i = 0; i < 3; ++i) {
				if (origin[i] < -FLT_MAX
					|| origin[i] > FLT_MAX
					|| extents[i] < 0
					|| extents[i] > FLT_MAX)
				{
					valid = false;
				}
			}

			return valid;
		}

		/** Get the extents of this AABB.
		 *
		 * @returns
		 * A const reference to a Vector3 containing the AABB's extents.
		 */
		const Vector3& getExtents() const {
			return extents;
		}
};

const float c_aabb_max = FLT_MAX;

inline bool extents_valid (float f)
{
	return f >= 0.0f && f <= c_aabb_max;
}

inline bool origin_valid (float f)
{
	return f >= -c_aabb_max && f <= c_aabb_max;
}

inline bool aabb_valid (const AABB& aabb)
{
	return origin_valid(aabb.origin[0]) && origin_valid(aabb.origin[1]) && origin_valid(aabb.origin[2])
			&& extents_valid(aabb.extents[0]) && extents_valid(aabb.extents[1]) && extents_valid(aabb.extents[2]);
}

template<typename Index>
class AABBExtend
{
	public:
		static void apply (AABB& aabb, const AABB& other)
		{
			float displacement = other.origin[Index::VALUE] - aabb.origin[Index::VALUE];
			float difference = other.extents[Index::VALUE] - aabb.extents[Index::VALUE];
			if (fabs(displacement) > fabs(difference)) {
				float half_difference = static_cast<float> (0.5 * (fabs(displacement) + difference));
				if (half_difference > 0.0f) {
					aabb.origin[Index::VALUE] += (displacement >= 0.0f) ? half_difference : -half_difference;
					aabb.extents[Index::VALUE] += half_difference;
				}
			} else if (difference > 0.0f) {
				aabb.origin[Index::VALUE] = other.origin[Index::VALUE];
				aabb.extents[Index::VALUE] = other.extents[Index::VALUE];
			}
		}
};

inline void aabb_extend_by_point_safe (AABB& aabb, const Vector3& point)
{
	if (aabb_valid(aabb)) {
		aabb.includePoint(point);
	} else {
		aabb.origin = point;
		aabb.extents = Vector3(0, 0, 0);
	}
}

class AABBExtendByPoint
{
		AABB& m_aabb;
	public:
		AABBExtendByPoint (AABB& aabb) :
			m_aabb(aabb)
		{
		}
		void operator() (const Vector3& point) const
		{
			aabb_extend_by_point_safe(m_aabb, point);
		}
};

inline void aabb_extend_by_aabb (AABB& aabb, const AABB& other)
{
	AABBExtend<IntegralConstant<0> >::apply(aabb, other);
	AABBExtend<IntegralConstant<1> >::apply(aabb, other);
	AABBExtend<IntegralConstant<2> >::apply(aabb, other);
}

inline void aabb_extend_by_vec3 (AABB& aabb, const Vector3& extension)
{
	aabb.extents += extension;
}

template<typename Index>
inline bool aabb_intersects_point_dimension (const AABB& aabb, const Vector3& point)
{
	return fabs(point[Index::VALUE] - aabb.origin[Index::VALUE]) < aabb.extents[Index::VALUE];
}

inline bool aabb_intersects_point (const AABB& aabb, const Vector3& point)
{
	return aabb_intersects_point_dimension<IntegralConstant<0> > (aabb, point) && aabb_intersects_point_dimension<
			IntegralConstant<1> > (aabb, point) && aabb_intersects_point_dimension<IntegralConstant<2> > (aabb, point);
}

template<typename Index>
inline bool aabb_intersects_aabb_dimension (const AABB& aabb, const AABB& other)
{
	return fabs(other.origin[Index::VALUE] - aabb.origin[Index::VALUE]) < (aabb.extents[Index::VALUE]
			+ other.extents[Index::VALUE]);
}

inline bool aabb_intersects_aabb (const AABB& aabb, const AABB& other)
{
	return aabb_intersects_aabb_dimension<IntegralConstant<0> > (aabb, other) && aabb_intersects_aabb_dimension<
			IntegralConstant<1> > (aabb, other) && aabb_intersects_aabb_dimension<IntegralConstant<2> > (aabb, other);
}

inline unsigned int aabb_classify_plane (const AABB& aabb, const Plane3& plane)
{
	double distance_origin = plane.normal().dot(aabb.origin) + plane.dist();

	if (fabs(distance_origin) < (fabs(plane.normal().x() * aabb.extents[0]) + fabs(plane.normal().y() * aabb.extents[1]) + fabs(plane.normal().z()
			* aabb.extents[2]))) {
		return 1; // partially inside
	} else if (distance_origin < 0) {
		return 2; // totally inside
	}
	return 0; // totally outside
}

inline unsigned int aabb_oriented_classify_plane (const AABB& aabb, const Matrix4& transform, const Plane3& plane)
{
	double distance_origin = plane.normal().dot(aabb.origin) + plane.dist();

	if (fabs(distance_origin) < (fabs(aabb.extents[0] * plane.normal().dot(transform.x().getVector3())) + fabs(
			aabb.extents[1] * plane.normal().dot(transform.y().getVector3())) + fabs(aabb.extents[2]
			* plane.normal().dot(transform.z().getVector3())))) {
		return 1; // partially inside
	} else if (distance_origin < 0) {
		return 2; // totally inside
	}
	return 0; // totally outside
}

inline void aabb_corners (const AABB& aabb, Vector3 corners[8])
{
	Vector3 min(aabb.origin - aabb.extents);
	Vector3 max(aabb.origin + aabb.extents);
	corners[0] = Vector3(min[0], max[1], max[2]);
	corners[1] = Vector3(max[0], max[1], max[2]);
	corners[2] = Vector3(max[0], min[1], max[2]);
	corners[3] = Vector3(min[0], min[1], max[2]);
	corners[4] = Vector3(min[0], max[1], min[2]);
	corners[5] = Vector3(max[0], max[1], min[2]);
	corners[6] = Vector3(max[0], min[1], min[2]);
	corners[7] = Vector3(min[0], min[1], min[2]);
}

inline void aabb_corners_oriented (const AABB& aabb, const Matrix4& rotation, Vector3 corners[8])
{
	Vector3 x = rotation.x().getVector3() * aabb.extents.x();
	Vector3 y = rotation.y().getVector3() * aabb.extents.y();
	Vector3 z = rotation.z().getVector3() * aabb.extents.z();

	corners[0] = aabb.origin + -x + y + z;
	corners[1] = aabb.origin + x + y + z;
	corners[2] = aabb.origin + x + -y + z;
	corners[3] = aabb.origin + -x + -y + z;
	corners[4] = aabb.origin + -x + y + -z;
	corners[5] = aabb.origin + x + y + -z;
	corners[6] = aabb.origin + x + -y + -z;
	corners[7] = aabb.origin + -x + -y + -z;
}

inline void aabb_planes (const AABB& aabb, Plane3 planes[6])
{
	planes[0] = Plane3(g_vector3_axes[0], aabb.origin[0] + aabb.extents[0]);
	planes[1] = Plane3(-g_vector3_axes[0], -(aabb.origin[0] - aabb.extents[0]));
	planes[2] = Plane3(g_vector3_axes[1], aabb.origin[1] + aabb.extents[1]);
	planes[3] = Plane3(-g_vector3_axes[1], -(aabb.origin[1] - aabb.extents[1]));
	planes[4] = Plane3(g_vector3_axes[2], aabb.origin[2] + aabb.extents[2]);
	planes[5] = Plane3(-g_vector3_axes[2], -(aabb.origin[2] - aabb.extents[2]));
}

inline void aabb_planes_oriented (const AABB& aabb, const Matrix4& rotation, Plane3 planes[6])
{
	double x = rotation.x().getVector3().dot(aabb.origin);
	double y = rotation.y().getVector3().dot(aabb.origin);
	double z = rotation.z().getVector3().dot(aabb.origin);

	planes[0] = Plane3(rotation.x().getVector3(), x + aabb.extents[0]);
	planes[1] = Plane3(-rotation.x().getVector3(), -(x - aabb.extents[0]));
	planes[2] = Plane3(rotation.y().getVector3(), y + aabb.extents[1]);
	planes[3] = Plane3(-rotation.y().getVector3(), -(y - aabb.extents[1]));
	planes[4] = Plane3(rotation.z().getVector3(), z + aabb.extents[2]);
	planes[5] = Plane3(-rotation.z().getVector3(), -(z - aabb.extents[2]));
}

const Vector3 aabb_normals[6] = { Vector3(1, 0, 0), Vector3(0, 1, 0), Vector3(0, 0, 1), Vector3(-1, 0, 0), Vector3(0,
		-1, 0), Vector3(0, 0, -1), };

const float aabb_texcoord_topleft[2] = { 0, 0 };
const float aabb_texcoord_topright[2] = { 1, 0 };
const float aabb_texcoord_botleft[2] = { 0, 1 };
const float aabb_texcoord_botright[2] = { 1, 1 };

inline AABB aabb_for_oriented_aabb (const AABB& aabb, const Matrix4& transform)
{
	return AABB(matrix4_transformed_point(transform, aabb.origin), Vector3(static_cast<float> (fabs(transform[0]
			* aabb.extents[0]) + fabs(transform[4] * aabb.extents[1]) + fabs(transform[8] * aabb.extents[2])),
			static_cast<float> (fabs(transform[1] * aabb.extents[0]) + fabs(transform[5] * aabb.extents[1]) + fabs(
					transform[9] * aabb.extents[2])), static_cast<float> (fabs(transform[2] * aabb.extents[0]) + fabs(
					transform[6] * aabb.extents[1]) + fabs(transform[10] * aabb.extents[2]))));
}

inline AABB aabb_for_oriented_aabb_safe (const AABB& aabb, const Matrix4& transform)
{
	if (aabb_valid(aabb)) {
		return aabb_for_oriented_aabb(aabb, transform);
	}
	return aabb;
}

inline AABB aabb_infinite ()
{
	return AABB(Vector3(0, 0, 0), Vector3(c_aabb_max, c_aabb_max, c_aabb_max));
}

#endif
