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
/// @file glm/core/func_common.inl
/// @date 2008-08-03 / 2011-06-15
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#include "func_vector_relational.hpp"
#include "type_vec2.hpp"
#include "type_vec3.hpp"
#include "type_vec4.hpp"
#include "_vectorize.hpp"
#include <limits>

namespace glm{
namespace detail
{
	template <typename genFIType, bool /*signed*/>
	struct compute_abs
	{};

	template <typename genFIType>
	struct compute_abs<genFIType, true>
	{
		GLM_FUNC_QUALIFIER static genFIType call(genFIType const & x)
		{
			GLM_STATIC_ASSERT(
				std::numeric_limits<genFIType>::is_iec559 || std::numeric_limits<genFIType>::is_signed,
				"'abs' only accept floating-point and integer scalar or vector inputs");
			return x >= genFIType(0) ? x : -x;
			// TODO, perf comp with: *(((int *) &x) + 1) &= 0x7fffffff;
		}
	};

	template <typename genFIType>
	struct compute_abs<genFIType, false>
	{
		GLM_FUNC_QUALIFIER static genFIType call(genFIType const & x)
		{
			GLM_STATIC_ASSERT(
				!std::numeric_limits<genFIType>::is_signed && std::numeric_limits<genFIType>::is_integer,
				"'abs' only accept floating-point and integer scalar or vector inputs");
			return x;
		}
	};

	template <typename T, typename U, precision P, template <class, precision> class vecType>
	struct compute_mix_vector
	{
		GLM_FUNC_QUALIFIER static vecType<T, P> call(vecType<T, P> const & x, vecType<T, P> const & y, vecType<U, P> const & a)
		{
			GLM_STATIC_ASSERT(std::numeric_limits<U>::is_iec559, "'mix' only accept floating-point inputs for the interpolator a");

			return vecType<T, P>(vecType<U, P>(x) + a * vecType<U, P>(y - x));
		}
	};

	template <typename T, precision P, template <class, precision> class vecType>
	struct compute_mix_vector<T, bool, P, vecType>
	{
		GLM_FUNC_QUALIFIER static vecType<T, P> call(vecType<T, P> const & x, vecType<T, P> const & y, vecType<bool, P> const & a)
		{
			vecType<T, P> Result;
			for(length_t i = 0; i < x.length(); ++i)
				Result[i] = a[i] ? y[i] : x[i];
			return Result;
		}
	};

	template <typename T, typename U, precision P, template <class, precision> class vecType>
	struct compute_mix_scalar
	{
		GLM_FUNC_QUALIFIER static vecType<T, P> call(vecType<T, P> const & x, vecType<T, P> const & y, U const & a)
		{
			GLM_STATIC_ASSERT(std::numeric_limits<U>::is_iec559, "'mix' only accept floating-point inputs for the interpolator a");

			return vecType<T, P>(vecType<U, P>(x) + a * vecType<U, P>(y - x));
		}
	};

	template <typename T, precision P, template <class, precision> class vecType>
	struct compute_mix_scalar<T, bool, P, vecType>
	{
		GLM_FUNC_QUALIFIER static vecType<T, P> call(vecType<T, P> const & x, vecType<T, P> const & y, bool const & a)
		{
			return a ? y : x;
		}
	};

	template <typename T, typename U>
	struct compute_mix
	{
		GLM_FUNC_QUALIFIER static T call(T const & x, T const & y, U const & a)
		{
			GLM_STATIC_ASSERT(std::numeric_limits<U>::is_iec559, "'mix' only accept floating-point inputs for the interpolator a");

			return static_cast<T>(static_cast<U>(x) + a * static_cast<U>(y - x));
		}
	};

	template <typename T>
	struct compute_mix<T, bool>
	{
		GLM_FUNC_QUALIFIER static T call(T const & x, T const & y, bool const & a)
		{
			return a ? y : x;
		}
	};
}//namespace detail

	// abs
	template <typename genFIType>
	GLM_FUNC_QUALIFIER genFIType abs
	(
		genFIType const & x
	)
	{
		return detail::compute_abs<genFIType, std::numeric_limits<genFIType>::is_signed>::call(x);
	}

	VECTORIZE_VEC(abs)

	// sign
	//Try something like based on x >> 31 to get the sign bit
	template <typename genFIType> 
	GLM_FUNC_QUALIFIER genFIType sign
	(
		genFIType const & x
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<genFIType>::is_iec559 ||
			(std::numeric_limits<genFIType>::is_signed && std::numeric_limits<genFIType>::is_integer), "'sign' only accept signed inputs");

		genFIType result;
		if(x > genFIType(0))
			result = genFIType(1);
		else if(x < genFIType(0))
			result = genFIType(-1);
		else
			result = genFIType(0);
		return result;
	}
	
	VECTORIZE_VEC(sign)

	// floor
	template <typename genType>
	GLM_FUNC_QUALIFIER genType floor(genType const & x)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<genType>::is_iec559,
			"'floor' only accept floating-point inputs");

		return ::std::floor(x);
	}

	VECTORIZE_VEC(floor)

	// trunc
	template <typename genType>
	GLM_FUNC_QUALIFIER genType trunc(genType const & x)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<genType>::is_iec559,
			"'trunc' only accept floating-point inputs");

		// TODO, add C++11 std::trunk
		return x < 0 ? -floor(-x) : floor(x);
	}

	VECTORIZE_VEC(trunc)

	// round
	template <typename genType>
	GLM_FUNC_QUALIFIER genType round(genType const& x)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<genType>::is_iec559,
			"'round' only accept floating-point inputs");

		// TODO, add C++11 std::round
		return x < 0 ? genType(int(x - genType(0.5))) : genType(int(x + genType(0.5)));
	}

	VECTORIZE_VEC(round)

/*
	// roundEven
	template <typename genType>
	GLM_FUNC_QUALIFIER genType roundEven(genType const& x)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<genType>::is_iec559, "'roundEven' only accept floating-point inputs");

		return genType(int(x + genType(int(x) % 2)));
	}
*/
	
	// roundEven
	template <typename genType>
	GLM_FUNC_QUALIFIER genType roundEven(genType const & x)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<genType>::is_iec559,
			"'roundEven' only accept floating-point inputs");
		
		int Integer = static_cast<int>(x);
		genType IntegerPart = static_cast<genType>(Integer);
		genType FractionalPart = fract(x);

		if(FractionalPart > static_cast<genType>(0.5) || FractionalPart < static_cast<genType>(0.5))
		{
			return round(x);
		}
		else if((Integer % 2) == 0)
		{
			return IntegerPart;
		}
		else if(x <= static_cast<genType>(0)) // Work around... 
		{
			return IntegerPart - static_cast<genType>(1);
		}
		else
		{
			return IntegerPart + static_cast<genType>(1);
		}
		//else // Bug on MinGW 4.5.2
		//{
		//	return mix(IntegerPart + genType(-1), IntegerPart + genType(1), x <= genType(0));
		//}
	}
	
	VECTORIZE_VEC(roundEven)

	// ceil
	template <typename genType>
	GLM_FUNC_QUALIFIER genType ceil(genType const & x)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<genType>::is_iec559,
			"'ceil' only accept floating-point inputs");

		return ::std::ceil(x);
	}

	VECTORIZE_VEC(ceil)

	// fract
	template <typename genType>
	GLM_FUNC_QUALIFIER genType fract
	(
		genType const & x
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<genType>::is_iec559,
			"'fract' only accept floating-point inputs");

		return x - floor(x);
	}

	VECTORIZE_VEC(fract)

	// mod
	template <typename genType>
	GLM_FUNC_QUALIFIER genType mod
	(
		genType const & x, 
		genType const & y
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<genType>::is_iec559,
			"'mod' only accept floating-point inputs");

		return x - y * floor(x / y);
	}

	VECTORIZE_VEC_SCA(mod)
	VECTORIZE_VEC_VEC(mod)

	// modf
	template <typename genType>
	GLM_FUNC_QUALIFIER genType modf
	(
		genType const & x, 
		genType & i
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<genType>::is_iec559,
			"'modf' only accept floating-point inputs");

		return std::modf(x, &i);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec2<T, P> modf
	(
		detail::tvec2<T, P> const & x,
		detail::tvec2<T, P> & i
	)
	{
		return detail::tvec2<T, P>(
			modf(x.x, i.x),
			modf(x.y, i.y));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> modf
	(
		detail::tvec3<T, P> const & x,
		detail::tvec3<T, P> & i
	)
	{
		return detail::tvec3<T, P>(
			modf(x.x, i.x),
			modf(x.y, i.y),
			modf(x.z, i.z));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec4<T, P> modf
	(
		detail::tvec4<T, P> const & x,
		detail::tvec4<T, P> & i
	)
	{
		return detail::tvec4<T, P>(
			modf(x.x, i.x),
			modf(x.y, i.y),
			modf(x.z, i.z),
			modf(x.w, i.w));
	}

	//// Only valid if (INT_MIN <= x-y <= INT_MAX)
	//// min(x,y)
	//r = y + ((x - y) & ((x - y) >> (sizeof(int) *
	//CHAR_BIT - 1)));
	//// max(x,y)
	//r = x - ((x - y) & ((x - y) >> (sizeof(int) *
	//CHAR_BIT - 1)));

	// min
	template <typename genType>
	GLM_FUNC_QUALIFIER genType min
	(
		genType const & x,
		genType const & y
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<genType>::is_iec559 || std::numeric_limits<genType>::is_integer,
			"'min' only accept floating-point or integer inputs");

		return x < y ? x : y;
	}

	VECTORIZE_VEC_SCA(min)
	VECTORIZE_VEC_VEC(min)

	// max
	template <typename genType>
	GLM_FUNC_QUALIFIER genType max
	(
		genType const & x,
		genType const & y
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<genType>::is_iec559 || std::numeric_limits<genType>::is_integer,
			"'max' only accept floating-point or integer inputs");

		return x > y ? x : y;
	}

	VECTORIZE_VEC_SCA(max)
	VECTORIZE_VEC_VEC(max)

	// clamp
	template <typename genType>
	GLM_FUNC_QUALIFIER genType clamp
	(
		genType const & x,
		genType const & minVal,
		genType const & maxVal
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<genType>::is_iec559 || std::numeric_limits<genType>::is_integer,
			"'clamp' only accept floating-point or integer inputs");
		
		return min(maxVal, max(minVal, x));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec2<T, P> clamp
	(
		detail::tvec2<T, P> const & x,
		T const & minVal,
		T const & maxVal
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<T>::is_iec559 || std::numeric_limits<T>::is_integer,
			"'clamp' only accept floating-point or integer inputs");

		return detail::tvec2<T, P>(
			clamp(x.x, minVal, maxVal),
			clamp(x.y, minVal, maxVal));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> clamp
	(
		detail::tvec3<T, P> const & x,
		T const & minVal,
		T const & maxVal
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<T>::is_iec559 || std::numeric_limits<T>::is_integer,
			"'clamp' only accept floating-point or integer inputs");

		return detail::tvec3<T, P>(
			clamp(x.x, minVal, maxVal),
			clamp(x.y, minVal, maxVal),
			clamp(x.z, minVal, maxVal));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec4<T, P> clamp
	(
		detail::tvec4<T, P> const & x,
		T const & minVal,
		T const & maxVal
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<T>::is_iec559 || std::numeric_limits<T>::is_integer,
			"'clamp' only accept floating-point or integer inputs");

		return detail::tvec4<T, P>(
			clamp(x.x, minVal, maxVal),
			clamp(x.y, minVal, maxVal),
			clamp(x.z, minVal, maxVal),
			clamp(x.w, minVal, maxVal));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec2<T, P> clamp
	(
		detail::tvec2<T, P> const & x,
		detail::tvec2<T, P> const & minVal,
		detail::tvec2<T, P> const & maxVal
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<T>::is_iec559 || std::numeric_limits<T>::is_integer,
			"'clamp' only accept floating-point or integer inputs");

		return detail::tvec2<T, P>(
			clamp(x.x, minVal.x, maxVal.x),
			clamp(x.y, minVal.y, maxVal.y));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> clamp
	(
		detail::tvec3<T, P> const & x,
		detail::tvec3<T, P> const & minVal,
		detail::tvec3<T, P> const & maxVal
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<T>::is_iec559 || std::numeric_limits<T>::is_integer,
			"'clamp' only accept floating-point or integer inputs");

		return detail::tvec3<T, P>(
			clamp(x.x, minVal.x, maxVal.x),
			clamp(x.y, minVal.y, maxVal.y),
			clamp(x.z, minVal.z, maxVal.z));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec4<T, P> clamp
	(
		detail::tvec4<T, P> const & x,
		detail::tvec4<T, P> const & minVal,
		detail::tvec4<T, P> const & maxVal
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<T>::is_iec559 || std::numeric_limits<T>::is_integer,
			"'clamp' only accept floating-point or integer inputs");

		return detail::tvec4<T, P>(
			clamp(x.x, minVal.x, maxVal.x),
			clamp(x.y, minVal.y, maxVal.y),
			clamp(x.z, minVal.z, maxVal.z),
			clamp(x.w, minVal.w, maxVal.w));
	}

	template <typename T, typename U, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<T, P> mix
	(
		vecType<T, P> const & x,
		vecType<T, P> const & y,
		vecType<U, P> const & a
	)
	{
		return detail::compute_mix_vector<T, U, P, vecType>::call(x, y, a);
	}

	template <typename T, typename U, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<T, P> mix
	(
		vecType<T, P> const & x,
		vecType<T, P> const & y,
		U const & a
	)
	{
		return detail::compute_mix_scalar<T, U, P, vecType>::call(x, y, a);
	}

	template <typename genTypeT, typename genTypeU>
	GLM_FUNC_QUALIFIER genTypeT mix
	(
		genTypeT const & x,
		genTypeT const & y,
		genTypeU const & a
	)
	{
		return detail::compute_mix<genTypeT, genTypeU>::call(x, y, a);
	}

	// step
	template <typename genType>
	GLM_FUNC_QUALIFIER genType step
	(
		genType const & edge,
		genType const & x
	)
	{
		return mix(genType(1), genType(0), glm::lessThan(x, edge));
	}

	template <template <typename, precision> class vecType, typename T, precision P>
	GLM_FUNC_QUALIFIER vecType<T, P> step
	(
		T const & edge,
		vecType<T, P> const & x
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<T>::is_iec559,
			"'step' only accept floating-point inputs");

		return mix(vecType<T, P>(1), vecType<T, P>(0), glm::lessThan(x, vecType<T, P>(edge)));
	}

	// smoothstep
	template <typename genType>
	GLM_FUNC_QUALIFIER genType smoothstep
	(
		genType const & edge0,
		genType const & edge1,
		genType const & x
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<genType>::is_iec559,
			"'smoothstep' only accept floating-point inputs");

		genType tmp = clamp((x - edge0) / (edge1 - edge0), genType(0), genType(1));
		return tmp * tmp * (genType(3) - genType(2) * tmp);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec2<T, P> smoothstep
	(
		T const & edge0,
		T const & edge1,
		detail::tvec2<T, P> const & x
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<T>::is_iec559,
			"'smoothstep' only accept floating-point inputs");

		return detail::tvec2<T, P>(
			smoothstep(edge0, edge1, x.x),
			smoothstep(edge0, edge1, x.y));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> smoothstep
	(
		T const & edge0,
		T const & edge1,
		detail::tvec3<T, P> const & x
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<T>::is_iec559,
			"'smoothstep' only accept floating-point inputs");

		return detail::tvec3<T, P>(
			smoothstep(edge0, edge1, x.x),
			smoothstep(edge0, edge1, x.y),
			smoothstep(edge0, edge1, x.z));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec4<T, P> smoothstep
	(
		T const & edge0,
		T const & edge1,
		detail::tvec4<T, P> const & x
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<T>::is_iec559,
			"'smoothstep' only accept floating-point inputs");

		return detail::tvec4<T, P>(
			smoothstep(edge0, edge1, x.x),
			smoothstep(edge0, edge1, x.y),
			smoothstep(edge0, edge1, x.z),
			smoothstep(edge0, edge1, x.w));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec2<T, P> smoothstep
	(
		detail::tvec2<T, P> const & edge0,
		detail::tvec2<T, P> const & edge1,
		detail::tvec2<T, P> const & x
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<T>::is_iec559,
			"'smoothstep' only accept floating-point inputs");

		return detail::tvec2<T, P>(
			smoothstep(edge0.x, edge1.x, x.x),
			smoothstep(edge0.y, edge1.y, x.y));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> smoothstep
	(
		detail::tvec3<T, P> const & edge0,
		detail::tvec3<T, P> const & edge1,
		detail::tvec3<T, P> const & x
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<T>::is_iec559,
			"'smoothstep' only accept floating-point inputs");

		return detail::tvec3<T, P>(
			smoothstep(edge0.x, edge1.x, x.x),
			smoothstep(edge0.y, edge1.y, x.y),
			smoothstep(edge0.z, edge1.z, x.z));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec4<T, P> smoothstep
	(
		detail::tvec4<T, P> const & edge0,
		detail::tvec4<T, P> const & edge1,
		detail::tvec4<T, P> const & x
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<T>::is_iec559,
			"'smoothstep' only accept floating-point inputs");

		return detail::tvec4<T, P>(
			smoothstep(edge0.x, edge1.x, x.x),
			smoothstep(edge0.y, edge1.y, x.y),
			smoothstep(edge0.z, edge1.z, x.z),
			smoothstep(edge0.w, edge1.w, x.w));
	}

	// TODO: Not working on MinGW...
	template <typename genType> 
	GLM_FUNC_QUALIFIER bool isnan(genType const & x)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<genType>::is_iec559,
			"'isnan' only accept floating-point inputs");

#		if(GLM_COMPILER & (GLM_COMPILER_VC | GLM_COMPILER_INTEL))
			return _isnan(x) != 0;
#		elif(GLM_COMPILER & (GLM_COMPILER_GCC | GLM_COMPILER_CLANG))
#			if(GLM_PLATFORM & GLM_PLATFORM_ANDROID && __cplusplus < 201103L)
				return _isnan(x) != 0;
#			else
				return std::isnan(x);
#			endif
#		elif(GLM_COMPILER & GLM_COMPILER_CUDA)
			return isnan(x) != 0;
#		else
			return std::isnan(x);
#		endif
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER typename detail::tvec2<T, P>::bool_type isnan
	(
		detail::tvec2<T, P> const & x
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<T>::is_iec559,
			"'isnan' only accept floating-point inputs");

		return typename detail::tvec2<T, P>::bool_type(
			isnan(x.x),
			isnan(x.y));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER typename detail::tvec3<T, P>::bool_type isnan
	(
		detail::tvec3<T, P> const & x
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<T>::is_iec559,
			"'isnan' only accept floating-point inputs");

		return typename detail::tvec3<T, P>::bool_type(
			isnan(x.x),
			isnan(x.y),
			isnan(x.z));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER typename detail::tvec4<T, P>::bool_type isnan
	(
		detail::tvec4<T, P> const & x
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<T>::is_iec559,
			"'isnan' only accept floating-point inputs");

		return typename detail::tvec4<T, P>::bool_type(
			isnan(x.x),
			isnan(x.y),
			isnan(x.z),
			isnan(x.w));
	}

	template <typename genType> 
	GLM_FUNC_QUALIFIER bool isinf(
		genType const & x)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<genType>::is_iec559,
			"'isinf' only accept floating-point inputs");

#		if(GLM_COMPILER & (GLM_COMPILER_INTEL | GLM_COMPILER_VC))
			return _fpclass(x) == _FPCLASS_NINF || _fpclass(x) == _FPCLASS_PINF;
#		elif(GLM_COMPILER & (GLM_COMPILER_GCC | GLM_COMPILER_CLANG))
#			if(GLM_PLATFORM & GLM_PLATFORM_ANDROID && __cplusplus < 201103L)
				return _isinf(x) != 0;
#			else
				return std::isinf(x);
#			endif
#		elif(GLM_COMPILER & GLM_COMPILER_CUDA)
			// http://developer.download.nvidia.com/compute/cuda/4_2/rel/toolkit/docs/online/group__CUDA__MATH__DOUBLE_g13431dd2b40b51f9139cbb7f50c18fab.html#g13431dd2b40b51f9139cbb7f50c18fab
			return isinf(double(x)) != 0;
#		else
			return std::isinf(x);
#		endif
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER typename detail::tvec2<T, P>::bool_type isinf
	(
		detail::tvec2<T, P> const & x
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<T>::is_iec559,
			"'isinf' only accept floating-point inputs");

		return typename detail::tvec2<T, P>::bool_type(
			isinf(x.x),
			isinf(x.y));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER typename detail::tvec3<T, P>::bool_type isinf
	(
		detail::tvec3<T, P> const & x
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<T>::is_iec559,
			"'isinf' only accept floating-point inputs");

		return typename detail::tvec3<T, P>::bool_type(
			isinf(x.x),
			isinf(x.y),
			isinf(x.z));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER typename detail::tvec4<T, P>::bool_type isinf
	(
		detail::tvec4<T, P> const & x
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<T>::is_iec559,
			"'isinf' only accept floating-point inputs");

		return typename detail::tvec4<T, P>::bool_type(
			isinf(x.x),
			isinf(x.y),
			isinf(x.z),
			isinf(x.w));
	}

	GLM_FUNC_QUALIFIER int floatBitsToInt(float const & v)
	{
		return reinterpret_cast<int&>(const_cast<float&>(v));
	}

	template <template <typename, precision> class vecType, precision P>
	GLM_FUNC_QUALIFIER vecType<int, P> floatBitsToInt(vecType<float, P> const & v)
	{
		return reinterpret_cast<vecType<int, P>&>(const_cast<vecType<float, P>&>(v));
	}

	GLM_FUNC_QUALIFIER uint floatBitsToUint(float const & v)
	{
		return reinterpret_cast<uint&>(const_cast<float&>(v));
	}

	template <template <typename, precision> class vecType, precision P>
	GLM_FUNC_QUALIFIER vecType<uint, P> floatBitsToUint(vecType<float, P> const & v)
	{
		return reinterpret_cast<vecType<uint, P>&>(const_cast<vecType<float, P>&>(v));
	}

	GLM_FUNC_QUALIFIER float intBitsToFloat(int const & v)
	{
		return reinterpret_cast<float&>(const_cast<int&>(v));
	}

	template <template <typename, precision> class vecType, precision P>
	GLM_FUNC_QUALIFIER vecType<float, P> intBitsToFloat(vecType<int, P> const & v)
	{
		return reinterpret_cast<vecType<float, P>&>(const_cast<vecType<int, P>&>(v));
	}

	GLM_FUNC_QUALIFIER float uintBitsToFloat(uint const & v)
	{
		return reinterpret_cast<float&>(const_cast<uint&>(v));
	}

	template <template <typename, precision> class vecType, precision P>
	GLM_FUNC_QUALIFIER vecType<float, P> uintBitsToFloat(vecType<uint, P> const & v)
	{
		return reinterpret_cast<vecType<float, P>&>(const_cast<vecType<uint, P>&>(v));
	}
	
	template <typename genType>
	GLM_FUNC_QUALIFIER genType fma
	(
		genType const & a,
		genType const & b,
		genType const & c
	)
	{
		return a * b + c;
	}

	template <typename genType>
	GLM_FUNC_QUALIFIER genType frexp
	(
		genType const & x,
		int & exp
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<genType>::is_iec559,
			"'frexp' only accept floating-point inputs");

		return std::frexp(x, exp);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec2<T, P> frexp
	(
		detail::tvec2<T, P> const & x,
		detail::tvec2<int, P> & exp
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<T>::is_iec559,
			"'frexp' only accept floating-point inputs");

		return detail::tvec2<T, P>(
			frexp(x.x, exp.x),
			frexp(x.y, exp.y));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> frexp
	(
		detail::tvec3<T, P> const & x,
		detail::tvec3<int, P> & exp
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<T>::is_iec559,
			"'frexp' only accept floating-point inputs");

		return detail::tvec3<T, P>(
			frexp(x.x, exp.x),
			frexp(x.y, exp.y),
			frexp(x.z, exp.z));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec4<T, P> frexp
	(
		detail::tvec4<T, P> const & x,
		detail::tvec4<int, P> & exp
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<T>::is_iec559,
			"'frexp' only accept floating-point inputs");

		return detail::tvec4<T, P>(
			frexp(x.x, exp.x),
			frexp(x.y, exp.y),
			frexp(x.z, exp.z),
			frexp(x.w, exp.w));
	}

	template <typename genType, precision P>
	GLM_FUNC_QUALIFIER genType ldexp
	(
		genType const & x,
		int const & exp
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<genType>::is_iec559,
			"'frexp' only accept floating-point inputs");

		return std::ldexp(x, exp);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec2<T, P> ldexp
	(
		detail::tvec2<T, P> const & x,
		detail::tvec2<int, P> const & exp
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<T>::is_iec559,
			"'ldexp' only accept floating-point inputs");

		return detail::tvec2<T, P>(
			ldexp(x.x, exp.x),
			ldexp(x.y, exp.y));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> ldexp
	(
		detail::tvec3<T, P> const & x,
		detail::tvec3<int, P> const & exp
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<T>::is_iec559,
			"'ldexp' only accept floating-point inputs");

		return detail::tvec3<T, P>(
			ldexp(x.x, exp.x),
			ldexp(x.y, exp.y),
			ldexp(x.z, exp.z));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec4<T, P> ldexp
	(
		detail::tvec4<T, P> const & x,
		detail::tvec4<int, P> const & exp
	)
	{
		GLM_STATIC_ASSERT(
			std::numeric_limits<T>::is_iec559,
			"'ldexp' only accept floating-point inputs");

		return detail::tvec4<T, P>(
			ldexp(x.x, exp.x),
			ldexp(x.y, exp.y),
			ldexp(x.z, exp.z),
			ldexp(x.w, exp.w));
	}

}//namespace glm
