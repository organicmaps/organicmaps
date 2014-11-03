///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2006-04-21
// Updated : 2006-12-06
// Licence : This source is under MIT License
// File    : glm/gtx/inertia.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace glm
{
/*
	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat3x3<T, P> boxInertia3
	(
		T const & Mass, 
		detail::tvec3<T, P> const & Scale
	)
	{
		detail::tmat3x3<T, P> Result(T(1));
		Result[0][0] = (Scale.y * Scale.y + Scale.z * Scale.z) * Mass / T(12);
		Result[1][1] = (Scale.x * Scale.x + Scale.z * Scale.z) * Mass / T(12);
		Result[2][2] = (Scale.x * Scale.x + Scale.y * Scale.y) * Mass / T(12);
		return Result;
	}

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, P> boxInertia4
	(
		T const & Mass, 
		detail::tvec3<T, P> const & Scale
	)
	{
		detail::tmat4x4<T, P> Result(T(1));
		Result[0][0] = (Scale.y * Scale.y + Scale.z * Scale.z) * Mass / T(12);
		Result[1][1] = (Scale.x * Scale.x + Scale.z * Scale.z) * Mass / T(12);
		Result[2][2] = (Scale.x * Scale.x + Scale.y * Scale.y) * Mass / T(12);
		return Result;
	}

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat3x3<T, P> diskInertia3
	(
		T const & Mass, 
		T const & Radius
	)
	{
		T a = Mass * Radius * Radius / T(2);
		detail::tmat3x3<T, P> Result(a);
		Result[2][2] *= static_cast<T>(2);
		return Result;
	}

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, P> diskInertia4
	(
		T const & Mass, 
		T const & Radius
	)
	{
		T a = Mass * Radius * Radius / T(2);
		detail::tmat4x4<T, P> Result(a);
		Result[2][2] *= static_cast<T>(2);
		Result[3][3] = static_cast<T>(1);
		return Result;
	}

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat3x3<T, P> ballInertia3
	(
		T const & Mass, 
		T const & Radius
	)
	{
		T a = static_cast<T>(2) * Mass * Radius * Radius / T(5);
		return detail::tmat3x3<T, P>(a);
	}

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, P> ballInertia4
	(
		T const & Mass, 
		T const & Radius
	)
	{
		T a = static_cast<T>(2) * Mass * Radius * Radius / T(5);
		detail::tmat4x4<T, P> Result(a);
		Result[3][3] = static_cast<T>(1);
		return Result;
	}

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat3x3<T, P> sphereInertia3
	(
		T const & Mass, 
		T const & Radius
	)
	{
		T a = static_cast<T>(2) * Mass * Radius * Radius / T(3);
		return detail::tmat3x3<T, P>(a);
	}

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, P> sphereInertia4
	(
		T const & Mass, 
		T const & Radius
	)
	{
		T a = static_cast<T>(2) * Mass * Radius * Radius / T(3);
		detail::tmat4x4<T, P> Result(a);
		Result[3][3] = static_cast<T>(1);
		return Result;
	}
 */
}//namespace glm
