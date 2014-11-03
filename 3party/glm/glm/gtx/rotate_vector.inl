///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2006-11-02
// Updated : 2009-02-19
// Licence : This source is under MIT License
// File    : glm/gtx/rotate_vector.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace glm
{
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec2<T, P> rotate
	(
		detail::tvec2<T, P> const & v,
		T const & angle
	)
	{
		detail::tvec2<T, P> Result;
#ifdef GLM_FORCE_RADIANS
		T const Cos(cos(angle));
		T const Sin(sin(angle));
#else
#		pragma message("GLM: rotate function taking degrees as parameters is deprecated. #define GLM_FORCE_RADIANS before including GLM headers to remove this message.")
		T const Cos = cos(radians(angle));
		T const Sin = sin(radians(angle));
#endif
		Result.x = v.x * Cos - v.y * Sin;
		Result.y = v.x * Sin + v.y * Cos;
		return Result;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> rotate
	(
		detail::tvec3<T, P> const & v,
		T const & angle,
		detail::tvec3<T, P> const & normal
	)
	{
		return detail::tmat3x3<T, P>(glm::rotate(angle, normal)) * v;
	}
	/*
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> rotateGTX(
		const detail::tvec3<T, P>& x,
		T angle,
		const detail::tvec3<T, P>& normal)
	{
		const T Cos = cos(radians(angle));
		const T Sin = sin(radians(angle));
		return x * Cos + ((x * normal) * (T(1) - Cos)) * normal + cross(x, normal) * Sin;
	}
	*/
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec4<T, P> rotate
	(
		detail::tvec4<T, P> const & v,
		T const & angle,
		detail::tvec3<T, P> const & normal
	)
	{
		return rotate(angle, normal) * v;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> rotateX
	(
		detail::tvec3<T, P> const & v,
		T const & angle
	)
	{
		detail::tvec3<T, P> Result(v);

#ifdef GLM_FORCE_RADIANS
		T const Cos(cos(angle));
		T const Sin(sin(angle));
#else
#		pragma message("GLM: rotateX function taking degrees as parameters is deprecated. #define GLM_FORCE_RADIANS before including GLM headers to remove this message.")
		T const Cos = cos(radians(angle));
		T const Sin = sin(radians(angle));
#endif

		Result.y = v.y * Cos - v.z * Sin;
		Result.z = v.y * Sin + v.z * Cos;
		return Result;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> rotateY
	(
		detail::tvec3<T, P> const & v,
		T const & angle
	)
	{
		detail::tvec3<T, P> Result = v;

#ifdef GLM_FORCE_RADIANS
		T const Cos(cos(angle));
		T const Sin(sin(angle));
#else
#		pragma message("GLM: rotateY function taking degrees as parameters is deprecated. #define GLM_FORCE_RADIANS before including GLM headers to remove this message.")
		T const Cos(cos(radians(angle)));
		T const Sin(sin(radians(angle)));
#endif

		Result.x =  v.x * Cos + v.z * Sin;
		Result.z = -v.x * Sin + v.z * Cos;
		return Result;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> rotateZ
	(
		detail::tvec3<T, P> const & v,
		T const & angle
	)
	{
		detail::tvec3<T, P> Result = v;

#ifdef GLM_FORCE_RADIANS
		T const Cos(cos(angle));
		T const Sin(sin(angle));
#else
#		pragma message("GLM: rotateZ function taking degrees as parameters is deprecated. #define GLM_FORCE_RADIANS before including GLM headers to remove this message.")
		T const Cos(cos(radians(angle)));
		T const Sin(sin(radians(angle)));
#endif

		Result.x = v.x * Cos - v.y * Sin;
		Result.y = v.x * Sin + v.y * Cos;
		return Result;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec4<T, P> rotateX
	(
		detail::tvec4<T, P> const & v,
		T const & angle
	)
	{
		detail::tvec4<T, P> Result = v;

#ifdef GLM_FORCE_RADIANS
		T const Cos(cos(angle));
		T const Sin(sin(angle));
#else
#		pragma message("GLM: rotateX function taking degrees as parameters is deprecated. #define GLM_FORCE_RADIANS before including GLM headers to remove this message.")
		T const Cos(cos(radians(angle)));
		T const Sin(sin(radians(angle)));
#endif

		Result.y = v.y * Cos - v.z * Sin;
		Result.z = v.y * Sin + v.z * Cos;
		return Result;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec4<T, P> rotateY
	(
		detail::tvec4<T, P> const & v,
		T const & angle
	)
	{
		detail::tvec4<T, P> Result = v;

#ifdef GLM_FORCE_RADIANS
		T const Cos(cos(angle));
		T const Sin(sin(angle));
#else
#		pragma message("GLM: rotateX function taking degrees as parameters is deprecated. #define GLM_FORCE_RADIANS before including GLM headers to remove this message.")
		T const Cos(cos(radians(angle)));
		T const Sin(sin(radians(angle)));
#endif

		Result.x =  v.x * Cos + v.z * Sin;
		Result.z = -v.x * Sin + v.z * Cos;
		return Result;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec4<T, P> rotateZ
	(
		detail::tvec4<T, P> const & v,
		T const & angle
	)
	{
		detail::tvec4<T, P> Result = v;

#ifdef GLM_FORCE_RADIANS
		T const Cos(cos(angle));
		T const Sin(sin(angle));
#else
#		pragma message("GLM: rotateZ function taking degrees as parameters is deprecated. #define GLM_FORCE_RADIANS before including GLM headers to remove this message.")
		T const Cos(cos(radians(angle)));
		T const Sin(sin(radians(angle)));
#endif

		Result.x = v.x * Cos - v.y * Sin;
		Result.y = v.x * Sin + v.y * Cos;
		return Result;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tmat4x4<T, P> orientation
	(
		detail::tvec3<T, P> const & Normal,
		detail::tvec3<T, P> const & Up
	)
	{
		if(all(equal(Normal, Up)))
			return detail::tmat4x4<T, P>(T(1));

		detail::tvec3<T, P> RotationAxis = cross(Up, Normal);
#		ifdef GLM_FORCE_RADIANS
			T Angle = acos(dot(Normal, Up));
#		else
#			pragma message("GLM: rotateZ function taking degrees as parameters is deprecated. #define GLM_FORCE_RADIANS before including GLM headers to remove this message.")
			T Angle = degrees(acos(dot(Normal, Up)));
#		endif
		return rotate(Angle, RotationAxis);
	}
}//namespace glm
