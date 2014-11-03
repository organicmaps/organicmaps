///////////////////////////////////////////////////////////////////////////////////
/// OpenGL Mathematics (glm.g-truc.net)
///
/// Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
/// 
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
/// 
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
/// THE SOFTWARE.
///
/// @ref gtc_quaternion
/// @file glm/gtc/quaternion.hpp
/// @date 2009-05-21 / 2012-12-20
/// @author Christophe Riccio
///
/// @see core (dependence)
/// @see gtc_half_float (dependence)
/// @see gtc_constants (dependence)
///
/// @defgroup gtc_quaternion GLM_GTC_quaternion
/// @ingroup gtc
/// 
/// @brief Defines a templated quaternion type and several quaternion operations.
/// 
/// <glm/gtc/quaternion.hpp> need to be included to use these functionalities.
///////////////////////////////////////////////////////////////////////////////////

#ifndef GLM_GTC_quaternion
#define GLM_GTC_quaternion

// Dependency:
#include "../mat3x3.hpp"
#include "../mat4x4.hpp"
#include "../vec3.hpp"
#include "../vec4.hpp"
#include "../gtc/constants.hpp"

#if(defined(GLM_MESSAGES) && !defined(GLM_EXT_INCLUDED))
#	pragma message("GLM: GLM_GTC_quaternion extension included")
#endif

namespace glm{
namespace detail
{
	template <typename T, precision P>
	struct tquat
	{
		enum ctor{null};

                typedef T value_type;
		typedef tvec4<bool, P> bool_type;

	public:
		T x, y, z, w;

		GLM_FUNC_DECL GLM_CONSTEXPR length_t length() const;

		// Constructors
		GLM_FUNC_DECL tquat();
		template <typename U, precision Q>
		GLM_FUNC_DECL explicit tquat(
			tquat<U, Q> const & q);
		GLM_FUNC_DECL tquat(
			T const & s,
			tvec3<T, P> const & v);
		GLM_FUNC_DECL tquat(
			T const & w,
			T const & x,
			T const & y,
			T const & z);

		// Convertions

		/// Create a quaternion from two normalized axis
		/// 
		/// @param u A first normalized axis
		/// @param v A second normalized axis
		/// @see gtc_quaternion
		/// @see http://lolengine.net/blog/2013/09/18/beautiful-maths-quaternion-from-vectors
		GLM_FUNC_DECL explicit tquat(
			detail::tvec3<T, P> const & u,
			detail::tvec3<T, P> const & v);
		/// Build a quaternion from euler angles (pitch, yaw, roll), in radians.
		GLM_FUNC_DECL explicit tquat(
			tvec3<T, P> const & eulerAngles);
		GLM_FUNC_DECL explicit tquat(
			tmat3x3<T, P> const & m);
		GLM_FUNC_DECL explicit tquat(
			tmat4x4<T, P> const & m);

		// Accesses
		GLM_FUNC_DECL T & operator[](length_t i);
		GLM_FUNC_DECL T const & operator[](length_t i) const;

		// Operators
		GLM_FUNC_DECL tquat<T, P> & operator+=(tquat<T, P> const & q);
		GLM_FUNC_DECL tquat<T, P> & operator*=(tquat<T, P> const & q);
		GLM_FUNC_DECL tquat<T, P> & operator*=(T const & s);
		GLM_FUNC_DECL tquat<T, P> & operator/=(T const & s);
	};

	template <typename T, precision P>
	GLM_FUNC_DECL detail::tquat<T, P> operator- (
		detail::tquat<T, P> const & q);

	template <typename T, precision P>
	GLM_FUNC_DECL detail::tquat<T, P> operator+ (
		detail::tquat<T, P> const & q,
		detail::tquat<T, P> const & p);

	template <typename T, precision P>
	GLM_FUNC_DECL detail::tquat<T, P> operator* (
		detail::tquat<T, P> const & q,
		detail::tquat<T, P> const & p);

	template <typename T, precision P>
	GLM_FUNC_DECL detail::tvec3<T, P> operator* (
		detail::tquat<T, P> const & q,
		detail::tvec3<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL detail::tvec3<T, P> operator* (
		detail::tvec3<T, P> const & v,
		detail::tquat<T, P> const & q);

	template <typename T, precision P>
	GLM_FUNC_DECL detail::tvec4<T, P> operator* (
		detail::tquat<T, P> const & q, 
		detail::tvec4<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL detail::tvec4<T, P> operator* (
		detail::tvec4<T, P> const & v,
		detail::tquat<T, P> const & q);

	template <typename T, precision P>
	GLM_FUNC_DECL detail::tquat<T, P> operator* (
		detail::tquat<T, P> const & q,
		T const & s);

	template <typename T, precision P>
	GLM_FUNC_DECL detail::tquat<T, P> operator* (
		T const & s,
		detail::tquat<T, P> const & q);

	template <typename T, precision P>
	GLM_FUNC_DECL detail::tquat<T, P> operator/ (
		detail::tquat<T, P> const & q,
		T const & s);

} //namespace detail

	/// @addtogroup gtc_quaternion
	/// @{

	/// Returns the length of the quaternion.
	/// 
	/// @see gtc_quaternion
	template <typename T, precision P>
	GLM_FUNC_DECL T length(
		detail::tquat<T, P> const & q);

	/// Returns the normalized quaternion.
	/// 
	/// @see gtc_quaternion
	template <typename T, precision P>
	GLM_FUNC_DECL detail::tquat<T, P> normalize(
		detail::tquat<T, P> const & q);
		
	/// Returns dot product of q1 and q2, i.e., q1[0] * q2[0] + q1[1] * q2[1] + ...
	/// 
	/// @see gtc_quaternion
	template <typename T, precision P, template <typename, precision> class quatType>
	GLM_FUNC_DECL T dot(
		quatType<T, P> const & x,
		quatType<T, P> const & y);

	/// Spherical linear interpolation of two quaternions.
	/// The interpolation is oriented and the rotation is performed at constant speed.
	/// For short path spherical linear interpolation, use the slerp function.
	/// 
	/// @param x A quaternion
	/// @param y A quaternion
	/// @param a Interpolation factor. The interpolation is defined beyond the range [0, 1].
	/// @tparam T Value type used to build the quaternion. Supported: half, float or double.
	/// @see gtc_quaternion
	/// @see - slerp(detail::tquat<T, P> const & x, detail::tquat<T, P> const & y, T const & a)
	template <typename T, precision P>
	GLM_FUNC_DECL detail::tquat<T, P> mix(
		detail::tquat<T, P> const & x,
		detail::tquat<T, P> const & y,
		T const & a);

	/// Linear interpolation of two quaternions.
	/// The interpolation is oriented.
	/// 
	/// @param x A quaternion
	/// @param y A quaternion
	/// @param a Interpolation factor. The interpolation is defined in the range [0, 1].
	/// @tparam T Value type used to build the quaternion. Supported: half, float or double.
	/// @see gtc_quaternion
	template <typename T, precision P>
	GLM_FUNC_DECL detail::tquat<T, P> lerp(
		detail::tquat<T, P> const & x,
		detail::tquat<T, P> const & y,
		T const & a);

	/// Spherical linear interpolation of two quaternions.
	/// The interpolation always take the short path and the rotation is performed at constant speed.
	/// 
	/// @param x A quaternion
	/// @param y A quaternion
	/// @param a Interpolation factor. The interpolation is defined beyond the range [0, 1].
	/// @tparam T Value type used to build the quaternion. Supported: half, float or double.
	/// @see gtc_quaternion
	template <typename T, precision P>
	GLM_FUNC_DECL detail::tquat<T, P> slerp(
		detail::tquat<T, P> const & x,
		detail::tquat<T, P> const & y,
		T const & a);

	/// Returns the q conjugate.
	/// 
	/// @see gtc_quaternion
	template <typename T, precision P>
	GLM_FUNC_DECL detail::tquat<T, P> conjugate(
		detail::tquat<T, P> const & q);

	/// Returns the q inverse.
	/// 
	/// @see gtc_quaternion
	template <typename T, precision P>
	GLM_FUNC_DECL detail::tquat<T, P> inverse(
		detail::tquat<T, P> const & q);

	/// Rotates a quaternion from a vector of 3 components axis and an angle.
	/// 
	/// @param q Source orientation
	/// @param angle Angle expressed in radians if GLM_FORCE_RADIANS is define or degrees otherwise.
	/// @param axis Axis of the rotation
	/// 
	/// @see gtc_quaternion
	template <typename T, precision P>
	GLM_FUNC_DECL detail::tquat<T, P> rotate(
		detail::tquat<T, P> const & q,
		T const & angle,
		detail::tvec3<T, P> const & axis);

	/// Returns euler angles, yitch as x, yaw as y, roll as z.
	/// The result is expressed in radians if GLM_FORCE_RADIANS is defined or degrees otherwise.
	/// 
	/// @see gtc_quaternion
	template <typename T, precision P>
	GLM_FUNC_DECL detail::tvec3<T, P> eulerAngles(
		detail::tquat<T, P> const & x);

	/// Returns roll value of euler angles expressed in radians if GLM_FORCE_RADIANS is defined or degrees otherwise.
	///
	/// @see gtx_quaternion
	template <typename T, precision P>
	GLM_FUNC_DECL T roll(detail::tquat<T, P> const & x);

	/// Returns pitch value of euler angles expressed in radians if GLM_FORCE_RADIANS is defined or degrees otherwise.
	///
	/// @see gtx_quaternion
	template <typename T, precision P>
	GLM_FUNC_DECL T pitch(detail::tquat<T, P> const & x);

	/// Returns yaw value of euler angles expressed in radians if GLM_FORCE_RADIANS is defined or degrees otherwise.
	///
	/// @see gtx_quaternion
	template <typename T, precision P>
	GLM_FUNC_DECL T yaw(detail::tquat<T, P> const & x);

	/// Converts a quaternion to a 3 * 3 matrix.
	/// 
	/// @see gtc_quaternion
	template <typename T, precision P>
	GLM_FUNC_DECL detail::tmat3x3<T, P> mat3_cast(
		detail::tquat<T, P> const & x);

	/// Converts a quaternion to a 4 * 4 matrix.
	/// 
	/// @see gtc_quaternion
	template <typename T, precision P>
	GLM_FUNC_DECL detail::tmat4x4<T, P> mat4_cast(
		detail::tquat<T, P> const & x);

	/// Converts a 3 * 3 matrix to a quaternion.
	/// 
	/// @see gtc_quaternion
	template <typename T, precision P>
	GLM_FUNC_DECL detail::tquat<T, P> quat_cast(
		detail::tmat3x3<T, P> const & x);

	/// Converts a 4 * 4 matrix to a quaternion.
	/// 
	/// @see gtc_quaternion
	template <typename T, precision P>
	GLM_FUNC_DECL detail::tquat<T, P> quat_cast(
		detail::tmat4x4<T, P> const & x);

	/// Returns the quaternion rotation angle.
	///
	/// @see gtc_quaternion
	template <typename T, precision P>
	GLM_FUNC_DECL T angle(detail::tquat<T, P> const & x);

	/// Returns the q rotation axis.
	///
	/// @see gtc_quaternion
	template <typename T, precision P>
	GLM_FUNC_DECL detail::tvec3<T, P> axis(
		detail::tquat<T, P> const & x);

	/// Build a quaternion from an angle and a normalized axis.
	///
	/// @param angle Angle expressed in radians if GLM_FORCE_RADIANS is define or degrees otherwise.
	/// @param axis Axis of the quaternion, must be normalized.
	///
	/// @see gtc_quaternion
	template <typename T, precision P>
	GLM_FUNC_DECL detail::tquat<T, P> angleAxis(
		T const & angle,
		detail::tvec3<T, P> const & axis);

	/// Returns the component-wise comparison result of x < y.
	/// 
	/// @tparam quatType Floating-point quaternion types.
	///
	/// @see gtc_quaternion
	template <typename T, precision P>
	GLM_FUNC_DECL detail::tvec4<bool, P> lessThan(
		detail::tquat<T, P> const & x, 
		detail::tquat<T, P> const & y);

	/// Returns the component-wise comparison of result x <= y.
	///
	/// @tparam quatType Floating-point quaternion types.
	///
	/// @see gtc_quaternion
	template <typename T, precision P>
	GLM_FUNC_DECL detail::tvec4<bool, P> lessThanEqual(
		detail::tquat<T, P> const & x, 
		detail::tquat<T, P> const & y);

	/// Returns the component-wise comparison of result x > y.
	///
	/// @tparam quatType Floating-point quaternion types.
	///
	/// @see gtc_quaternion
	template <typename T, precision P>
	GLM_FUNC_DECL detail::tvec4<bool, P> greaterThan(
		detail::tquat<T, P> const & x, 
		detail::tquat<T, P> const & y);

	/// Returns the component-wise comparison of result x >= y.
	///
	/// @tparam quatType Floating-point quaternion types.
	///
	/// @see gtc_quaternion
	template <typename T, precision P>
	GLM_FUNC_DECL detail::tvec4<bool, P> greaterThanEqual(
		detail::tquat<T, P> const & x, 
		detail::tquat<T, P> const & y);

	/// Returns the component-wise comparison of result x == y.
	///
	/// @tparam quatType Floating-point quaternion types.
	///
	/// @see gtc_quaternion
	template <typename T, precision P>
	GLM_FUNC_DECL detail::tvec4<bool, P> equal(
		detail::tquat<T, P> const & x, 
		detail::tquat<T, P> const & y);

	/// Returns the component-wise comparison of result x != y.
	/// 
	/// @tparam quatType Floating-point quaternion types.
	///
	/// @see gtc_quaternion
	template <typename T, precision P>
	GLM_FUNC_DECL detail::tvec4<bool, P> notEqual(
		detail::tquat<T, P> const & x, 
		detail::tquat<T, P> const & y);

	/// @}
} //namespace glm

#include "quaternion.inl"

#endif//GLM_GTC_quaternion
