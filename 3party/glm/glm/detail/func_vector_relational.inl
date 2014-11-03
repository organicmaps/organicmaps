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
/// @file glm/core/func_vector_relational.inl
/// @date 2008-08-03 / 2011-09-09
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#include <limits>

namespace glm
{
	template <typename T, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER typename vecType<T, P>::bool_type lessThan
	(
		vecType<T, P> const & x,
		vecType<T, P> const & y
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559 || std::numeric_limits<T>::is_integer,
			"Invalid template instantiation of 'lessThan', GLM vector types required floating-point or integer value types vectors");
		assert(x.length() == y.length());

		typename vecType<bool, P>::bool_type Result(vecType<bool, P>::_null);
		for(int i = 0; i < x.length(); ++i)
			Result[i] = x[i] < y[i];

		return Result;
	}

	template <typename T, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER typename vecType<T, P>::bool_type lessThanEqual
	(
		vecType<T, P> const & x,
		vecType<T, P> const & y
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559 || std::numeric_limits<T>::is_integer,
			"Invalid template instantiation of 'lessThanEqual', GLM vector types required floating-point or integer value types vectors");
		assert(x.length() == y.length());

		typename vecType<bool, P>::bool_type Result(vecType<bool, P>::_null);
		for(int i = 0; i < x.length(); ++i)
			Result[i] = x[i] <= y[i];
		return Result;
	}

	template <typename T, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER typename vecType<T, P>::bool_type greaterThan
	(
		vecType<T, P> const & x,
		vecType<T, P> const & y
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559 || std::numeric_limits<T>::is_integer,
			"Invalid template instantiation of 'greaterThan', GLM vector types required floating-point or integer value types vectors");
		assert(x.length() == y.length());

		typename vecType<bool, P>::bool_type Result(vecType<bool, P>::_null);
		for(int i = 0; i < x.length(); ++i)
			Result[i] = x[i] > y[i];
		return Result;
	}

	template <typename T, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER typename vecType<T, P>::bool_type greaterThanEqual
	(
		vecType<T, P> const & x,
		vecType<T, P> const & y
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559 || std::numeric_limits<T>::is_integer,
			"Invalid template instantiation of 'greaterThanEqual', GLM vector types required floating-point or integer value types vectors");
		assert(x.length() == y.length());

		typename vecType<bool, P>::bool_type Result(vecType<bool, P>::_null);
		for(int i = 0; i < x.length(); ++i)
			Result[i] = x[i] >= y[i];
		return Result;
	}

	template <typename T, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER typename vecType<T, P>::bool_type equal
	(
		vecType<T, P> const & x,
		vecType<T, P> const & y
	)
	{
		assert(x.length() == y.length());

		typename vecType<bool, P>::bool_type Result(vecType<bool, P>::_null);
		for(int i = 0; i < x.length(); ++i)
			Result[i] = x[i] == y[i];
		return Result;
	}

	template <typename T, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER typename vecType<T, P>::bool_type notEqual
	(
		vecType<T, P> const & x,
		vecType<T, P> const & y
	)
	{
		assert(x.length() == y.length());

		typename vecType<bool, P>::bool_type Result(vecType<bool, P>::_null);
		for(int i = 0; i < x.length(); ++i)
			Result[i] = x[i] != y[i];
		return Result;
	}

	template <precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER bool any(vecType<bool, P> const & v)
	{
		bool Result = false;
		for(int i = 0; i < v.length(); ++i)
			Result = Result || v[i];
		return Result;
	}

	template <precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER bool all(vecType<bool, P> const & v)
	{
		bool Result = true;
		for(int i = 0; i < v.length(); ++i)
			Result = Result && v[i];
		return Result;
	}

	template <precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<bool, P> not_(vecType<bool, P> const & v)
	{
		typename vecType<bool, P>::bool_type Result(vecType<bool, P>::_null);
		for(int i = 0; i < v.length(); ++i)
			Result[i] = !v[i];
		return Result;
	}
}//namespace glm

