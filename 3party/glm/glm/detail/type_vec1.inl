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
/// @file glm/core/type_vec1.inl
/// @date 2008-08-25 / 2011-06-15
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

namespace glm{
namespace detail
{
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR length_t tvec1<T, P>::length() const
	{
		return 1;
	}

	//////////////////////////////////////
	// Accesses

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER T & tvec1<T, P>::operator[](length_t i)
	{
		assert(i >= 0 && i < this->length());
		return (&x)[i];
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER T const & tvec1<T, P>::operator[](length_t i) const
	{
		assert(i >= 0 && i < this->length());
		return (&x)[i];
	}

	//////////////////////////////////////
	// Implicit basic constructors

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P>::tvec1() :
		x(static_cast<T>(0))
	{}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P>::tvec1(tvec1<T, P> const & v) :
		x(v.x)
	{}

	template <typename T, precision P>
	template <precision Q>
	GLM_FUNC_QUALIFIER tvec1<T, P>::tvec1(tvec1<T, Q> const & v) :
		x(v.x)
	{}

	//////////////////////////////////////
	// Explicit basic constructors

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P>::tvec1(ctor)
	{}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P>::tvec1(T const & s) :
		x(s)
	{}

	//////////////////////////////////////
	// Conversion vector constructors

	template <typename T, precision P>
	template <typename U, precision Q>
	GLM_FUNC_QUALIFIER tvec1<T, P>::tvec1
	(
		tvec1<U, Q> const & v
	) :
		x(static_cast<T>(v.x))
	{}

	template <typename T, precision P>
	template <typename U, precision Q>
	GLM_FUNC_QUALIFIER tvec1<T, P>::tvec1
	(
		tvec2<U, Q> const & v
	) :
		x(static_cast<T>(v.x))
	{}

	template <typename T, precision P>
	template <typename U, precision Q>
	GLM_FUNC_QUALIFIER tvec1<T, P>::tvec1
	(
		tvec3<U, Q> const & v
	) :
		x(static_cast<T>(v.x))
	{}

	template <typename T, precision P>
	template <typename U, precision Q>
	GLM_FUNC_QUALIFIER tvec1<T, P>::tvec1
	(
		tvec4<U, Q> const & v
	) :
		x(static_cast<T>(v.x))
	{}

	//////////////////////////////////////
	// Unary arithmetic operators

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> & tvec1<T, P>::operator=
	(
		tvec1<T, P> const & v
	)
	{
		this->x = v.x;
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec1<T, P> & tvec1<T, P>::operator=
	(
		tvec1<U, P> const & v
	)
	{
		this->x = static_cast<T>(v.x);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec1<T, P> & tvec1<T, P>::operator+=
	(
		U const & s
	)
	{
		this->x += static_cast<T>(s);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec1<T, P> & tvec1<T, P>::operator+=
	(
		tvec1<U, P> const & v
	)
	{
		this->x += static_cast<T>(v.x);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec1<T, P> & tvec1<T, P>::operator-=
	(
		U const & s
	)
	{
		this->x -= static_cast<T>(s);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec1<T, P> & tvec1<T, P>::operator-=
	(
		tvec1<U, P> const & v
	)
	{
		this->x -= static_cast<T>(v.x);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec1<T, P> & tvec1<T, P>::operator*=
	(
		U const & s
	)
	{
		this->x *= static_cast<T>(s);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec1<T, P> & tvec1<T, P>::operator*=
	(	
		tvec1<U, P> const & v
	)
	{
		this->x *= static_cast<T>(v.x);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec1<T, P> & tvec1<T, P>::operator/=
	(
		U const & s
	)
	{
		this->x /= static_cast<T>(s);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec1<T, P> & tvec1<T, P>::operator/=
	(
		tvec1<U, P> const & v
	)
	{
		this->x /= static_cast<T>(v.x);
		return *this;
	}

	//////////////////////////////////////
	// Increment and decrement operators

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> & tvec1<T, P>::operator++()
	{
		++this->x;
		return *this;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> & tvec1<T, P>::operator--()
	{
		--this->x;
		return *this;
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec1<T, P> tvec1<T, P>::operator++(int)
	{
		tvec1<T, P> Result(*this);
		++*this;
		return Result;
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec1<T, P> tvec1<T, P>::operator--(int)
	{
		tvec1<T, P> Result(*this);
		--*this;
		return Result;
	}

	//////////////////////////////////////
	// Boolean operators

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER bool operator==
	(
		tvec1<T, P> const & v1,
		tvec1<T, P> const & v2
	)
	{
		return (v1.x == v2.x);
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER bool operator!=
	(
		tvec1<T, P> const & v1,
		tvec1<T, P> const & v2
	)
	{
		return (v1.x != v2.x);
	}

	//////////////////////////////////////
	// Unary bit operators

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec1<T, P> & tvec1<T, P>::operator%=
	(
		U const & s
	)
	{
		this->x %= static_cast<T>(s);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec1<T, P> & tvec1<T, P>::operator%=
	(
		tvec1<U, P> const & v
	)
	{
		this->x %= static_cast<T>(v.x);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec1<T, P> & tvec1<T, P>::operator&=
	(
		U const & s
	)
	{
		this->x &= static_cast<T>(s);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec1<T, P> & tvec1<T, P>::operator&=
	(
		tvec1<U, P> const & v
	)
	{
		this->x &= static_cast<T>(v.x);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec1<T, P> & tvec1<T, P>::operator|=
	(
		U const & s
	)
	{
		this->x |= static_cast<T>(s);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec1<T, P> & tvec1<T, P>::operator|=
	(
		tvec1<U, P> const & v
	)
	{
		this->x |= U(v.x);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec1<T, P> & tvec1<T, P>::operator^=
	(
		U const & s
	)
	{
		this->x ^= static_cast<T>(s);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec1<T, P> & tvec1<T, P>::operator^=
	(
		tvec1<U, P> const & v
	)
	{
		this->x ^= static_cast<T>(v.x);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec1<T, P> & tvec1<T, P>::operator<<=
	(
		U const & s
	)
	{
		this->x <<= static_cast<T>(s);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec1<T, P> & tvec1<T, P>::operator<<=
	(
		tvec1<U, P> const & v
	)
	{
		this->x <<= static_cast<T>(v.x);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec1<T, P> & tvec1<T, P>::operator>>=
	(
		U const & s
	)
	{
		this->x >>= static_cast<T>(s);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec1<T, P> & tvec1<T, P>::operator>>=
	(
		tvec1<U, P> const & v
	)
	{
		this->x >>= static_cast<T>(v.x);
		return *this;
	}

	//////////////////////////////////////
	// Binary arithmetic operators

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec1<T, P> operator+
	(
		tvec1<T, P> const & v,
		T const & s
	)
	{
		return tvec1<T, P>(
			v.x + s);
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec1<T, P> operator+ 
	(
		T const & s,
		tvec1<T, P> const & v
	)
	{
		return tvec1<T, P>(
			s + v.x);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator+
	(
		tvec1<T, P> const & v1,
		tvec1<T, P> const & v2
	)
	{
		return tvec1<T, P>(
			v1.x + v2.x);
	}

	//operator-
	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec1<T, P> operator-
	(
		tvec1<T, P> const & v, 
		T const & s
	)
	{
		return tvec1<T, P>(
			v.x - s);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator-
	(
		T const & s,
		tvec1<T, P> const & v
	)
	{
		return tvec1<T, P>(
			s - v.x);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator-
	(
		tvec1<T, P> const & v1,
		tvec1<T, P> const & v2
	)
	{
		return tvec1<T, P>(
			v1.x - v2.x);
	}

	//operator*
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator*
	(
		tvec1<T, P> const & v,
		T const & s
	)
	{
		return tvec1<T, P>(
			v.x * s);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator*
	(
		T const & s,
		tvec1<T, P> const & v
	)
	{
		return tvec1<T, P>(
			s * v.x);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator*
	(
		tvec1<T, P> const & v1,
		tvec1<T, P> const & v2
	)
	{
		return tvec1<T, P>(
			v1.x * v2.x);
	}

	//operator/
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator/
	(
		tvec1<T, P> const & v,
		T const & s
	)
	{
		return tvec1<T, P>(
			v.x / s);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator/
	(
		T const & s,
		tvec1<T, P> const & v
	)
	{
		return tvec1<T, P>(
			s / v.x);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator/
	(
		tvec1<T, P> const & v1,
		tvec1<T, P> const & v2
	)
	{
		return tvec1<T, P>(
			v1.x / v2.x);
	}

	// Unary constant operators
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator-
	(
		tvec1<T, P> const & v
	)
	{
		return tvec1<T, P>(
			-v.x);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator++
	(
		tvec1<T, P> const & v,
		int
	)
	{
		return tvec1<T, P>(
			v.x + T(1));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator--
	(
		tvec1<T, P> const & v,
		int
	)
	{
		return tvec1<T, P>(
			v.x - T(1));
	}

	//////////////////////////////////////
	// Binary bit operators

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator%
	(
		tvec1<T, P> const & v,
		T const & s
	)
	{
		return tvec1<T, P>(
			v.x % s);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator%
	(
		T const & s,
		tvec1<T, P> const & v
	)
	{
		return tvec1<T, P>(
			s % v.x);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator%
	(
		tvec1<T, P> const & v1,
		tvec1<T, P> const & v2
	)
	{
		return tvec1<T, P>(
			v1.x % v2.x);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator&
	(
		tvec1<T, P> const & v, 
		T const & s
	)
	{
		return tvec1<T, P>(
			v.x & s);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator&
	(
		T const & s,
		tvec1<T, P> const & v
	)
	{
		return tvec1<T, P>(
			s & v.x);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator&
	(
		tvec1<T, P> const & v1,
		tvec1<T, P> const & v2
	)
	{
		return tvec1<T, P>(
			v1.x & v2.x);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator|
	(
		tvec1<T, P> const & v,
		T const & s
	)
	{
		return tvec1<T, P>(
			v.x | s);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator|
	(
		T const & s,
		tvec1<T, P> const & v
	)
	{
		return tvec1<T, P>(
			s | v.x);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator|
	(
		tvec1<T, P> const & v1,
		tvec1<T, P> const & v2
	)
	{
		return tvec1<T, P>(
			v1.x | v2.x);
	}
		
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator^
	(
		tvec1<T, P> const & v,
		T const & s
	)
	{
		return tvec1<T, P>(
			v.x ^ s);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator^
	(
		T const & s,
		tvec1<T, P> const & v
	)
	{
		return tvec1<T, P>(
			s ^ v.x);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator^
	(
		tvec1<T, P> const & v1,
		tvec1<T, P> const & v2
	)
	{
		return tvec1<T, P>(
			v1.x ^ v2.x);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator<<
	(
		tvec1<T, P> const & v, 
		T const & s
	)
	{
		return tvec1<T, P>(
			v.x << s);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator<<
	(
		T const & s,
		tvec1<T, P> const & v
	)
	{
		return tvec1<T, P>(
			s << v.x);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator<<
	(
		tvec1<T, P> const & v1,
		tvec1<T, P> const & v2
	)
	{
		return tvec1<T, P>(
			v1.x << v2.x);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator>>
	(
		tvec1<T, P> const & v,
		T const & s
	)
	{
		return tvec1<T, P>(
			v.x >> s);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator>>
	(
		T const & s,
		tvec1<T, P> const & v
	)
	{
		return tvec1<T, P>(
			s >> v.x);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator>>
	(
		tvec1<T, P> const & v1,
		tvec1<T, P> const & v2
	)
	{
		return tvec1<T, P>(
			v1.x >> v2.x);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec1<T, P> operator~
	(
		tvec1<T, P> const & v
	)
	{
		return tvec1<T, P>(
			~v.x);
	}

}//namespace detail
}//namespace glm
