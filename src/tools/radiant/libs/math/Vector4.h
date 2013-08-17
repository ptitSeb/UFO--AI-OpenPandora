#ifndef VECTOR4_H_
#define VECTOR4_H_

/* greebo: This file contains the templated class definition of the three-component vector
 *
 * BasicVector4: A vector with three components of type <Element>
 *
 * The BasicVector4 is equipped with the most important operators like *, *= and so on.
 *
 * Note: The most commonly used Vector4 is a BasicVector4<float>, this is also defined in this file
 *
 * Note: that the multiplication of a Vector4 with another one (Vector4*Vector4) does NOT
 * result in an inner product but in a component-wise scaling. Use the .dot() method to
 * execute an inner product of two vectors.
 */

#include "FloatTools.h"
#include "Vector3.h"

// A 4-element vector of type <Element>
template<typename Element>
class BasicVector4
{

		// The components of this vector
		Element m_elements[4];

	public:

		// Constructor (no arguments)
		BasicVector4 ()
		{
		}

		// Construct a BasicVector4 out of the 4 arguments
		BasicVector4 (Element x_, Element y_, Element z_, Element w_)
		{
			x() = x_;
			y() = y_;
			z() = z_;
			w() = w_;
		}

		// Construct a BasicVector4 out of a Vector3 plus a fourth argument
		BasicVector4 (const BasicVector3<Element>& self, Element w_)
		{
			x() = self.x();
			y() = self.y();
			z() = self.z();
			w() = w_;
		}

		/** Set all 4 components to the provided values.
		 */
		void set (const Element& x, const Element& y, const Element& z, const Element& a)
		{
			m_elements[0] = x;
			m_elements[1] = y;
			m_elements[2] = z;
			m_elements[3] = a;
		}

		// Return non-constant references to the components
		Element& x ()
		{
			return m_elements[0];
		}
		Element& y ()
		{
			return m_elements[1];
		}
		Element& z ()
		{
			return m_elements[2];
		}
		Element& w ()
		{
			return m_elements[3];
		}

		// Return constant references to the components
		const Element& x () const
		{
			return m_elements[0];
		}
		const Element& y () const
		{
			return m_elements[1];
		}
		const Element& z () const
		{
			return m_elements[2];
		}
		const Element& w () const
		{
			return m_elements[3];
		}

		Element index (std::size_t i) const
		{
			return m_elements[i];
		}
		Element& index (std::size_t i)
		{
			return m_elements[i];
		}

		/** Compare this BasicVector4 against another for equality.
		 */
		bool operator== (const BasicVector4& other) const
		{
			return (other.x() == x() && other.y() == y() && other.z() == z() && other.w() == w());
		}

		/** Compare this BasicVector4 against another for inequality.
		 */
		bool operator!= (const BasicVector4& other) const
		{
			return !(*this == other);
		}

		/*	Define the addition operators + and += with any other BasicVector4 of type OtherElement
		 *  The vectors are added to each other element-wise
		 */
		template<typename OtherElement>
		BasicVector4<Element> operator+ (const BasicVector4<OtherElement>& other) const
		{
			return BasicVector4<Element> (m_elements[0] + static_cast<Element> (other.x()), m_elements[1]
					+ static_cast<Element> (other.y()), m_elements[2] + static_cast<Element> (other.z()), m_elements[3]
					+ static_cast<Element> (other.w()));
		}

		template<typename OtherElement>
		void operator+= (const BasicVector4<OtherElement>& other)
		{
			m_elements[0] += static_cast<Element> (other.x());
			m_elements[1] += static_cast<Element> (other.y());
			m_elements[2] += static_cast<Element> (other.z());
			m_elements[3] += static_cast<Element> (other.w());
		}

		/*	Define the substraction operators - and -= with any other BasicVector4 of type OtherElement
		 *  The vectors are substracted from each other element-wise
		 */
		template<typename OtherElement>
		BasicVector4<Element> operator- (const BasicVector4<OtherElement>& other) const
		{
			return BasicVector4<Element> (m_elements[0] - static_cast<Element> (other.x()), m_elements[1]
					- static_cast<Element> (other.y()), m_elements[2] - static_cast<Element> (other.z()), m_elements[3]
					- static_cast<Element> (other.w()));
		}

		template<typename OtherElement>
		void operator-= (const BasicVector4<OtherElement>& other)
		{
			m_elements[0] -= static_cast<Element> (other.x());
			m_elements[1] -= static_cast<Element> (other.y());
			m_elements[2] -= static_cast<Element> (other.z());
			m_elements[3] -= static_cast<Element> (other.w());
		}

		/*	Define the multiplication operators * and *= with another Vector4 of type OtherElement
		 *
		 *  The vectors are multiplied element-wise
		 *
		 *  greebo: This is mathematically kind of senseless, as this is a mixture of
		 *  a dot product and scalar multiplication. It can be used to scale each
		 *  vector component by a different factor, so maybe this comes in handy.
		 */
		template<typename OtherElement>
		BasicVector4<Element> operator* (const BasicVector4<OtherElement>& other) const
		{
			return BasicVector4<Element> (m_elements[0] * static_cast<Element> (other.x()), m_elements[1]
					* static_cast<Element> (other.y()), m_elements[2] * static_cast<Element> (other.z()), m_elements[3]
					* static_cast<Element> (other.w()));
		}

		template<typename OtherElement>
		void operator*= (const BasicVector4<OtherElement>& other)
		{
			m_elements[0] *= static_cast<Element> (other.x());
			m_elements[1] *= static_cast<Element> (other.y());
			m_elements[2] *= static_cast<Element> (other.z());
			m_elements[3] *= static_cast<Element> (other.w());
		}

		/*	Define the multiplications * and *= with a scalar
		 */
		template<typename OtherElement>
		BasicVector4<Element> operator* (const OtherElement& other) const
		{
			Element factor = static_cast<Element> (other);
			return BasicVector4<Element> (m_elements[0] * factor, m_elements[1] * factor, m_elements[2] * factor,
					m_elements[3] * factor);
		}

		template<typename OtherElement>
		void operator*= (const OtherElement& other)
		{
			Element factor = static_cast<Element> (other);
			m_elements[0] *= factor;
			m_elements[1] *= factor;
			m_elements[2] *= factor;
			m_elements[3] *= factor;
		}

		/*	Define the division operators / and /= with another Vector4 of type OtherElement
		 *  The vectors are divided element-wise
		 */
		template<typename OtherElement>
		BasicVector4<Element> operator/ (const BasicVector4<OtherElement>& other) const
		{
			return BasicVector4<Element> (m_elements[0] / static_cast<Element> (other.x()), m_elements[1]
					/ static_cast<Element> (other.y()), m_elements[2] / static_cast<Element> (other.z()), m_elements[3]
					/ static_cast<Element> (other.w()));
		}

		template<typename OtherElement>
		void operator/= (const BasicVector4<OtherElement>& other)
		{
			m_elements[0] /= static_cast<Element> (other.x());
			m_elements[1] /= static_cast<Element> (other.y());
			m_elements[2] /= static_cast<Element> (other.z());
			m_elements[3] /= static_cast<Element> (other.w());
		}

		/*	Define the scalar divisions / and /=
		 */
		template<typename OtherElement>
		BasicVector4<Element> operator/ (const OtherElement& other) const
		{
			Element divisor = static_cast<Element> (other);
			return BasicVector4<Element> (m_elements[0] / divisor, m_elements[1] / divisor, m_elements[2] / divisor,
					m_elements[3] / divisor);
		}

		template<typename OtherElement>
		void operator/= (const OtherElement& other)
		{
			Element divisor = static_cast<Element> (other);
			m_elements[0] /= divisor;
			m_elements[1] /= divisor;
			m_elements[2] /= divisor;
			m_elements[3] /= divisor;
		}

		/* Scalar product this vector with another Vector4,
		 * returning the projection of <self> onto <other>
		 *
		 * @param other
		 * The Vector4 to dot-product with this Vector4.
		 *
		 * @returns
		 * The inner product (a scalar): a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + a[3]*b[3]
		 */

		template<typename OtherT>
		Element dot (const BasicVector4<OtherT>& other) const
		{
			return Element(m_elements[0] * other.x() + m_elements[1] * other.y() + m_elements[2] * other.z()
					+ m_elements[3] * other.w());
		}

		/** Project this homogeneous Vector4 into a Cartesian Vector3
		 * by dividing by w.
		 *
		 * @returns
		 * A Vector3 representing the Cartesian equivalent of this
		 * homogeneous vector.
		 */
		BasicVector3<Element> getProjected ()
		{
			return BasicVector3<Element> (m_elements[0] / m_elements[3], m_elements[1] / m_elements[3], m_elements[2]
					/ m_elements[3]);
		}

		/**
		 * @return String representation of the vector - values are separated by space
		 */
		std::string toString () const
		{
			std::stringstream ss;
			ss << m_elements[0] << " " << m_elements[1] << " " << m_elements[2] << " " << m_elements[3];
			return ss.str();
		}

		/** Cast to std::string
		 */
		operator std::string() const {
			return toString();
		}

		/** Implicit cast to C-style array. This allows a Vector4 to be
		 * passed directly to GL functions that expect an array (e.g.
		 * glFloat4fv()). These functions implicitly provide operator[]
		 * as well, since the C-style array provides this function.
		 */

		operator const Element* () const
		{
			return m_elements;
		}

		operator Element* ()
		{
			return m_elements;
		}

		/*	Cast this Vector4 onto a Vector3, both const and non-const
		 */
		BasicVector3<Element>& getVector3 ()
		{
			return *reinterpret_cast<BasicVector3<Element>*> (m_elements);
		}

		const BasicVector3<Element>& getVector3 () const
		{
			return *reinterpret_cast<const BasicVector3<Element>*> (m_elements);
		}

}; // BasicVector4

// ==========================================================================================

// A 4-element vector stored in single-precision floating-point.
typedef BasicVector4<float> Vector4;

// =============== Common Vector4 Methods ==================================================

template<typename Element, typename OtherElement>
inline bool vector4_equal_epsilon (const BasicVector4<Element>& self, const BasicVector4<OtherElement>& other,
		Element epsilon)
{
	return float_equal_epsilon(self.x(), other.x(), epsilon) && float_equal_epsilon(self.y(), other.y(), epsilon)
			&& float_equal_epsilon(self.z(), other.z(), epsilon) && float_equal_epsilon(self.w(), other.w(), epsilon);
}

#endif /*VECTOR4_H_*/
