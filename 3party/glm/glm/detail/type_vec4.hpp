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
/// @file glm/core/type_vec4.hpp
/// @date 2008-08-22 / 2011-06-15
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#ifndef glm_core_type_gentype4
#define glm_core_type_gentype4

//#include "../fwd.hpp"
#include "setup.hpp"
#include "type_vec.hpp"
#ifdef GLM_SWIZZLE
#	if GLM_HAS_ANONYMOUS_UNION
#		include "_swizzle.hpp"
#	else
#		include "_swizzle_func.hpp"
#	endif
#endif //GLM_SWIZZLE
#include <cstddef>

namespace glm{
namespace detail
{
	template <typename T, precision P>
	struct tvec4
	{
		//////////////////////////////////////
		// Implementation detail

		enum ctor{_null};

		typedef tvec4<T, P> type;
		typedef tvec4<bool, P> bool_type;
		typedef T value_type;
		typedef int size_type;

		//////////////////////////////////////
		// Helper

		GLM_FUNC_DECL GLM_CONSTEXPR length_t length() const;

		//////////////////////////////////////
		// Data

#		if(GLM_HAS_ANONYMOUS_UNION && defined(GLM_SWIZZLE))
			union
			{
				struct { T r, g, b, a; };
				struct { T s, t, p, q; };
				struct { T x, y, z, w;};

				_GLM_SWIZZLE4_2_MEMBERS(T, P, tvec2, x, y, z, w)
				_GLM_SWIZZLE4_2_MEMBERS(T, P, tvec2, r, g, b, a)
				_GLM_SWIZZLE4_2_MEMBERS(T, P, tvec2, s, t, p, q)
				_GLM_SWIZZLE4_3_MEMBERS(T, P, tvec3, x, y, z, w)
				_GLM_SWIZZLE4_3_MEMBERS(T, P, tvec3, r, g, b, a)
				_GLM_SWIZZLE4_3_MEMBERS(T, P, tvec3, s, t, p, q)
				_GLM_SWIZZLE4_4_MEMBERS(T, P, tvec4, x, y, z, w)
				_GLM_SWIZZLE4_4_MEMBERS(T, P, tvec4, r, g, b, a)
				_GLM_SWIZZLE4_4_MEMBERS(T, P, tvec4, s, t, p, q)
			};
#		else
			union { T x, r, s; };
			union { T y, g, t; };
			union { T z, b, p; };
			union { T w, a, q; };

#			ifdef GLM_SWIZZLE
				GLM_SWIZZLE_GEN_VEC_FROM_VEC4(T, P, detail::tvec4, detail::tvec2, detail::tvec3, detail::tvec4)
#			endif
#		endif//GLM_LANG

		//////////////////////////////////////
		// Accesses

		GLM_FUNC_DECL T & operator[](length_t i);
		GLM_FUNC_DECL T const & operator[](length_t i) const;

		//////////////////////////////////////
		// Implicit basic constructors

		GLM_FUNC_DECL tvec4();
		GLM_FUNC_DECL tvec4(type const & v);
		template <precision Q>
		GLM_FUNC_DECL tvec4(tvec4<T, Q> const & v);

		//////////////////////////////////////
		// Explicit basic constructors

		GLM_FUNC_DECL explicit tvec4(
			ctor);
		GLM_FUNC_DECL explicit tvec4(
			T const & s);
		GLM_FUNC_DECL tvec4(
			T const & s0,
			T const & s1,
			T const & s2,
			T const & s3);

		//////////////////////////////////////
		// Conversion scalar constructors

		/// Explicit converions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
		template <typename A, typename B, typename C, typename D>
		GLM_FUNC_DECL tvec4(
			A const & x,
			B const & y,
			C const & z,
			D const & w);

		//////////////////////////////////////
		// Conversion vector constructors

		//! Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
		template <typename A, typename B, typename C, precision Q>
		GLM_FUNC_DECL explicit tvec4(tvec2<A, Q> const & v, B const & s1, C const & s2);
		//! Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
		template <typename A, typename B, typename C, precision Q>
		GLM_FUNC_DECL explicit tvec4(A const & s1, tvec2<B, Q> const & v, C const & s2);
		//! Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
		template <typename A, typename B, typename C, precision Q>
		GLM_FUNC_DECL explicit tvec4(A const & s1, B const & s2, tvec2<C, Q> const & v);
		//! Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
		template <typename A, typename B, precision Q>
		GLM_FUNC_DECL explicit tvec4(tvec3<A, Q> const & v, B const & s);
		//! Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
		template <typename A, typename B, precision Q>
		GLM_FUNC_DECL explicit tvec4(A const & s, tvec3<B, Q> const & v);
		//! Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
		template <typename A, typename B, precision Q>
		GLM_FUNC_DECL explicit tvec4(tvec2<A, Q> const & v1, tvec2<B, Q> const & v2);
		//! Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
		template <typename U, precision Q>
		GLM_FUNC_DECL explicit tvec4(tvec4<U, Q> const & v);

		//////////////////////////////////////
		// Swizzle constructors

#		if(GLM_HAS_ANONYMOUS_UNION && defined(GLM_SWIZZLE))
		template <int E0, int E1, int E2, int E3>
		GLM_FUNC_DECL tvec4(_swizzle<4, T, P, tvec4<T, P>, E0, E1, E2, E3> const & that)
		{
			*this = that();
		}

		template <int E0, int E1, int F0, int F1>
		GLM_FUNC_DECL tvec4(_swizzle<2, T, P, tvec2<T, P>, E0, E1, -1, -2> const & v, _swizzle<2, T, P, tvec2<T, P>, F0, F1, -1, -2> const & u)
		{
			*this = tvec4<T, P>(v(), u());
		}

		template <int E0, int E1>
		GLM_FUNC_DECL tvec4(T const & x, T const & y, _swizzle<2, T, P, tvec2<T, P>, E0, E1, -1, -2> const & v)
		{
			*this = tvec4<T, P>(x, y, v());
		}

		template <int E0, int E1>
		GLM_FUNC_DECL tvec4(T const & x, _swizzle<2, T, P, tvec2<T, P>, E0, E1, -1, -2> const & v, T const & w)
		{
			*this = tvec4<T, P>(x, v(), w);
		}

		template <int E0, int E1>
		GLM_FUNC_DECL tvec4(_swizzle<2, T, P, tvec2<T, P>, E0, E1, -1, -2> const & v, T const & z, T const & w)
		{
			*this = tvec4<T, P>(v(), z, w);
		}

		template <int E0, int E1, int E2>
		GLM_FUNC_DECL tvec4(_swizzle<3, T, P, tvec3<T, P>, E0, E1, E2, -1> const & v, T const & w)
		{
			*this = tvec4<T, P>(v(), w);
		}

		template <int E0, int E1, int E2>
		GLM_FUNC_DECL tvec4(T const & x, _swizzle<3, T, P, tvec3<T, P>, E0, E1, E2, -1> const & v)
		{
			*this = tvec4<T, P>(x, v());
		}
#		endif//(GLM_HAS_ANONYMOUS_UNION && defined(GLM_SWIZZLE))

		//////////////////////////////////////
		// Unary arithmetic operators

		GLM_FUNC_DECL tvec4<T, P> & operator= (tvec4<T, P> const & v);
		template <typename U, precision Q>
		GLM_FUNC_DECL tvec4<T, P> & operator= (tvec4<U, Q> const & v);

		template <typename U>
		GLM_FUNC_DECL tvec4<T, P> & operator+=(U s);
		template <typename U>
		GLM_FUNC_DECL tvec4<T, P> & operator+=(tvec4<U, P> const & v);
		template <typename U>
		GLM_FUNC_DECL tvec4<T, P> & operator-=(U s);
		template <typename U>
		GLM_FUNC_DECL tvec4<T, P> & operator-=(tvec4<U, P> const & v);
		template <typename U>
		GLM_FUNC_DECL tvec4<T, P> & operator*=(U s);
		template <typename U>
		GLM_FUNC_DECL tvec4<T, P> & operator*=(tvec4<U, P> const & v);
		template <typename U>
		GLM_FUNC_DECL tvec4<T, P> & operator/=(U s);
		template <typename U>
		GLM_FUNC_DECL tvec4<T, P> & operator/=(tvec4<U, P> const & v);

		//////////////////////////////////////
		// Increment and decrement operators

		GLM_FUNC_DECL tvec4<T, P> & operator++();
		GLM_FUNC_DECL tvec4<T, P> & operator--();
		GLM_FUNC_DECL tvec4<T, P> operator++(int);
		GLM_FUNC_DECL tvec4<T, P> operator--(int);

		//////////////////////////////////////
		// Unary bit operators

		template <typename U>
		GLM_FUNC_DECL tvec4<T, P> & operator%= (U s);
		template <typename U>
		GLM_FUNC_DECL tvec4<T, P> & operator%= (tvec4<U, P> const & v);
		template <typename U>
		GLM_FUNC_DECL tvec4<T, P> & operator&= (U s);
		template <typename U>
		GLM_FUNC_DECL tvec4<T, P> & operator&= (tvec4<U, P> const & v);
		template <typename U>
		GLM_FUNC_DECL tvec4<T, P> & operator|= (U s);
		template <typename U>
		GLM_FUNC_DECL tvec4<T, P> & operator|= (tvec4<U, P> const & v);
		template <typename U>
		GLM_FUNC_DECL tvec4<T, P> & operator^= (U s);
		template <typename U>
		GLM_FUNC_DECL tvec4<T, P> & operator^= (tvec4<U, P> const & v);
		template <typename U>
		GLM_FUNC_DECL tvec4<T, P> & operator<<=(U s);
		template <typename U>
		GLM_FUNC_DECL tvec4<T, P> & operator<<=(tvec4<U, P> const & v);
		template <typename U>
		GLM_FUNC_DECL tvec4<T, P> & operator>>=(U s);
		template <typename U>
		GLM_FUNC_DECL tvec4<T, P> & operator>>=(tvec4<U, P> const & v);
	};

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator+(tvec4<T, P> const & v, T const & s);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator+(T const & s, tvec4<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator+(tvec4<T, P> const & v1, tvec4<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator-(tvec4<T, P> const & v, T const & s);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator-(T const & s, tvec4<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator-	(tvec4<T, P> const & v1, tvec4<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator*(tvec4<T, P> const & v, T const & s);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator*(T const & s, tvec4<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator*(tvec4<T, P> const & v1, tvec4<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator/(tvec4<T, P> const & v, T const & s);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator/(T const & s, tvec4<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator/(tvec4<T, P> const & v1, tvec4<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator-(tvec4<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL bool operator==(tvec4<T, P> const & v1, tvec4<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL bool operator!=(tvec4<T, P> const & v1, tvec4<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator%(tvec4<T, P> const & v, T const & s);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator%(T const & s, tvec4<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator%(tvec4<T, P> const & v1, tvec4<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator&(tvec4<T, P> const & v, T const & s);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator&(T const & s, tvec4<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator&(tvec4<T, P> const & v1, tvec4<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator|(tvec4<T, P> const & v, T const & s);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator|(T const & s, tvec4<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator|(tvec4<T, P> const & v1, tvec4<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator^(tvec4<T, P> const & v, T const & s);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator^(T const & s, tvec4<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator^(tvec4<T, P> const & v1, tvec4<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator<<(tvec4<T, P> const & v, T const & s);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator<<(T const & s, tvec4<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator<<(tvec4<T, P> const & v1, tvec4<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator>>(tvec4<T, P> const & v, T const & s);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator>>(T const & s, tvec4<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec4<T, P> operator>>(tvec4<T, P> const & v1, tvec4<T, P> const & v2);

	template <typename T, precision P> 
	GLM_FUNC_DECL tvec4<T, P> operator~(tvec4<T, P> const & v);

}//namespace detail
}//namespace glm

#ifndef GLM_EXTERNAL_TEMPLATE
#include "type_vec4.inl"
#endif//GLM_EXTERNAL_TEMPLATE

#endif//glm_core_type_gentype4
