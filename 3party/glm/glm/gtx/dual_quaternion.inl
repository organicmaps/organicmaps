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
/// @ref gtx_dual_quaternion
/// @file glm/gtx/dual_quaternion.inl
/// @date 2013-02-10 / 2013-02-13
/// @author Maksim Vorobiev (msomeone@gmail.com)
///////////////////////////////////////////////////////////////////////////////////

#include "../geometric.hpp"
#include <limits>

namespace glm{
namespace detail
{
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR int tdualquat<T, P>::length() const
	{
		return 8;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tdualquat<T, P>::tdualquat() :
		real(tquat<T, P>()),
		dual(tquat<T, P>(T(0), T(0), T(0), T(0)))
	{}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tdualquat<T, P>::tdualquat
	(
		tquat<T, P> const & r
	) :
		real(r),
		dual(tquat<T, P>(T(0), T(0), T(0), T(0)))
	{}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tdualquat<T, P>::tdualquat
	(
		tquat<T, P> const & r,
		tquat<T, P> const & d
	) :
		real(r),
		dual(d)
	{}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tdualquat<T, P>::tdualquat
	(
		tquat<T, P> const & q,
		tvec3<T, P> const& p
	) :
		real(q),
		dual(
			T(-0.5) * ( p.x*q.x + p.y*q.y + p.z*q.z),
			T(+0.5) * ( p.x*q.w + p.y*q.z - p.z*q.y),
			T(+0.5) * (-p.x*q.z + p.y*q.w + p.z*q.x),
			T(+0.5) * ( p.x*q.y - p.y*q.x + p.z*q.w))
	{}

	//////////////////////////////////////////////////////////////
	// tdualquat conversions
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tdualquat<T, P>::tdualquat
	(
		tmat2x4<T, P> const & m
	)
	{
		*this = dualquat_cast(m);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tdualquat<T, P>::tdualquat
	(
		tmat3x4<T, P> const & m
	)
	{
		*this = dualquat_cast(m);
	}

	//////////////////////////////////////////////////////////////
	// tdualquat<T, P> accesses

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER typename tdualquat<T, P>::part_type & tdualquat<T, P>::operator [] (int i)
	{
		assert(i >= 0 && i < this->length());
		return (&real)[i];
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER typename tdualquat<T, P>::part_type const & tdualquat<T, P>::operator [] (int i) const
	{
		assert(i >= 0 && i < this->length());
		return (&real)[i];
	}

	//////////////////////////////////////////////////////////////
	// tdualquat<valType> operators

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tdualquat<T, P> & tdualquat<T, P>::operator *=
	(
		T const & s
	)
	{
		this->real *= s;
		this->dual *= s;
		return *this;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tdualquat<T, P> & tdualquat<T, P>::operator /=
	(
		T const & s
	)
	{
		this->real /= s;
		this->dual /= s;
		return *this;
	}

	//////////////////////////////////////////////////////////////
	// tquat<valType> external operators

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tdualquat<T, P> operator-
	(
		detail::tdualquat<T, P> const & q
	)
	{
		return detail::tdualquat<T, P>(-q.real,-q.dual);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tdualquat<T, P> operator+
	(
		detail::tdualquat<T, P> const & q,
		detail::tdualquat<T, P> const & p
	)
	{
		return detail::tdualquat<T, P>(q.real + p.real,q.dual + p.dual);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tdualquat<T, P> operator*
	(
		detail::tdualquat<T, P> const & p,
		detail::tdualquat<T, P> const & o
	)
	{
		return detail::tdualquat<T, P>(p.real * o.real,p.real * o.dual + p.dual * o.real);
	}

	// Transformation
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> operator*
	(
		detail::tdualquat<T, P> const & q,
		detail::tvec3<T, P> const & v
	)
	{
		detail::tvec3<T, P> const real_v3(q.real.x,q.real.y,q.real.z);
		detail::tvec3<T, P> const dual_v3(q.dual.x,q.dual.y,q.dual.z);
		return (cross(real_v3, cross(real_v3,v) + v * q.real.w + dual_v3) + dual_v3 * q.real.w - real_v3 * q.dual.w) * T(2) + v;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> operator*
	(
		detail::tvec3<T, P> const & v,
		detail::tdualquat<T, P> const & q
	)
	{
		return glm::inverse(q) * v;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec4<T, P> operator*
	(
		detail::tdualquat<T, P> const & q,
		detail::tvec4<T, P> const & v
	)
	{
		return detail::tvec4<T, P>(q * detail::tvec3<T, P>(v), v.w);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec4<T, P> operator*
	(
		detail::tvec4<T, P> const & v,
		detail::tdualquat<T, P> const & q
	)
	{
		return glm::inverse(q) * v;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tdualquat<T, P> operator*
	(
		detail::tdualquat<T, P> const & q,
		T const & s
	)
	{
		return detail::tdualquat<T, P>(q.real * s, q.dual * s);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tdualquat<T, P> operator*
	(
		T const & s,
		detail::tdualquat<T, P> const & q
	)
	{
		return q * s;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tdualquat<T, P> operator/
	(
		detail::tdualquat<T, P> const & q,
		T const & s
	)
	{
		return detail::tdualquat<T, P>(q.real / s, q.dual / s);
	}

	//////////////////////////////////////
	// Boolean operators
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER bool operator==
	(
		detail::tdualquat<T, P> const & q1,
		detail::tdualquat<T, P> const & q2
	)
	{
		return (q1.real == q2.real) && (q1.dual == q2.dual);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER bool operator!=
	(
		detail::tdualquat<T, P> const & q1,
		detail::tdualquat<T, P> const & q2
	)
	{
		return (q1.real != q2.dual) || (q1.real != q2.dual);
	}
	}//namespace detail

	////////////////////////////////////////////////////////
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tdualquat<T, P> normalize
	(
		detail::tdualquat<T, P> const & q
	)
	{
		return q / length(q.real);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tdualquat<T, P> lerp
	(
		detail::tdualquat<T, P> const & x,
		detail::tdualquat<T, P> const & y,
		T const & a
	)
	{
		// Dual Quaternion Linear blend aka DLB:
		// Lerp is only defined in [0, 1]
		assert(a >= static_cast<T>(0));
		assert(a <= static_cast<T>(1));
		T const k = dot(x.real,y.real) < static_cast<T>(0) ? -a : a;
		T const one(1);
		return detail::tdualquat<T, P>(x * (one - a) + y * k);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tdualquat<T, P> inverse
	(
		detail::tdualquat<T, P> const & q
	)
	{
		const glm::detail::tquat<T, P> real = conjugate(q.real);
		const glm::detail::tquat<T, P> dual = conjugate(q.dual);
		return detail::tdualquat<T, P>(real, dual + (real * (-2.0f * dot(real,dual))));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tmat2x4<T, P> mat2x4_cast
	(
		detail::tdualquat<T, P> const & x
	)
	{
		return detail::tmat2x4<T, P>( x[0].x, x[0].y, x[0].z, x[0].w, x[1].x, x[1].y, x[1].z, x[1].w );
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tmat3x4<T, P> mat3x4_cast
	(
		detail::tdualquat<T, P> const & x
	)
	{
		detail::tquat<T, P> r = x.real / length2(x.real);
		
		detail::tquat<T, P> const rr(r.w * x.real.w, r.x * x.real.x, r.y * x.real.y, r.z * x.real.z);
		r *= static_cast<T>(2);
		
		T const xy = r.x * x.real.y;
		T const xz = r.x * x.real.z;
		T const yz = r.y * x.real.z;
		T const wx = r.w * x.real.x;
		T const wy = r.w * x.real.y;
		T const wz = r.w * x.real.z;
		
		detail::tvec4<T, P> const a(
			rr.w + rr.x - rr.y - rr.z,
			xy - wz,
			xz + wy,
			-(x.dual.w * r.x - x.dual.x * r.w + x.dual.y * r.z - x.dual.z * r.y));
		
		detail::tvec4<T, P> const b(
			xy + wz,
			rr.w + rr.y - rr.x - rr.z,
			yz - wx,
			-(x.dual.w * r.y - x.dual.x * r.z - x.dual.y * r.w + x.dual.z * r.x));
		
		detail::tvec4<T, P> const c(
			xz - wy,
			yz + wx,
			rr.w + rr.z - rr.x - rr.y,
			-(x.dual.w * r.z + x.dual.x * r.y - x.dual.y * r.x - x.dual.z * r.w));
		
		return detail::tmat3x4<T, P>(a, b, c);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tdualquat<T, P> dualquat_cast
	(
		detail::tmat2x4<T, P> const & x
	)
	{
		return detail::tdualquat<T, P>(
			detail::tquat<T, P>( x[0].w, x[0].x, x[0].y, x[0].z ),
			detail::tquat<T, P>( x[1].w, x[1].x, x[1].y, x[1].z ));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tdualquat<T, P> dualquat_cast
	(
		detail::tmat3x4<T, P> const & x
	)
	{
		detail::tquat<T, P> real;
		
		T const trace = x[0].x + x[1].y + x[2].z;
		if(trace > T(0))
		{
			T const r = sqrt(T(1) + trace);
			T const invr = static_cast<T>(0.5) / r;
			real.w = static_cast<T>(0.5) * r;
			real.x = (x[2].y - x[1].z) * invr;
			real.y = (x[0].z - x[2].x) * invr;
			real.z = (x[1].x - x[0].y) * invr;
		}
		else if(x[0].x > x[1].y && x[0].x > x[2].z)
		{
			T const r = sqrt(T(1) + x[0].x - x[1].y - x[2].z);
			T const invr = static_cast<T>(0.5) / r;
			real.x = static_cast<T>(0.5)*r;
			real.y = (x[1].x + x[0].y) * invr;
			real.z = (x[0].z + x[2].x) * invr;
			real.w = (x[2].y - x[1].z) * invr;
		}
		else if(x[1].y > x[2].z)
		{
			T const r = sqrt(T(1) + x[1].y - x[0].x - x[2].z);
			T const invr = static_cast<T>(0.5) / r;
			real.x = (x[1].x + x[0].y) * invr;
			real.y = static_cast<T>(0.5) * r;
			real.z = (x[2].y + x[1].z) * invr;
			real.w = (x[0].z - x[2].x) * invr;
		}
		else
		{
			T const r = sqrt(T(1) + x[2].z - x[0].x - x[1].y);
			T const invr = static_cast<T>(0.5) / r;
			real.x = (x[0].z + x[2].x) * invr;
			real.y = (x[2].y + x[1].z) * invr;
			real.z = static_cast<T>(0.5) * r;
			real.w = (x[1].x - x[0].y) * invr;
		}
		
		detail::tquat<T, P> dual;
		dual.x =  T(0.5) * ( x[0].w * real.w + x[1].w * real.z - x[2].w * real.y);
		dual.y =  T(0.5) * (-x[0].w * real.z + x[1].w * real.w + x[2].w * real.x);
		dual.z =  T(0.5) * ( x[0].w * real.y - x[1].w * real.x + x[2].w * real.w);
		dual.w = -T(0.5) * ( x[0].w * real.x + x[1].w * real.y + x[2].w * real.z);
		return detail::tdualquat<T, P>(real, dual);
	}

}//namespace glm
