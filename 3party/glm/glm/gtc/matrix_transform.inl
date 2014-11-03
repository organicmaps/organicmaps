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
/// @ref gtc_matrix_transform
/// @file glm/gtc/matrix_transform.inl
/// @date 2009-04-29 / 2011-06-15
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#include "../geometric.hpp"
#include "../trigonometric.hpp"
#include "../matrix.hpp"

namespace glm
{
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, P> translate
	(
		detail::tmat4x4<T, P> const & m,
		detail::tvec3<T, P> const & v
	)
	{
		detail::tmat4x4<T, P> Result(m);
		Result[3] = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3];
		return Result;
	}
	
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, P> rotate
	(
		detail::tmat4x4<T, P> const & m,
		T const & angle,
		detail::tvec3<T, P> const & v
	)
	{
#ifdef GLM_FORCE_RADIANS
		T a = angle;
#else
#		pragma message("GLM: rotate function taking degrees as a parameter is deprecated. #define GLM_FORCE_RADIANS before including GLM headers to remove this message.")
		T a = radians(angle);
#endif
		T c = cos(a);
		T s = sin(a);

		detail::tvec3<T, P> axis(normalize(v));
		detail::tvec3<T, P> temp((T(1) - c) * axis);

		detail::tmat4x4<T, P> Rotate(detail::tmat4x4<T, P>::_null);
		Rotate[0][0] = c + temp[0] * axis[0];
		Rotate[0][1] = 0 + temp[0] * axis[1] + s * axis[2];
		Rotate[0][2] = 0 + temp[0] * axis[2] - s * axis[1];

		Rotate[1][0] = 0 + temp[1] * axis[0] - s * axis[2];
		Rotate[1][1] = c + temp[1] * axis[1];
		Rotate[1][2] = 0 + temp[1] * axis[2] + s * axis[0];

		Rotate[2][0] = 0 + temp[2] * axis[0] + s * axis[1];
		Rotate[2][1] = 0 + temp[2] * axis[1] - s * axis[0];
		Rotate[2][2] = c + temp[2] * axis[2];

		detail::tmat4x4<T, P> Result(detail::tmat4x4<T, P>::_null);
		Result[0] = m[0] * Rotate[0][0] + m[1] * Rotate[0][1] + m[2] * Rotate[0][2];
		Result[1] = m[0] * Rotate[1][0] + m[1] * Rotate[1][1] + m[2] * Rotate[1][2];
		Result[2] = m[0] * Rotate[2][0] + m[1] * Rotate[2][1] + m[2] * Rotate[2][2];
		Result[3] = m[3];
		return Result;
	}
		
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, P> rotate_slow
	(
		detail::tmat4x4<T, P> const & m,
		T const & angle, 
		detail::tvec3<T, P> const & v
	)
	{
#ifdef GLM_FORCE_RADIANS
		T const a = angle;
#else
#		pragma message("GLM: rotate_slow function taking degrees as a parameter is deprecated. #define GLM_FORCE_RADIANS before including GLM headers to remove this message.")
		T const a = radians(angle);
#endif
		T c = cos(a);
		T s = sin(a);
		detail::tmat4x4<T, P> Result;

		detail::tvec3<T, P> axis = normalize(v);

		Result[0][0] = c + (1 - c)      * axis.x     * axis.x;
		Result[0][1] = (1 - c) * axis.x * axis.y + s * axis.z;
		Result[0][2] = (1 - c) * axis.x * axis.z - s * axis.y;
		Result[0][3] = 0;

		Result[1][0] = (1 - c) * axis.y * axis.x - s * axis.z;
		Result[1][1] = c + (1 - c) * axis.y * axis.y;
		Result[1][2] = (1 - c) * axis.y * axis.z + s * axis.x;
		Result[1][3] = 0;

		Result[2][0] = (1 - c) * axis.z * axis.x + s * axis.y;
		Result[2][1] = (1 - c) * axis.z * axis.y - s * axis.x;
		Result[2][2] = c + (1 - c) * axis.z * axis.z;
		Result[2][3] = 0;

		Result[3] = detail::tvec4<T, P>(0, 0, 0, 1);
		return m * Result;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, P> scale
		(
		detail::tmat4x4<T, P> const & m,
		detail::tvec3<T, P> const & v
		)
	{
		detail::tmat4x4<T, P> Result(detail::tmat4x4<T, P>::_null);
		Result[0] = m[0] * v[0];
		Result[1] = m[1] * v[1];
		Result[2] = m[2] * v[2];
		Result[3] = m[3];
		return Result;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, P> scale_slow
	(
		detail::tmat4x4<T, P> const & m,
		detail::tvec3<T, P> const & v
	)
	{
		detail::tmat4x4<T, P> Result(T(1));
		Result[0][0] = v.x;
		Result[1][1] = v.y;
		Result[2][2] = v.z;
		return m * Result;
	}

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, defaultp> ortho
	(
		T const & left,
		T const & right,
		T const & bottom,
		T const & top,
		T const & zNear,
		T const & zFar
	)
	{
		detail::tmat4x4<T, defaultp> Result(1);
		Result[0][0] = static_cast<T>(2) / (right - left);
		Result[1][1] = static_cast<T>(2) / (top - bottom);
		Result[2][2] = - T(2) / (zFar - zNear);
		Result[3][0] = - (right + left) / (right - left);
		Result[3][1] = - (top + bottom) / (top - bottom);
		Result[3][2] = - (zFar + zNear) / (zFar - zNear);
		return Result;
	}

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, defaultp> ortho
	(
		T const & left,
		T const & right,
		T const & bottom,
		T const & top
	)
	{
		detail::tmat4x4<T, defaultp> Result(1);
		Result[0][0] = static_cast<T>(2) / (right - left);
		Result[1][1] = static_cast<T>(2) / (top - bottom);
		Result[2][2] = - T(1);
		Result[3][0] = - (right + left) / (right - left);
		Result[3][1] = - (top + bottom) / (top - bottom);
		return Result;
	}

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, defaultp> frustum
	(
		T const & left,
		T const & right,
		T const & bottom,
		T const & top,
		T const & nearVal,
		T const & farVal
	)
	{
		detail::tmat4x4<T, defaultp> Result(0);
		Result[0][0] = (static_cast<T>(2) * nearVal) / (right - left);
		Result[1][1] = (static_cast<T>(2) * nearVal) / (top - bottom);
		Result[2][0] = (right + left) / (right - left);
		Result[2][1] = (top + bottom) / (top - bottom);
		Result[2][2] = -(farVal + nearVal) / (farVal - nearVal);
		Result[2][3] = static_cast<T>(-1);
		Result[3][2] = -(static_cast<T>(2) * farVal * nearVal) / (farVal - nearVal);
		return Result;
	}

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, defaultp> perspective
	(
		T const & fovy,
		T const & aspect,
		T const & zNear,
		T const & zFar
	)
	{
		assert(aspect != static_cast<T>(0));
		assert(zFar != zNear);

#ifdef GLM_FORCE_RADIANS
		T const rad = fovy;
#else
#		pragma message("GLM: perspective function taking degrees as a parameter is deprecated. #define GLM_FORCE_RADIANS before including GLM headers to remove this message.")
		T const rad = glm::radians(fovy);
#endif

		T tanHalfFovy = tan(rad / static_cast<T>(2));

		detail::tmat4x4<T, defaultp> Result(static_cast<T>(0));
		Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
		Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
		Result[2][2] = - (zFar + zNear) / (zFar - zNear);
		Result[2][3] = - static_cast<T>(1);
		Result[3][2] = - (static_cast<T>(2) * zFar * zNear) / (zFar - zNear);
		return Result;
	}
	
	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, defaultp> perspectiveFov
	(
		T const & fov,
		T const & width,
		T const & height,
		T const & zNear,
		T const & zFar
	)
	{
		assert(width > static_cast<T>(0));
		assert(height > static_cast<T>(0));
		assert(fov > static_cast<T>(0));
	
#ifdef GLM_FORCE_RADIANS
		T rad = fov;
#else
#		pragma message("GLM: perspectiveFov function taking degrees as a parameter is deprecated. #define GLM_FORCE_RADIANS before including GLM headers to remove this message.")
		T rad = glm::radians(fov);
#endif
		T h = glm::cos(static_cast<T>(0.5) * rad) / glm::sin(static_cast<T>(0.5) * rad);
		T w = h * height / width; ///todo max(width , Height) / min(width , Height)?

		detail::tmat4x4<T, defaultp> Result(static_cast<T>(0));
		Result[0][0] = w;
		Result[1][1] = h;
		Result[2][2] = - (zFar + zNear) / (zFar - zNear);
		Result[2][3] = - static_cast<T>(1);
		Result[3][2] = - (static_cast<T>(2) * zFar * zNear) / (zFar - zNear);
		return Result;
	}

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, defaultp> infinitePerspective
	(
		T fovy,
		T aspect,
		T zNear
	)
	{
#ifdef GLM_FORCE_RADIANS
		T const range = tan(fovy / T(2)) * zNear;	
#else
#		pragma message("GLM: infinitePerspective function taking degrees as a parameter is deprecated. #define GLM_FORCE_RADIANS before including GLM headers to remove this message.")
		T const range = tan(radians(fovy / T(2))) * zNear;	
#endif
		T left = -range * aspect;
		T right = range * aspect;
		T bottom = -range;
		T top = range;

		detail::tmat4x4<T, defaultp> Result(T(0));
		Result[0][0] = (T(2) * zNear) / (right - left);
		Result[1][1] = (T(2) * zNear) / (top - bottom);
		Result[2][2] = - T(1);
		Result[2][3] = - T(1);
		Result[3][2] = - T(2) * zNear;
		return Result;
	}

	// Infinite projection matrix: http://www.terathon.com/gdc07_lengyel.pdf
	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, defaultp> tweakedInfinitePerspective
	(
		T fovy,
		T aspect,
		T zNear,
		T ep
	)
	{
#ifdef GLM_FORCE_RADIANS
		T range = tan(fovy / T(2)) * zNear;	
#else
#		pragma message("GLM: tweakedInfinitePerspective function taking degrees as a parameter is deprecated. #define GLM_FORCE_RADIANS before including GLM headers to remove this message.")
		T range = tan(radians(fovy / T(2))) * zNear;	
#endif
		T left = -range * aspect;
		T right = range * aspect;
		T bottom = -range;
		T top = range;

		detail::tmat4x4<T, defaultp> Result(T(0));
		Result[0][0] = (static_cast<T>(2) * zNear) / (right - left);
		Result[1][1] = (static_cast<T>(2) * zNear) / (top - bottom);
		Result[2][2] = ep - static_cast<T>(1);
		Result[2][3] = static_cast<T>(-1);
		Result[3][2] = (ep - static_cast<T>(2)) * zNear;
		return Result;
	}

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, defaultp> tweakedInfinitePerspective
	(
		T fovy,
		T aspect,
		T zNear
	)
	{
		return tweakedInfinitePerspective(fovy, aspect, zNear, epsilon<T>());
	}

	template <typename T, typename U, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> project
	(
		detail::tvec3<T, P> const & obj,
		detail::tmat4x4<T, P> const & model,
		detail::tmat4x4<T, P> const & proj,
		detail::tvec4<U, P> const & viewport
	)
	{
		detail::tvec4<T, P> tmp = detail::tvec4<T, P>(obj, T(1));
		tmp = model * tmp;
		tmp = proj * tmp;

		tmp /= tmp.w;
		tmp = tmp * T(0.5) + T(0.5);
		tmp[0] = tmp[0] * T(viewport[2]) + T(viewport[0]);
		tmp[1] = tmp[1] * T(viewport[3]) + T(viewport[1]);

		return detail::tvec3<T, P>(tmp);
	}

	template <typename T, typename U, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> unProject
	(
		detail::tvec3<T, P> const & win,
		detail::tmat4x4<T, P> const & model,
		detail::tmat4x4<T, P> const & proj,
		detail::tvec4<U, P> const & viewport
	)
	{
		detail::tmat4x4<T, P> Inverse = inverse(proj * model);

		detail::tvec4<T, P> tmp = detail::tvec4<T, P>(win, T(1));
		tmp.x = (tmp.x - T(viewport[0])) / T(viewport[2]);
		tmp.y = (tmp.y - T(viewport[1])) / T(viewport[3]);
		tmp = tmp * T(2) - T(1);

		detail::tvec4<T, P> obj = Inverse * tmp;
		obj /= obj.w;

		return detail::tvec3<T, P>(obj);
	}

	template <typename T, precision P, typename U>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, P> pickMatrix
	(
		detail::tvec2<T, P> const & center,
		detail::tvec2<T, P> const & delta,
		detail::tvec4<U, P> const & viewport
	)
	{
		assert(delta.x > T(0) && delta.y > T(0));
		detail::tmat4x4<T, P> Result(1.0f);

		if(!(delta.x > T(0) && delta.y > T(0)))
			return Result; // Error

		detail::tvec3<T, P> Temp(
			(T(viewport[2]) - T(2) * (center.x - T(viewport[0]))) / delta.x,
			(T(viewport[3]) - T(2) * (center.y - T(viewport[1]))) / delta.y,
			T(0));

		// Translate and scale the picked region to the entire window
		Result = translate(Result, Temp);
		return scale(Result, detail::tvec3<T, P>(T(viewport[2]) / delta.x, T(viewport[3]) / delta.y, T(1)));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, P> lookAt
	(
		detail::tvec3<T, P> const & eye,
		detail::tvec3<T, P> const & center,
		detail::tvec3<T, P> const & up
	)
	{
		detail::tvec3<T, P> f(normalize(center - eye));
		detail::tvec3<T, P> s(normalize(cross(f, up)));
		detail::tvec3<T, P> u(cross(s, f));

		detail::tmat4x4<T, P> Result(1);
		Result[0][0] = s.x;
		Result[1][0] = s.y;
		Result[2][0] = s.z;
		Result[0][1] = u.x;
		Result[1][1] = u.y;
		Result[2][1] = u.z;
		Result[0][2] =-f.x;
		Result[1][2] =-f.y;
		Result[2][2] =-f.z;
		Result[3][0] =-dot(s, eye);
		Result[3][1] =-dot(u, eye);
		Result[3][2] = dot(f, eye);
		return Result;
	}
}//namespace glm
