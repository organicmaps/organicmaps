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
/// @file glm/core/type_tvec3.inl
/// @date 2008-08-22 / 2011-06-15
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

namespace glm{
namespace detail
{
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR length_t tvec3<T, P>::length() const
	{
		return 3;
	}

	//////////////////////////////////////
	// Accesses

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER T & tvec3<T, P>::operator[](length_t i)
	{
		assert(i >= 0 && i < this->length());
		return (&x)[i];
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER T const & tvec3<T, P>::operator[](length_t i) const
	{
		assert(i >= 0 && i < this->length());
		return (&x)[i];
	}

	//////////////////////////////////////
	// Implicit basic constructors

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P>::tvec3() :
		x(0),
		y(0),
		z(0)
	{}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P>::tvec3(tvec3<T, P> const & v) :
		x(v.x),
		y(v.y),
		z(v.z)
	{}

	template <typename T, precision P>
	template <precision Q>
	GLM_FUNC_QUALIFIER tvec3<T, P>::tvec3(tvec3<T, Q> const & v) :
		x(v.x),
		y(v.y),
		z(v.z)
	{}

	//////////////////////////////////////
	// Explicit basic constructors

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P>::tvec3(ctor)
	{}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P>::tvec3(T const & s) :
		x(s),
		y(s),
		z(s)
	{}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P>::tvec3
	(
		T const & s0,
		T const & s1,
		T const & s2
	) :
		x(s0),
		y(s1),
		z(s2)
	{}

	//////////////////////////////////////
	// Conversion scalar constructors

	template <typename T, precision P>
	template <typename A, typename B, typename C>
	GLM_FUNC_QUALIFIER tvec3<T, P>::tvec3
	(
		A const & x,
		B const & y,
		C const & z
	) :
		x(static_cast<T>(x)),
		y(static_cast<T>(y)),
		z(static_cast<T>(z))
	{}

	//////////////////////////////////////
	// Conversion vector constructors

	template <typename T, precision P>
	template <typename A, typename B, precision Q>
	GLM_FUNC_QUALIFIER tvec3<T, P>::tvec3
	(
		tvec2<A, Q> const & v,
		B const & s
	) :
		x(static_cast<T>(v.x)),
		y(static_cast<T>(v.y)),
		z(static_cast<T>(s))
	{}

	template <typename T, precision P>
	template <typename A, typename B, precision Q>
	GLM_FUNC_QUALIFIER tvec3<T, P>::tvec3
	(	
		A const & s,
		tvec2<B, Q> const & v
	) :
		x(static_cast<T>(s)),
		y(static_cast<T>(v.x)),
		z(static_cast<T>(v.y))
	{}

	template <typename T, precision P>
	template <typename U, precision Q>
	GLM_FUNC_QUALIFIER tvec3<T, P>::tvec3
	(
		tvec3<U, Q> const & v
	) :
		x(static_cast<T>(v.x)),
		y(static_cast<T>(v.y)),
		z(static_cast<T>(v.z))
	{}

	template <typename T, precision P>
	template <typename U, precision Q>
	GLM_FUNC_QUALIFIER tvec3<T, P>::tvec3
	(
		tvec4<U, Q> const & v
	) :
		x(static_cast<T>(v.x)),
		y(static_cast<T>(v.y)),
		z(static_cast<T>(v.z))
	{}

	//////////////////////////////////////
	// Unary arithmetic operators

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P>& tvec3<T, P>::operator= (tvec3<T, P> const & v)
	{
		this->x = v.x;
		this->y = v.y;
		this->z = v.z;
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P>& tvec3<T, P>::operator= (tvec3<U, P> const & v)
	{
		this->x = static_cast<T>(v.x);
		this->y = static_cast<T>(v.y);
		this->z = static_cast<T>(v.z);
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator+= (U s)
	{
		this->x += static_cast<T>(s);
		this->y += static_cast<T>(s);
		this->z += static_cast<T>(s);
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator+= (tvec3<U, P> const & v)
	{
		this->x += static_cast<T>(v.x);
		this->y += static_cast<T>(v.y);
		this->z += static_cast<T>(v.z);
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator-= (U s)
	{
		this->x -= static_cast<T>(s);
		this->y -= static_cast<T>(s);
		this->z -= static_cast<T>(s);
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator-= (tvec3<U, P> const & v)
	{
		this->x -= static_cast<T>(v.x);
		this->y -= static_cast<T>(v.y);
		this->z -= static_cast<T>(v.z);
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator*= (U s)
	{
		this->x *= static_cast<T>(s);
		this->y *= static_cast<T>(s);
		this->z *= static_cast<T>(s);
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator*= (tvec3<U, P> const & v)
	{
		this->x *= static_cast<T>(v.x);
		this->y *= static_cast<T>(v.y);
		this->z *= static_cast<T>(v.z);
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator/= (U s)
	{
		this->x /= static_cast<T>(s);
		this->y /= static_cast<T>(s);
		this->z /= static_cast<T>(s);
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator/= (tvec3<U, P> const & v)
	{
		this->x /= static_cast<T>(v.x);
		this->y /= static_cast<T>(v.y);
		this->z /= static_cast<T>(v.z);
		return *this;
	}

	//////////////////////////////////////
	// Increment and decrement operators

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator++()
	{
		++this->x;
		++this->y;
		++this->z;
		return *this;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator--()
	{
		--this->x;
		--this->y;
		--this->z;
		return *this;
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec3<T, P> tvec3<T, P>::operator++(int)
	{
		tvec3<T, P> Result(*this);
		++*this;
		return Result;
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec3<T, P> tvec3<T, P>::operator--(int)
	{
		tvec3<T, P> Result(*this);
		--*this;
		return Result;
	}

	//////////////////////////////////////
	// Boolean operators

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER bool operator==
	(
		tvec3<T, P> const & v1,
		tvec3<T, P> const & v2
	)
	{
		return (v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER bool operator!=
	(
		tvec3<T, P> const & v1,
		tvec3<T, P> const & v2
	)
	{
		return (v1.x != v2.x) || (v1.y != v2.y) || (v1.z != v2.z);
	}

	//////////////////////////////////////
	// Unary bit operators

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator%= (U s)
	{
		this->x %= s;
		this->y %= s;
		this->z %= s;
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator%= (tvec3<U, P> const & v)
	{
		this->x %= v.x;
		this->y %= v.y;
		this->z %= v.z;
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator&= (U s)
	{
		this->x &= s;
		this->y &= s;
		this->z &= s;
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator&= (tvec3<U, P> const & v)
	{
		this->x &= v.x;
		this->y &= v.y;
		this->z &= v.z;
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator|= (U s)
	{
		this->x |= s;
		this->y |= s;
		this->z |= s;
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator|= (tvec3<U, P> const & v)
	{
		this->x |= v.x;
		this->y |= v.y;
		this->z |= v.z;
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator^= (U s)
	{
		this->x ^= s;
		this->y ^= s;
		this->z ^= s;
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator^= (tvec3<U, P> const & v)
	{
		this->x ^= v.x;
		this->y ^= v.y;
		this->z ^= v.z;
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator<<= (U s)
	{
		this->x <<= s;
		this->y <<= s;
		this->z <<= s;
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator<<= (tvec3<U, P> const & v)
	{
		this->x <<= static_cast<T>(v.x);
		this->y <<= static_cast<T>(v.y);
		this->z <<= static_cast<T>(v.z);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator>>= (U s)
	{
		this->x >>= static_cast<T>(s);
		this->y >>= static_cast<T>(s);
		this->z >>= static_cast<T>(s);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator>>= (tvec3<U, P> const & v)
	{
		this->x >>= static_cast<T>(v.x);
		this->y >>= static_cast<T>(v.y);
		this->z >>= static_cast<T>(v.z);
		return *this;
	}

	//////////////////////////////////////
	// Binary arithmetic operators

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec3<T, P> operator+
	(
		tvec3<T, P> const & v, 
		T const & s
	)
	{
		return tvec3<T, P>(
			v.x + s,
			v.y + s,
			v.z + s);
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec3<T, P> operator+ 
	(
		T const & s, 
		tvec3<T, P> const & v
	)
	{
		return tvec3<T, P>(
			s + v.x,
			s + v.y,
			s + v.z);
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec3<T, P> operator+ 
	(
		tvec3<T, P> const & v1, 
		tvec3<T, P> const & v2
	)
	{
		return tvec3<T, P>(
			v1.x + v2.x,
			v1.y + v2.y,
			v1.z + v2.z);
	}

	//operator-
	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec3<T, P> operator- 
	(
		tvec3<T, P> const & v, 
		T const & s
	)
	{
		return tvec3<T, P>(
			v.x - s,
			v.y - s,
			v.z - s);
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec3<T, P> operator- 
	(
		T const & s, 
		tvec3<T, P> const & v
	)
	{
		return tvec3<T, P>(
			s - v.x,
			s - v.y,
			s - v.z);
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec3<T, P> operator- 
	(
		tvec3<T, P> const & v1, 
		tvec3<T, P> const & v2
	)
	{
		return tvec3<T, P>(
			v1.x - v2.x,
			v1.y - v2.y,
			v1.z - v2.z);
	}

	//operator*
	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec3<T, P> operator*
	(
		tvec3<T, P> const & v, 
		T const & s
	)
	{
		return tvec3<T, P>(
			v.x * s,
			v.y * s,
			v.z * s);
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec3<T, P> operator* 
	(
		T const & s, 
		tvec3<T, P> const & v
	)
	{
		return tvec3<T, P>(
			s * v.x,
			s * v.y,
			s * v.z);
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec3<T, P> operator* 
	(
		tvec3<T, P> const & v1, 
		tvec3<T, P> const & v2
	)
	{
		return tvec3<T, P>(
			v1.x * v2.x,
			v1.y * v2.y,
			v1.z * v2.z);
	}

	//operator/
	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec3<T, P> operator/
	(
		tvec3<T, P> const & v, 
		T const & s
	)
	{
		return tvec3<T, P>(
			v.x / s,
			v.y / s,
			v.z / s);
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec3<T, P> operator/ 
	(
		T const & s, 
		tvec3<T, P> const & v
	)
	{
		return tvec3<T, P>(
			s / v.x,
			s / v.y,
			s / v.z);
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec3<T, P> operator/ 
	(
		tvec3<T, P> const & v1, 
		tvec3<T, P> const & v2
	)
	{
		return tvec3<T, P>(
			v1.x / v2.x,
			v1.y / v2.y,
			v1.z / v2.z);
	}

	// Unary constant operators
	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec3<T, P> operator- 
	(
		tvec3<T, P> const & v
	)
	{
		return tvec3<T, P>(
			-v.x, 
			-v.y, 
			-v.z);
	}

	//////////////////////////////////////
	// Binary bit operators

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator% 
	(
		tvec3<T, P> const & v, 
		T const & s
	)
	{
		return tvec3<T, P>(
			v.x % s,
			v.y % s,
			v.z % s);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator%
	(
		T const & s, 
		tvec3<T, P> const & v
	)
	{
		return tvec3<T, P>(
			s % v.x,
			s % v.y,
			s % v.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator% 
	(
		tvec3<T, P> const & v1, 
		tvec3<T, P> const & v2
	)
	{
		return tvec3<T, P>(
			v1.x % v2.x,
			v1.y % v2.y,
			v1.z % v2.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator& 
	(
		tvec3<T, P> const & v, 
		T const & s
	)
	{
		return tvec3<T, P>(
			v.x & s,
			v.y & s,
			v.z & s);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator& 
	(
		T const & s, 
		tvec3<T, P> const & v
	)
	{
		return tvec3<T, P>(
			s & v.x,
			s & v.y,
			s & v.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator& 
	(
		tvec3<T, P> const & v1, 
		tvec3<T, P> const & v2
	)
	{
		return tvec3<T, P>(
			v1.x & v2.x,
			v1.y & v2.y,
			v1.z & v2.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator| 
	(
		tvec3<T, P> const & v, 
		T const & s
	)
	{
		return tvec3<T, P>(
			v.x | s,
			v.y | s,
			v.z | s);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator| 
	(
		T const & s, 
		tvec3<T, P> const & v
	)
	{
		return tvec3<T, P>(
			s | v.x,
			s | v.y,
			s | v.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator| 
	(
		tvec3<T, P> const & v1, 
		tvec3<T, P> const & v2
	)
	{
		return tvec3<T, P>(
			v1.x | v2.x,
			v1.y | v2.y,
			v1.z | v2.z);
	}
		
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator^ 
	(
		tvec3<T, P> const & v, 
		T const & s
	)
	{
		return tvec3<T, P>(
			v.x ^ s,
			v.y ^ s,
			v.z ^ s);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator^ 
	(
		T const & s, 
		tvec3<T, P> const & v
	)
	{
		return tvec3<T, P>(
			T(s) ^ v.x,
			T(s) ^ v.y,
			T(s) ^ v.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator^ 
	(
		tvec3<T, P> const & v1, 
		tvec3<T, P> const & v2
	)
	{
		return tvec3<T, P>(
			v1.x ^ T(v2.x),
			v1.y ^ T(v2.y),
			v1.z ^ T(v2.z));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator<< 
	(
		tvec3<T, P> const & v, 
		T const & s
	)
	{
		return tvec3<T, P>(
			v.x << T(s),
			v.y << T(s),
			v.z << T(s));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator<< 
	(
		T const & s, 
		tvec3<T, P> const & v
	)
	{
		return tvec3<T, P>(
			T(s) << v.x,
			T(s) << v.y,
			T(s) << v.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator<< 
	(
		tvec3<T, P> const & v1, 
		tvec3<T, P> const & v2
	)
	{
		return tvec3<T, P>(
			v1.x << T(v2.x),
			v1.y << T(v2.y),
			v1.z << T(v2.z));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator>> 
	(
		tvec3<T, P> const & v, 
		T const & s
	)
	{
		return tvec3<T, P>(
			v.x >> T(s),
			v.y >> T(s),
			v.z >> T(s));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator>> 
	(
		T const & s, 
		tvec3<T, P> const & v
	)
	{
		return tvec3<T, P>(
			s >> T(v.x),
			s >> T(v.y),
			s >> T(v.z));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator>> 
	(
		tvec3<T, P> const & v1, 
		tvec3<T, P> const & v2
	)
	{
		return tvec3<T, P>(
			v1.x >> T(v2.x),
			v1.y >> T(v2.y),
			v1.z >> T(v2.z));
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec3<T, P> operator~ 
	(
		tvec3<T, P> const & v
	)
	{
		return tvec3<T, P>(
			~v.x,
			~v.y,
			~v.z);
	}

}//namespace detail
}//namespace glm
