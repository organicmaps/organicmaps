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
/// @file glm/core/func_exponential.inl
/// @date 2008-08-03 / 2011-06-15
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#include "func_vector_relational.hpp"
#include "_vectorize.hpp"
#include <limits>
#include <cassert>

namespace glm{
namespace detail
{
	template <bool isFloat>
	struct compute_log2
	{
		template <typename T>
		T operator() (T const & Value) const;
	};

	template <>
	struct compute_log2<true>
	{
		template <typename T>
		GLM_FUNC_QUALIFIER T operator() (T const & Value) const
		{
			return static_cast<T>(::std::log(Value)) * static_cast<T>(1.4426950408889634073599246810019);
		}
	};

	template <template <class, precision> class vecType, typename T, precision P>
	struct compute_inversesqrt
	{
		GLM_FUNC_QUALIFIER static vecType<T, P> call(vecType<T, P> const & x)
		{
			return static_cast<T>(1) / sqrt(x);
		}
	};
		
	template <template <class, precision> class vecType>
	struct compute_inversesqrt<vecType, float, lowp>
	{
		GLM_FUNC_QUALIFIER static vecType<float, lowp> call(vecType<float, lowp> const & x)
		{
			vecType<float, lowp> tmp(x);
			vecType<float, lowp> xhalf(tmp * 0.5f);
			vecType<uint, lowp>* p = reinterpret_cast<vecType<uint, lowp>*>(const_cast<vecType<float, lowp>*>(&x));
			vecType<uint, lowp> i = vecType<uint, lowp>(0x5f375a86) - (*p >> vecType<uint, lowp>(1));
			vecType<float, lowp>* ptmp = reinterpret_cast<vecType<float, lowp>*>(&i);
			tmp = *ptmp;
			tmp = tmp * (1.5f - xhalf * tmp * tmp);
			return tmp;
		}
	};
}//namespace detail

	// pow
	template <typename genType>
	GLM_FUNC_QUALIFIER genType pow
	(
		genType const & x, 
		genType const & y
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<genType>::is_iec559,
			"'pow' only accept floating-point inputs");

		return std::pow(x, y);
	}

	VECTORIZE_VEC_VEC(pow)

	// exp
	template <typename genType>
	GLM_FUNC_QUALIFIER genType exp
	(
		genType const & x
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<genType>::is_iec559,
			"'exp' only accept floating-point inputs");

		return std::exp(x);
	}

	VECTORIZE_VEC(exp)

	// log
	template <typename genType>
	GLM_FUNC_QUALIFIER genType log
	(
		genType const & x
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<genType>::is_iec559,
			"'log' only accept floating-point inputs");

		return std::log(x);
	}

	VECTORIZE_VEC(log)

	//exp2, ln2 = 0.69314718055994530941723212145818f
	template <typename genType>
	GLM_FUNC_QUALIFIER genType exp2(genType const & x)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<genType>::is_iec559,
			"'exp2' only accept floating-point inputs");

		return std::exp(static_cast<genType>(0.69314718055994530941723212145818) * x);
	}

	VECTORIZE_VEC(exp2)

	// log2, ln2 = 0.69314718055994530941723212145818f
	template <typename genType>
	GLM_FUNC_QUALIFIER genType log2(genType x)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<genType>::is_iec559 || std::numeric_limits<genType>::is_integer,
			"GLM core 'log2' only accept floating-point inputs. Include <glm/gtx/integer.hpp> for additional integer support.");

		assert(x > genType(0)); // log2 is only defined on the range (0, inf]
		return detail::compute_log2<std::numeric_limits<genType>::is_iec559>()(x);
	}

	VECTORIZE_VEC(log2)

	namespace detail
	{
		template <template <class, precision> class vecType, typename T, precision P>
		struct compute_sqrt{};
		
		template <typename T, precision P>
		struct compute_sqrt<detail::tvec1, T, P>
		{
			GLM_FUNC_QUALIFIER static detail::tvec1<T, P> call(detail::tvec1<T, P> const & x)
			{
				return detail::tvec1<T, P>(std::sqrt(x.x));
			}
		};
		
		template <typename T, precision P>
		struct compute_sqrt<detail::tvec2, T, P>
		{
			GLM_FUNC_QUALIFIER static detail::tvec2<T, P> call(detail::tvec2<T, P> const & x)
			{
				return detail::tvec2<T, P>(std::sqrt(x.x), std::sqrt(x.y));
			}
		};
		
		template <typename T, precision P>
		struct compute_sqrt<detail::tvec3, T, P>
		{
			GLM_FUNC_QUALIFIER static detail::tvec3<T, P> call(detail::tvec3<T, P> const & x)
			{
				return detail::tvec3<T, P>(std::sqrt(x.x), std::sqrt(x.y), std::sqrt(x.z));
			}
		};
		
		template <typename T, precision P>
		struct compute_sqrt<detail::tvec4, T, P>
		{
			GLM_FUNC_QUALIFIER static detail::tvec4<T, P> call(detail::tvec4<T, P> const & x)
			{
				return detail::tvec4<T, P>(std::sqrt(x.x), std::sqrt(x.y), std::sqrt(x.z), std::sqrt(x.w));
			}
		};
	}//namespace detail
	
	// sqrt
	GLM_FUNC_QUALIFIER float sqrt(float x)
	{
#		ifdef __CUDACC__ // Wordaround for a CUDA compiler bug up to CUDA6
			detail::tvec1<float, highp> tmp(detail::compute_sqrt<detail::tvec1, float, highp>::call(x));
			return tmp.x;
#		else
			return detail::compute_sqrt<detail::tvec1, float, highp>::call(x).x;
#		endif
	}

	GLM_FUNC_QUALIFIER double sqrt(double x)
	{
#		ifdef __CUDACC__ // Wordaround for a CUDA compiler bug up to CUDA6
			detail::tvec1<double, highp> tmp(detail::compute_sqrt<detail::tvec1, double, highp>::call(x));
			return tmp.x;
#		else
			return detail::compute_sqrt<detail::tvec1, double, highp>::call(x).x;
#		endif
	}
		
	template <typename T, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<T, P> sqrt(vecType<T, P> const & x)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'sqrt' only accept floating-point inputs");
		return detail::compute_sqrt<vecType, T, P>::call(x);
	}

	// inversesqrt
	GLM_FUNC_QUALIFIER float inversesqrt(float const & x)
	{
		return 1.0f / sqrt(x);
	}
	
	GLM_FUNC_QUALIFIER double inversesqrt(double const & x)
	{
		return 1.0 / sqrt(x);
	}
	
	template <template <class, precision> class vecType, typename T, precision P>
	GLM_FUNC_QUALIFIER vecType<T, P> inversesqrt
	(
		vecType<T, P> const & x
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'inversesqrt' only accept floating-point inputs");
		return detail::compute_inversesqrt<vecType, T, P>::call(x);
	}

	VECTORIZE_VEC(inversesqrt)
}//namespace glm
