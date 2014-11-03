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
/// @file glm/core/type_tvec4.inl
/// @date 2008-08-23 / 2011-06-15
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

namespace glm{
namespace detail
{
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR length_t tvec4<T, P>::length() const
	{
		return 4;
	}

	//////////////////////////////////////
	// Accesses

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER T & tvec4<T, P>::operator[](length_t i)
	{
		assert(i >= 0 && i < this->length());
		return (&x)[i];
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER T const & tvec4<T, P>::operator[](length_t i) const
	{
		assert(i >= 0 && i < this->length());
		return (&x)[i];
	}

	//////////////////////////////////////
	// Implicit basic constructors

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec4<T, P>::tvec4() :
		x(0),
		y(0),
		z(0),
		w(0)
	{}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec4<T, P>::tvec4(tvec4<T, P> const & v) :
		x(v.x),
		y(v.y),
		z(v.z),
		w(v.w)
	{}

	template <typename T, precision P>
	template <precision Q>
	GLM_FUNC_QUALIFIER tvec4<T, P>::tvec4(tvec4<T, Q> const & v) :
		x(v.x),
		y(v.y),
		z(v.z),
		w(v.w)
	{}

	//////////////////////////////////////
	// Explicit basic constructors

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec4<T, P>::tvec4(ctor)
	{}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec4<T, P>::tvec4(T const & s) :
		x(s),
		y(s),
		z(s),
		w(s)
	{}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec4<T, P>::tvec4
	(
		T const & s1,
		T const & s2,
		T const & s3,
		T const & s4
	) :
		x(s1),
		y(s2),
		z(s3),
		w(s4)
	{}

	//////////////////////////////////////
	// Conversion scalar constructors

	template <typename T, precision P>
	template <typename A, typename B, typename C, typename D>
	GLM_FUNC_QUALIFIER tvec4<T, P>::tvec4
	(
		A const & x,
		B const & y,
		C const & z,
		D const & w
	) :
		x(static_cast<T>(x)),
		y(static_cast<T>(y)),
		z(static_cast<T>(z)),
		w(static_cast<T>(w))
	{}

	template <typename T, precision P>
	template <typename U, precision Q>
	GLM_FUNC_QUALIFIER tvec4<T, P>::tvec4
	(
		tvec4<U, Q> const & v
	) :
		x(static_cast<T>(v.x)),
		y(static_cast<T>(v.y)),
		z(static_cast<T>(v.z)),
		w(static_cast<T>(v.w))
	{}

	//////////////////////////////////////
	// Conversion vector constructors

	template <typename T, precision P>
	template <typename A, typename B, typename C, precision Q>
	GLM_FUNC_QUALIFIER tvec4<T, P>::tvec4
	(
		tvec2<A, Q> const & v,
		B const & s1,
		C const & s2
	) :
		x(static_cast<T>(v.x)),
		y(static_cast<T>(v.y)),
		z(static_cast<T>(s1)),
		w(static_cast<T>(s2))
	{}

	template <typename T, precision P>
	template <typename A, typename B, typename C, precision Q>
	GLM_FUNC_QUALIFIER tvec4<T, P>::tvec4
	(
		A const & s1,
		tvec2<B, Q> const & v,
		C const & s2
	) :
		x(static_cast<T>(s1)),
		y(static_cast<T>(v.x)),
		z(static_cast<T>(v.y)),
		w(static_cast<T>(s2))
	{}

	template <typename T, precision P>
	template <typename A, typename B, typename C, precision Q>
	GLM_FUNC_QUALIFIER tvec4<T, P>::tvec4
	(
		A const & s1,
		B const & s2,
		tvec2<C, Q> const & v
	) :
		x(static_cast<T>(s1)),
		y(static_cast<T>(s2)),
		z(static_cast<T>(v.x)),
		w(static_cast<T>(v.y))
	{}

	template <typename T, precision P>
	template <typename A, typename B, precision Q>
	GLM_FUNC_QUALIFIER tvec4<T, P>::tvec4
	(
		tvec3<A, Q> const & v,
		B const & s
	) :
		x(static_cast<T>(v.x)),
		y(static_cast<T>(v.y)),
		z(static_cast<T>(v.z)),
		w(static_cast<T>(s))
	{}

	template <typename T, precision P>
	template <typename A, typename B, precision Q>
	GLM_FUNC_QUALIFIER tvec4<T, P>::tvec4
	(
		A const & s,
		tvec3<B, Q> const & v
	) :
		x(static_cast<T>(s)),
		y(static_cast<T>(v.x)),
		z(static_cast<T>(v.y)),
		w(static_cast<T>(v.z))
	{}

	template <typename T, precision P>
	template <typename A, typename B, precision Q>
	GLM_FUNC_QUALIFIER tvec4<T, P>::tvec4
	(
		tvec2<A, Q> const & v1,
		tvec2<B, Q> const & v2
	) :
		x(static_cast<T>(v1.x)),
		y(static_cast<T>(v1.y)),
		z(static_cast<T>(v2.x)),
		w(static_cast<T>(v2.y))
	{}

	//////////////////////////////////////
	// Unary arithmetic operators

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec4<T, P> & tvec4<T, P>::operator= (tvec4<T, P> const & v)
	{
		this->x = v.x;
		this->y = v.y;
		this->z = v.z;
		this->w = v.w;
		return *this;
	}

	template <typename T, precision P>
	template <typename U, precision Q>
	GLM_FUNC_QUALIFIER tvec4<T, P> & tvec4<T, P>::operator= (tvec4<U, Q> const & v)
	{
		this->x = static_cast<T>(v.x);
		this->y = static_cast<T>(v.y);
		this->z = static_cast<T>(v.z);
		this->w = static_cast<T>(v.w);
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec4<T, P> & tvec4<T, P>::operator+= (U s)
	{
		this->x += static_cast<T>(s);
		this->y += static_cast<T>(s);
		this->z += static_cast<T>(s);
		this->w += static_cast<T>(s);
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec4<T, P> & tvec4<T, P>::operator+= (tvec4<U, P> const & v)
	{
		this->x += static_cast<T>(v.x);
		this->y += static_cast<T>(v.y);
		this->z += static_cast<T>(v.z);
		this->w += static_cast<T>(v.w);
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec4<T, P> & tvec4<T, P>::operator-= (U s)
	{
		this->x -= static_cast<T>(s);
		this->y -= static_cast<T>(s);
		this->z -= static_cast<T>(s);
		this->w -= static_cast<T>(s);
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec4<T, P> & tvec4<T, P>::operator-= (tvec4<U, P> const & v)
	{
		this->x -= static_cast<T>(v.x);
		this->y -= static_cast<T>(v.y);
		this->z -= static_cast<T>(v.z);
		this->w -= static_cast<T>(v.w);
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec4<T, P> & tvec4<T, P>::operator*= (U s)
	{
		this->x *= static_cast<T>(s);
		this->y *= static_cast<T>(s);
		this->z *= static_cast<T>(s);
		this->w *= static_cast<T>(s);
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec4<T, P> & tvec4<T, P>::operator*= (tvec4<U, P> const & v)
	{
		this->x *= static_cast<T>(v.x);
		this->y *= static_cast<T>(v.y);
		this->z *= static_cast<T>(v.z);
		this->w *= static_cast<T>(v.w);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec4<T, P> & tvec4<T, P>::operator/= (U s)
	{
		this->x /= static_cast<T>(s);
		this->y /= static_cast<T>(s);
		this->z /= static_cast<T>(s);
		this->w /= static_cast<T>(s);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec4<T, P> & tvec4<T, P>::operator/= (tvec4<U, P> const & v)
	{
		this->x /= static_cast<T>(v.x);
		this->y /= static_cast<T>(v.y);
		this->z /= static_cast<T>(v.z);
		this->w /= static_cast<T>(v.w);
		return *this;
	}

	//////////////////////////////////////
	// Increment and decrement operators

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec4<T, P> & tvec4<T, P>::operator++()
	{
		++this->x;
		++this->y;
		++this->z;
		++this->w;
		return *this;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec4<T, P> & tvec4<T, P>::operator--()
	{
		--this->x;
		--this->y;
		--this->z;
		--this->w;
		return *this;
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec4<T, P> tvec4<T, P>::operator++(int)
	{
		tvec4<T, P> Result(*this);
		++*this;
		return Result;
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec4<T, P> tvec4<T, P>::operator--(int)
	{
		tvec4<T, P> Result(*this);
		--*this;
		return Result;
	}

	//////////////////////////////////////
	// Unary bit operators

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec4<T, P> & tvec4<T, P>::operator%= (U s)
	{
		this->x %= static_cast<T>(s);
		this->y %= static_cast<T>(s);
		this->z %= static_cast<T>(s);
		this->w %= static_cast<T>(s);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec4<T, P> & tvec4<T, P>::operator%= (tvec4<U, P> const & v)
	{
		this->x %= static_cast<T>(v.x);
		this->y %= static_cast<T>(v.y);
		this->z %= static_cast<T>(v.z);
		this->w %= static_cast<T>(v.w);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec4<T, P> & tvec4<T, P>::operator&= (U s)
	{
		this->x &= static_cast<T>(s);
		this->y &= static_cast<T>(s);
		this->z &= static_cast<T>(s);
		this->w &= static_cast<T>(s);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec4<T, P> & tvec4<T, P>::operator&= (tvec4<U, P> const & v)
	{
		this->x &= static_cast<T>(v.x);
		this->y &= static_cast<T>(v.y);
		this->z &= static_cast<T>(v.z);
		this->w &= static_cast<T>(v.w);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec4<T, P> & tvec4<T, P>::operator|= (U s)
	{
		this->x |= static_cast<T>(s);
		this->y |= static_cast<T>(s);
		this->z |= static_cast<T>(s);
		this->w |= static_cast<T>(s);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec4<T, P> & tvec4<T, P>::operator|= (tvec4<U, P> const & v)
	{
		this->x |= static_cast<T>(v.x);
		this->y |= static_cast<T>(v.y);
		this->z |= static_cast<T>(v.z);
		this->w |= static_cast<T>(v.w);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec4<T, P> & tvec4<T, P>::operator^= (U s)
	{
		this->x ^= static_cast<T>(s);
		this->y ^= static_cast<T>(s);
		this->z ^= static_cast<T>(s);
		this->w ^= static_cast<T>(s);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec4<T, P> & tvec4<T, P>::operator^= (tvec4<U, P> const & v)
	{
		this->x ^= static_cast<T>(v.x);
		this->y ^= static_cast<T>(v.y);
		this->z ^= static_cast<T>(v.z);
		this->w ^= static_cast<T>(v.w);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec4<T, P> & tvec4<T, P>::operator<<= (U s)
	{
		this->x <<= static_cast<T>(s);
		this->y <<= static_cast<T>(s);
		this->z <<= static_cast<T>(s);
		this->w <<= static_cast<T>(s);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec4<T, P> & tvec4<T, P>::operator<<= (tvec4<U, P> const & v)
	{
		this->x <<= static_cast<T>(v.x);
		this->y <<= static_cast<T>(v.y);
		this->z <<= static_cast<T>(v.z);
		this->w <<= static_cast<T>(v.w);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec4<T, P> & tvec4<T, P>::operator>>= (U s)
	{
		this->x >>= static_cast<T>(s);
		this->y >>= static_cast<T>(s);
		this->z >>= static_cast<T>(s);
		this->w >>= static_cast<T>(s);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec4<T, P> & tvec4<T, P>::operator>>= (tvec4<U, P> const & v)
	{
		this->x >>= static_cast<T>(v.x);
		this->y >>= static_cast<T>(v.y);
		this->z >>= static_cast<T>(v.z);
		this->w >>= static_cast<T>(v.w);
		return *this;
	}

	//////////////////////////////////////
	// Binary arithmetic operators

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec4<T, P> operator+ 
	(
		tvec4<T, P> const & v, 
		T const & s
	)
	{
		return tvec4<T, P>(
			v.x + s,
			v.y + s,
			v.z + s,
			v.w + s);
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec4<T, P> operator+ 
	(
		T const & s, 
		tvec4<T, P> const & v
	)
	{
		return tvec4<T, P>(
			s + v.x,
			s + v.y,
			s + v.z,
			s + v.w);
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec4<T, P> operator+ 
	(
		tvec4<T, P> const & v1, 
		tvec4<T, P> const & v2
	)
	{
		return tvec4<T, P>(
			v1.x + v2.x,
			v1.y + v2.y,
			v1.z + v2.z,
			v1.w + v2.w);
	}

	//operator-
	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec4<T, P> operator- 
	(
		tvec4<T, P> const & v, 
		T const & s
	)
	{
		return tvec4<T, P>(
			v.x - s,
			v.y - s,
			v.z - s,
			v.w - s);
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec4<T, P> operator- 
	(
		T const & s, 
		tvec4<T, P> const & v
	)
	{
		return tvec4<T, P>(
			s - v.x,
			s - v.y,
			s - v.z,
			s - v.w);
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec4<T, P> operator- 
	(
		tvec4<T, P> const & v1, 
		tvec4<T, P> const & v2
	)
	{
		return tvec4<T, P>(
			v1.x - v2.x,
			v1.y - v2.y,
			v1.z - v2.z,
			v1.w - v2.w);
	}

	//operator*
	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec4<T, P> operator* 
	(
		tvec4<T, P> const & v, 
		T const & s
	)
	{
		return tvec4<T, P>(
			v.x * s,
			v.y * s,
			v.z * s,
			v.w * s);
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec4<T, P> operator* 
	(
		T const & s, 
		tvec4<T, P> const & v
	)
	{
		return tvec4<T, P>(
			s * v.x,
			s * v.y,
			s * v.z,
			s * v.w);
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec4<T, P> operator*
	(
		tvec4<T, P> const & v1, 
		tvec4<T, P> const & v2
	)
	{
		return tvec4<T, P>(
			v1.x * v2.x,
			v1.y * v2.y,
			v1.z * v2.z,
			v1.w * v2.w);
	}

	//operator/
	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec4<T, P> operator/ 
	(
		tvec4<T, P> const & v, 
		T const & s
	)
	{
		return tvec4<T, P>(
			v.x / s,
			v.y / s,
			v.z / s,
			v.w / s);
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec4<T, P> operator/ 
	(
		T const & s, 
		tvec4<T, P> const & v
	)
	{
		return tvec4<T, P>(
			s / v.x,
			s / v.y,
			s / v.z,
			s / v.w);
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec4<T, P> operator/ 
	(
		tvec4<T, P> const & v1, 
		tvec4<T, P> const & v2
	)
	{
		return tvec4<T, P>(
			v1.x / v2.x,
			v1.y / v2.y,
			v1.z / v2.z,
			v1.w / v2.w);
	}

	// Unary constant operators
	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec4<T, P> operator- 
	(
		tvec4<T, P> const & v
	)
	{
		return tvec4<T, P>(
			-v.x, 
			-v.y, 
			-v.z, 
			-v.w);
	}

	//////////////////////////////////////
	// Boolean operators

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER bool operator==
	(
		tvec4<T, P> const & v1, 
		tvec4<T, P> const & v2
	)
	{
		return (v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z) && (v1.w == v2.w);
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER bool operator!=
	(
		tvec4<T, P> const & v1, 
		tvec4<T, P> const & v2
	)
	{
		return (v1.x != v2.x) || (v1.y != v2.y) || (v1.z != v2.z) || (v1.w != v2.w);
	}

	//////////////////////////////////////
	// Binary bit operators

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec4<T, P> operator% 
	(
		tvec4<T, P> const & v, 
		T const & s
	)
	{
		return tvec4<T, P>(
			v.x % s,
			v.y % s,
			v.z % s,
			v.w % s);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec4<T, P> operator% 
	(
		T const & s, 
		tvec4<T, P> const & v
	)
	{
		return tvec4<T, P>(
			s % v.x,
			s % v.y,
			s % v.z,
			s % v.w);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec4<T, P> operator%
	(
		tvec4<T, P> const & v1, 
		tvec4<T, P> const & v2
	)
	{
		return tvec4<T, P>(
			v1.x % v2.x,
			v1.y % v2.y,
			v1.z % v2.z,
			v1.w % v2.w);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec4<T, P> operator& 
	(
		tvec4<T, P> const & v, 
		T const & s
	)
	{
		return tvec4<T, P>(
			v.x & s,
			v.y & s,
			v.z & s,
			v.w & s);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec4<T, P> operator& 
	(
		T const & s, 
		tvec4<T, P> const & v
	)
	{
		return tvec4<T, P>(
			s & v.x,
			s & v.y,
			s & v.z,
			s & v.w);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec4<T, P> operator&
	(
		tvec4<T, P> const & v1,
		tvec4<T, P> const & v2
	)
	{
		return tvec4<T, P>(
			v1.x & v2.x,
			v1.y & v2.y,
			v1.z & v2.z,
			v1.w & v2.w);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec4<T, P> operator|
	(
		tvec4<T, P> const & v, 
		T const & s
	)
	{
		return tvec4<T, P>(
			v.x | s,
			v.y | s,
			v.z | s,
			v.w | s);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec4<T, P> operator|
	(
		T const & s, 
		tvec4<T, P> const & v
	)
	{
		return tvec4<T, P>(
			s | v.x,
			s | v.y,
			s | v.z,
			s | v.w);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec4<T, P> operator|
	(
		tvec4<T, P> const & v1,
		tvec4<T, P> const & v2
	)
	{
		return tvec4<T, P>(
			v1.x | v2.x,
			v1.y | v2.y,
			v1.z | v2.z,
			v1.w | v2.w);
	}
		
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec4<T, P> operator^
	(
		tvec4<T, P> const & v, 
		T const & s
	)
	{
		return tvec4<T, P>(
			v.x ^ s,
			v.y ^ s,
			v.z ^ s,
			v.w ^ s);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec4<T, P> operator^
	(
		T const & s, 
		tvec4<T, P> const & v
	)
	{
		return tvec4<T, P>(
			s ^ v.x,
			s ^ v.y,
			s ^ v.z,
			s ^ v.w);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec4<T, P> operator^
	(
		tvec4<T, P> const & v1,
		tvec4<T, P> const & v2
	)
	{
		return tvec4<T, P>(
			v1.x ^ v2.x,
			v1.y ^ v2.y,
			v1.z ^ v2.z,
			v1.w ^ v2.w);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec4<T, P> operator<<
	(
		tvec4<T, P> const & v,
		T const & s
	)
	{
		return tvec4<T, P>(
			v.x << s,
			v.y << s,
			v.z << s,
			v.w << s);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec4<T, P> operator<<
	(
		T const & s,
		tvec4<T, P> const & v
	)
	{
		return tvec4<T, P>(
			s << v.x,
			s << v.y,
			s << v.z,
			s << v.w);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec4<T, P> operator<<
	(
		tvec4<T, P> const & v1,
		tvec4<T, P> const & v2
	)
	{
		return tvec4<T, P>(
			v1.x << v2.x,
			v1.y << v2.y,
			v1.z << v2.z,
			v1.w << v2.w);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec4<T, P> operator>>
	(
		tvec4<T, P> const & v,
		T const & s
	)
	{
		return tvec4<T, P>(
			v.x >> s,
			v.y >> s,
			v.z >> s,
			v.w >> s);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec4<T, P> operator>>
	(
		T const & s,
		tvec4<T, P> const & v
	)
	{
		return tvec4<T, P>(
			s >> v.x,
			s >> v.y,
			s >> v.z,
			s >> v.w);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec4<T, P> operator>>
	(
		tvec4<T, P> const & v1,
		tvec4<T, P> const & v2
	)
	{
		return tvec4<T, P>(
			v1.x >> v2.x,
			v1.y >> v2.y,
			v1.z >> v2.z,
			v1.w >> v2.w);
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec4<T, P> operator~
	(
		tvec4<T, P> const & v
	)
	{
		return tvec4<T, P>(
			~v.x,
			~v.y,
			~v.z,
			~v.w);
	}

}//namespace detail
}//namespace glm
