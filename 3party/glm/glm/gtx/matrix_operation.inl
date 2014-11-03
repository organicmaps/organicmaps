///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2009-08-29
// Updated : 2009-08-29
// Licence : This source is under MIT License
// File    : glm/gtx/matrix_operation.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace glm
{
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tmat2x2<T, P> diagonal2x2
	(
		detail::tvec2<T, P> const & v
	)
	{
		detail::tmat2x2<T, P> Result(static_cast<T>(1));
		Result[0][0] = v[0];
		Result[1][1] = v[1];
		return Result;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tmat2x3<T, P> diagonal2x3
	(
		detail::tvec2<T, P> const & v
	)
	{
		detail::tmat2x3<T, P> Result(static_cast<T>(1));
		Result[0][0] = v[0];
		Result[1][1] = v[1];
		return Result;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tmat2x4<T, P> diagonal2x4
	(
		detail::tvec2<T, P> const & v
	)
	{
		detail::tmat2x4<T, P> Result(static_cast<T>(1));
		Result[0][0] = v[0];
		Result[1][1] = v[1];
		return Result;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tmat3x2<T, P> diagonal3x2
	(
		detail::tvec2<T, P> const & v
	)
	{
		detail::tmat3x2<T, P> Result(static_cast<T>(1));
		Result[0][0] = v[0];
		Result[1][1] = v[1];
		return Result;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tmat3x3<T, P> diagonal3x3
	(
		detail::tvec3<T, P> const & v
	)
	{
		detail::tmat3x3<T, P> Result(static_cast<T>(1));
		Result[0][0] = v[0];
		Result[1][1] = v[1];
		Result[2][2] = v[2];
		return Result;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tmat3x4<T, P> diagonal3x4
	(
		detail::tvec3<T, P> const & v
	)
	{
		detail::tmat3x4<T, P> Result(static_cast<T>(1));
		Result[0][0] = v[0];
		Result[1][1] = v[1];
		Result[2][2] = v[2];
		return Result;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, P> diagonal4x4
	(
		detail::tvec4<T, P> const & v
	)
	{
		detail::tmat4x4<T, P> Result(static_cast<T>(1));
		Result[0][0] = v[0];
		Result[1][1] = v[1];
		Result[2][2] = v[2];
		Result[3][3] = v[3];
		return Result;		
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tmat4x3<T, P> diagonal4x3
	(
		detail::tvec3<T, P> const & v
	)
	{
		detail::tmat4x3<T, P> Result(static_cast<T>(1));
		Result[0][0] = v[0];
		Result[1][1] = v[1];
		Result[2][2] = v[2];
		return Result;		
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tmat4x2<T, P> diagonal4x2
	(
		detail::tvec2<T, P> const & v
	)
	{
		detail::tmat4x2<T, P> Result(static_cast<T>(1));
		Result[0][0] = v[0];
		Result[1][1] = v[1];
		return Result;		
	}
}//namespace glm
