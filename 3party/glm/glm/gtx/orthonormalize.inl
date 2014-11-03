///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2005-12-21
// Updated : 2005-12-21
// Licence : This source is under MIT License
// File    : glm/gtx/orthonormalize.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace glm
{
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tmat3x3<T, P> orthonormalize
	(
		const detail::tmat3x3<T, P>& m
	)
	{
		detail::tmat3x3<T, P> r = m;

		r[0] = normalize(r[0]);

		float d0 = dot(r[0], r[1]);
		r[1] -= r[0] * d0;
		r[1] = normalize(r[1]);

		float d1 = dot(r[1], r[2]);
		d0 = dot(r[0], r[2]);
		r[2] -= r[0] * d0 + r[1] * d1;
		r[2] = normalize(r[2]);

		return r;
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> orthonormalize
	(
		const detail::tvec3<T, P>& x, 
		const detail::tvec3<T, P>& y
	)
	{
		return normalize(x - y * dot(y, x));
	}
}//namespace glm
