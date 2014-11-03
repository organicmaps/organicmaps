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
/// @ref core
/// @file glm/core/func_geometric.inl
/// @date 2008-08-03 / 2011-06-15
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#include "func_exponential.hpp"
#include "func_common.hpp"
#include "type_vec2.hpp"
#include "type_vec4.hpp"
#include "type_float.hpp"

namespace glm{
namespace detail
{
	template <template <class, precision> class vecType, typename T, precision P>
	struct compute_dot{};

	template <typename T, precision P>
	struct compute_dot<detail::tvec1, T, P>
	{
		GLM_FUNC_QUALIFIER static T call(detail::tvec1<T, P> const & x, detail::tvec1<T, P> const & y)
		{
#			ifdef __CUDACC__ // Wordaround for a CUDA compiler bug up to CUDA6
				detail::tvec1<T, P> tmp(x * y);
				return tmp.x;
#			else
				return detail::tvec1<T, P>(x * y).x;
#			endif
		}
	};

	template <typename T, precision P>
	struct compute_dot<detail::tvec2, T, P>
	{
		GLM_FUNC_QUALIFIER static T call(detail::tvec2<T, P> const & x, detail::tvec2<T, P> const & y)
		{
			detail::tvec2<T, P> tmp(x * y);
			return tmp.x + tmp.y;
		}
	};

	template <typename T, precision P>
	struct compute_dot<detail::tvec3, T, P>
	{
		GLM_FUNC_QUALIFIER static T call(detail::tvec3<T, P> const & x, detail::tvec3<T, P> const & y)
		{
			detail::tvec3<T, P> tmp(x * y);
			return tmp.x + tmp.y + tmp.z;
		}
	};

	template <typename T, precision P>
	struct compute_dot<detail::tvec4, T, P>
	{
		GLM_FUNC_QUALIFIER static T call(detail::tvec4<T, P> const & x, detail::tvec4<T, P> const & y)
		{
			detail::tvec4<T, P> tmp(x * y);
			return (tmp.x + tmp.y) + (tmp.z + tmp.w);
		}
	};
}//namespace detail

	// length
	template <typename genType>
	GLM_FUNC_QUALIFIER genType length
	(
		genType const & x
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<genType>::is_iec559, "'length' only accept floating-point inputs");

		return abs(x);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER T length(detail::tvec2<T, P> const & v)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'length' only accept floating-point inputs");

		T sqr = v.x * v.x + v.y * v.y;
		return sqrt(sqr);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER T length(detail::tvec3<T, P> const & v)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'length' only accept floating-point inputs");

		T sqr = v.x * v.x + v.y * v.y + v.z * v.z;
		return sqrt(sqr);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER T length(detail::tvec4<T, P> const & v)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'length' only accept floating-point inputs");

		T sqr = v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
		return sqrt(sqr);
	}

	// distance
	template <typename genType>
	GLM_FUNC_QUALIFIER genType distance
	(
		genType const & p0,
		genType const & p1
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<genType>::is_iec559, "'distance' only accept floating-point inputs");

		return length(p1 - p0);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER T distance
	(
		detail::tvec2<T, P> const & p0,
		detail::tvec2<T, P> const & p1
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'distance' only accept floating-point inputs");

		return length(p1 - p0);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER T distance
	(
		detail::tvec3<T, P> const & p0,
		detail::tvec3<T, P> const & p1
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'distance' only accept floating-point inputs");

		return length(p1 - p0);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER T distance
	(
		detail::tvec4<T, P> const & p0,
		detail::tvec4<T, P> const & p1
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'distance' only accept floating-point inputs");

		return length(p1 - p0);
	}

	// dot
	template <typename T>
	GLM_FUNC_QUALIFIER T dot
	(
		T const & x,
		T const & y
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'dot' only accept floating-point inputs");
		return detail::compute_dot<detail::tvec1, T, highp>::call(x, y);
	}

	template <typename T, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER T dot
	(
		vecType<T, P> const & x,
		vecType<T, P> const & y
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'dot' only accept floating-point inputs");
		return detail::compute_dot<vecType, T, P>::call(x, y);
	}

/* // SSE3
	GLM_FUNC_QUALIFIER float dot(const tvec4<float>& x, const tvec4<float>& y)
	{
		float Result;
		__asm
		{
			mov		esi, x
			mov		edi, y
			movaps	xmm0, [esi]
			mulps	xmm0, [edi]
			haddps(	_xmm0, _xmm0 )
			haddps(	_xmm0, _xmm0 )
			movss	Result, xmm0
		}
		return Result;
	}
*/
	// cross
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> cross
	(
		detail::tvec3<T, P> const & x,
		detail::tvec3<T, P> const & y
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'cross' only accept floating-point inputs");

		return detail::tvec3<T, P>(
			x.y * y.z - y.y * x.z,
			x.z * y.x - y.z * x.x,
			x.x * y.y - y.x * x.y);
	}

	// normalize
	template <typename genType>
	GLM_FUNC_QUALIFIER genType normalize
	(
		genType const & x
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<genType>::is_iec559, "'normalize' only accept floating-point inputs");

		return x < genType(0) ? genType(-1) : genType(1);
	}

	// According to issue 10 GLSL 1.10 specification, if length(x) == 0 then result is undefine and generate an error
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec2<T, P> normalize
	(
		detail::tvec2<T, P> const & x
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'normalize' only accept floating-point inputs");
		
		T sqr = x.x * x.x + x.y * x.y;
		return x * inversesqrt(sqr);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> normalize
	(
		detail::tvec3<T, P> const & x
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'normalize' only accept floating-point inputs");

		T sqr = x.x * x.x + x.y * x.y + x.z * x.z;
		return x * inversesqrt(sqr);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec4<T, P> normalize
	(
		detail::tvec4<T, P> const & x
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'normalize' only accept floating-point inputs");
		
		T sqr = x.x * x.x + x.y * x.y + x.z * x.z + x.w * x.w;
		return x * inversesqrt(sqr);
	}

	// faceforward
	template <typename genType>
	GLM_FUNC_QUALIFIER genType faceforward
	(
		genType const & N,
		genType const & I,
		genType const & Nref
	)
	{
		return dot(Nref, I) < 0 ? N : -N;
	}

	// reflect
	template <typename genType>
	GLM_FUNC_QUALIFIER genType reflect
	(
		genType const & I,
		genType const & N
	)
	{
		return I - N * dot(N, I) * genType(2);
	}

	// refract
	template <typename genType>
	GLM_FUNC_QUALIFIER genType refract
	(
		genType const & I,
		genType const & N,
		genType const & eta
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<genType>::is_iec559, "'refract' only accept floating-point inputs");

		genType dotValue = dot(N, I);
		genType k = genType(1) - eta * eta * (genType(1) - dotValue * dotValue);
		if(k < genType(0))
			return genType(0);
		else
			return eta * I - (eta * dotValue + sqrt(k)) * N;
	}

	template <typename T, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<T, P> refract
	(
		vecType<T, P> const & I,
		vecType<T, P> const & N,
		T const & eta
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'refract' only accept floating-point inputs");

		T dotValue = dot(N, I);
		T k = T(1) - eta * eta * (T(1) - dotValue * dotValue);
		if(k < T(0))
			return vecType<T, P>(0);
		else
			return eta * I - (eta * dotValue + std::sqrt(k)) * N;
	}

}//namespace glm
