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
/// @file glm/core/type_vec1.hpp
/// @date 2008-08-25 / 2011-06-15
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#ifndef glm_core_type_gentype1
#define glm_core_type_gentype1

#include "../fwd.hpp"
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
	struct tvec1
	{
		//////////////////////////////////////
		// Implementation detail

		enum ctor{_null};

		typedef tvec1<T, P> type;
		typedef tvec1<bool, P> bool_type;
		typedef T value_type;

		//////////////////////////////////////
		// Helper

		GLM_FUNC_DECL GLM_CONSTEXPR length_t length() const;

		//////////////////////////////////////
		// Data

		union {T x, r, s;};

		//////////////////////////////////////
		// Accesses

		GLM_FUNC_DECL T & operator[](length_t i);
		GLM_FUNC_DECL T const & operator[](length_t i) const;

		//////////////////////////////////////
		// Implicit basic constructors

		GLM_FUNC_DECL tvec1();
		GLM_FUNC_DECL tvec1(tvec1<T, P> const & v);
		template <precision Q>
		GLM_FUNC_DECL tvec1(tvec1<T, Q> const & v);

		//////////////////////////////////////
		// Explicit basic constructors

		GLM_FUNC_DECL explicit tvec1(
			ctor);
		GLM_FUNC_DECL tvec1(
			T const & s);

		//////////////////////////////////////
		// Conversion vector constructors
		
		//! Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
		template <typename U, precision Q>
		GLM_FUNC_DECL explicit tvec1(tvec1<U, Q> const & v);
		//! Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
		template <typename U, precision Q>
		GLM_FUNC_DECL explicit tvec1(tvec2<U, Q> const & v);
		//! Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
		template <typename U, precision Q>
		GLM_FUNC_DECL explicit tvec1(tvec3<U, Q> const & v);
		//! Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
		template <typename U, precision Q>
		GLM_FUNC_DECL explicit tvec1(tvec4<U, Q> const & v);

		//////////////////////////////////////
		// Unary arithmetic operators

		GLM_FUNC_DECL tvec1<T, P> & operator= (tvec1<T, P> const & v);
		template <typename U> 
		GLM_FUNC_DECL tvec1<T, P> & operator= (tvec1<U, P> const & v);

		template <typename U> 
		GLM_FUNC_DECL tvec1<T, P> & operator+=(U const & s);
		template <typename U> 
		GLM_FUNC_DECL tvec1<T, P> & operator+=(tvec1<U, P> const & v);
		template <typename U> 
		GLM_FUNC_DECL tvec1<T, P> & operator-=(U const & s);
		template <typename U> 
		GLM_FUNC_DECL tvec1<T, P> & operator-=(tvec1<U, P> const & v);
		template <typename U> 
		GLM_FUNC_DECL tvec1<T, P> & operator*=(U const & s);
		template <typename U> 
		GLM_FUNC_DECL tvec1<T, P> & operator*=(tvec1<U, P> const & v);
		template <typename U> 
		GLM_FUNC_DECL tvec1<T, P> & operator/=(U const & s);
		template <typename U> 
		GLM_FUNC_DECL tvec1<T, P> & operator/=(tvec1<U, P> const & v);

		//////////////////////////////////////
		// Increment and decrement operators

		GLM_FUNC_DECL tvec1<T, P> & operator++();
		GLM_FUNC_DECL tvec1<T, P> & operator--();
		GLM_FUNC_DECL tvec1<T, P> operator++(int);
		GLM_FUNC_DECL tvec1<T, P> operator--(int);

		//////////////////////////////////////
		// Unary bit operators

		template <typename U> 
		GLM_FUNC_DECL tvec1<T, P> & operator%=(U const & s);
		template <typename U> 
		GLM_FUNC_DECL tvec1<T, P> & operator%=(tvec1<U, P> const & v);
		template <typename U> 
		GLM_FUNC_DECL tvec1<T, P> & operator&=(U const & s);
		template <typename U> 
		GLM_FUNC_DECL tvec1<T, P> & operator&=(tvec1<U, P> const & v);
		template <typename U> 
		GLM_FUNC_DECL tvec1<T, P> & operator|=(U const & s);
		template <typename U> 
		GLM_FUNC_DECL tvec1<T, P> & operator|=(tvec1<U, P> const & v);
		template <typename U> 
		GLM_FUNC_DECL tvec1<T, P> & operator^=(U const & s);
		template <typename U> 
		GLM_FUNC_DECL tvec1<T, P> & operator^=(tvec1<U, P> const & v);
		template <typename U> 
		GLM_FUNC_DECL tvec1<T, P> & operator<<=(U const & s);
		template <typename U> 
		GLM_FUNC_DECL tvec1<T, P> & operator<<=(tvec1<U, P> const & v);
		template <typename U> 
		GLM_FUNC_DECL tvec1<T, P> & operator>>=(U const & s);
		template <typename U> 
		GLM_FUNC_DECL tvec1<T, P> & operator>>=(tvec1<U, P> const & v);
	};


	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator+(tvec1<T, P> const & v, T const & s);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator+(T const & s, tvec1<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator+(tvec1<T, P> const & v1, tvec1<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator-(tvec1<T, P> const & v, T const & s);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator-(T const & s, tvec1<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator-	(tvec1<T, P> const & v1, tvec1<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator*(tvec1<T, P> const & v, T const & s);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator*(T const & s, tvec1<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator*(tvec1<T, P> const & v1, tvec1<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator/(tvec1<T, P> const & v, T const & s);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator/(T const & s, tvec1<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator/(tvec1<T, P> const & v1, tvec1<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator-(tvec1<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL bool operator==(tvec1<T, P> const & v1, tvec1<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL bool operator!=(tvec1<T, P> const & v1, tvec1<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator%(tvec1<T, P> const & v, T const & s);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator%(T const & s, tvec1<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator%(tvec1<T, P> const & v1, tvec1<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator&(tvec1<T, P> const & v, T const & s);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator&(T const & s, tvec1<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator&(tvec1<T, P> const & v1, tvec1<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator|(tvec1<T, P> const & v, T const & s);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator|(T const & s, tvec1<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator|(tvec1<T, P> const & v1, tvec1<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator^(tvec1<T, P> const & v, T const & s);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator^(T const & s, tvec1<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator^(tvec1<T, P> const & v1, tvec1<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator<<(tvec1<T, P> const & v, T const & s);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator<<(T const & s, tvec1<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator<<(tvec1<T, P> const & v1, tvec1<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator>>(tvec1<T, P> const & v, T const & s);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator>>(T const & s, tvec1<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator>>(tvec1<T, P> const & v1, tvec1<T, P> const & v2);

	template <typename T, precision P> 
	GLM_FUNC_DECL tvec1<T, P> operator~(tvec1<T, P> const & v);

}//namespace detail
}//namespace glm

#ifndef GLM_EXTERNAL_TEMPLATE
#include "type_vec1.inl"
#endif//GLM_EXTERNAL_TEMPLATE

#endif//glm_core_type_gentype1
