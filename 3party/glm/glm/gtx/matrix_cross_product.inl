///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2005-12-21
// Updated : 2005-12-21
// Licence : This source is under MIT License
// File    : glm/gtx/matrix_cross_product.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace glm
{
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tmat3x3<T, P> matrixCross3
	(
		detail::tvec3<T, P> const & x
	)
	{
		detail::tmat3x3<T, P> Result(T(0));
		Result[0][1] = x.z;
		Result[1][0] = -x.z;
		Result[0][2] = -x.y;
		Result[2][0] = x.y;
		Result[1][2] = x.x;
		Result[2][1] = -x.x;
		return Result;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, P> matrixCross4
	(
		detail::tvec3<T, P> const & x
	)
	{
		detail::tmat4x4<T, P> Result(T(0));
		Result[0][1] = x.z;
		Result[1][0] = -x.z;
		Result[0][2] = -x.y;
		Result[2][0] = x.y;
		Result[1][2] = x.x;
		Result[2][1] = -x.x;
		return Result;
	}

}//namespace glm
