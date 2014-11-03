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
/// @file glm/core/_vectorize.hpp
/// @date 2011-10-14 / 2011-10-14
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#ifndef GLM_CORE_DETAIL_INCLUDED
#define GLM_CORE_DETAIL_INCLUDED

#include "type_vec1.hpp"
#include "type_vec2.hpp"
#include "type_vec3.hpp"
#include "type_vec4.hpp"

#define VECTORIZE1_VEC(func)						\
	template <typename T, precision P>				\
	GLM_FUNC_QUALIFIER detail::tvec1<T, P> func(	\
		detail::tvec1<T, P> const & v)				\
	{												\
		return detail::tvec1<T, P>(					\
			func(v.x));								\
	}

#define VECTORIZE2_VEC(func)						\
	template <typename T, precision P>				\
	GLM_FUNC_QUALIFIER detail::tvec2<T, P> func(	\
		detail::tvec2<T, P> const & v)				\
	{												\
		return detail::tvec2<T, P>(					\
			func(v.x),								\
			func(v.y));								\
	}

#define VECTORIZE3_VEC(func)						\
	template <typename T, precision P>				\
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> func(	\
		detail::tvec3<T, P> const & v)				\
	{												\
		return detail::tvec3<T, P>(					\
			func(v.x),								\
			func(v.y),								\
			func(v.z));								\
	}

#define VECTORIZE4_VEC(func)						\
	template <typename T, precision P>				\
	GLM_FUNC_QUALIFIER detail::tvec4<T, P> func(	\
		detail::tvec4<T, P> const & v)				\
	{												\
		return detail::tvec4<T, P>(					\
			func(v.x),								\
			func(v.y),								\
			func(v.z),								\
			func(v.w));								\
	}

#define VECTORIZE_VEC(func)		\
	VECTORIZE1_VEC(func)		\
	VECTORIZE2_VEC(func)		\
	VECTORIZE3_VEC(func)		\
	VECTORIZE4_VEC(func)

#define VECTORIZE1_VEC_SCA(func)							\
	template <typename T, precision P>						\
	GLM_FUNC_QUALIFIER detail::tvec1<T, P> func				\
	(														\
		detail::tvec1<T, P> const & x,						\
		T const & y											\
	)														\
	{														\
		return detail::tvec1<T, P>(							\
			func(x.x, y));									\
	}

#define VECTORIZE2_VEC_SCA(func)							\
	template <typename T, precision P>						\
	GLM_FUNC_QUALIFIER detail::tvec2<T, P> func				\
	(														\
		detail::tvec2<T, P> const & x,						\
		T const & y	\
	)														\
	{														\
		return detail::tvec2<T, P>(							\
			func(x.x, y),									\
			func(x.y, y));									\
	}

#define VECTORIZE3_VEC_SCA(func)							\
	template <typename T, precision P>						\
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> func				\
	(														\
		detail::tvec3<T, P> const & x,						\
		T const & y	\
	)														\
	{														\
		return detail::tvec3<T, P>(							\
			func(x.x, y),									\
			func(x.y, y),									\
			func(x.z, y));									\
	}

#define VECTORIZE4_VEC_SCA(func)							\
	template <typename T, precision P>						\
	GLM_FUNC_QUALIFIER detail::tvec4<T, P> func				\
	(														\
		detail::tvec4<T, P> const & x,						\
		T const & y	\
	)														\
	{														\
		return detail::tvec4<T, P>(							\
			func(x.x, y),									\
			func(x.y, y),									\
			func(x.z, y),									\
			func(x.w, y));									\
	}

#define VECTORIZE_VEC_SCA(func)		\
	VECTORIZE1_VEC_SCA(func)		\
	VECTORIZE2_VEC_SCA(func)		\
	VECTORIZE3_VEC_SCA(func)		\
	VECTORIZE4_VEC_SCA(func)

#define VECTORIZE2_VEC_VEC(func)					\
	template <typename T, precision P>				\
	GLM_FUNC_QUALIFIER detail::tvec2<T, P> func		\
	(												\
		detail::tvec2<T, P> const & x,				\
		detail::tvec2<T, P> const & y				\
	)												\
	{												\
		return detail::tvec2<T, P>(					\
			func(x.x, y.x),							\
			func(x.y, y.y));						\
	}

#define VECTORIZE3_VEC_VEC(func)					\
	template <typename T, precision P>				\
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> func		\
	(												\
		detail::tvec3<T, P> const & x,				\
		detail::tvec3<T, P> const & y				\
	)												\
	{												\
		return detail::tvec3<T, P>(					\
			func(x.x, y.x),							\
			func(x.y, y.y),							\
			func(x.z, y.z));						\
	}

#define VECTORIZE4_VEC_VEC(func)				\
	template <typename T, precision P>			\
	GLM_FUNC_QUALIFIER detail::tvec4<T, P> func	\
	(											\
		detail::tvec4<T, P> const & x,			\
		detail::tvec4<T, P> const & y			\
	)											\
	{											\
		return detail::tvec4<T, P>(				\
			func(x.x, y.x),						\
			func(x.y, y.y),						\
			func(x.z, y.z),						\
			func(x.w, y.w));					\
	}

#define VECTORIZE_VEC_VEC(func)		\
	VECTORIZE2_VEC_VEC(func)		\
	VECTORIZE3_VEC_VEC(func)		\
	VECTORIZE4_VEC_VEC(func)

namespace glm{
namespace detail
{
	template<bool C>
	struct If
	{
		template<typename F, typename T>
		static GLM_FUNC_QUALIFIER T apply(F functor, const T& val)
		{
			return functor(val);
		}
	};

	template<>
	struct If<false>
	{
		template<typename F, typename T>
		static GLM_FUNC_QUALIFIER T apply(F, const T& val)
		{
			return val;
		}
	};
}//namespace detail
}//namespace glm

#endif//GLM_CORE_DETAIL_INCLUDED
