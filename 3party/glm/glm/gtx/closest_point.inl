///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2005-12-30
// Updated : 2008-10-05
// Licence : This source is under MIT License
// File    : glm/gtx/closest_point.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef glm_gtx_closest_point
#define glm_gtx_closest_point

namespace glm
{
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> closestPointOnLine
	(
		detail::tvec3<T, P> const & point,
		detail::tvec3<T, P> const & a,
		detail::tvec3<T, P> const & b
	)
	{
		T LineLength = distance(a, b);
		detail::tvec3<T, P> Vector = point - a;
		detail::tvec3<T, P> LineDirection = (b - a) / LineLength;

		// Project Vector to LineDirection to get the distance of point from a
		T Distance = dot(Vector, LineDirection);

		if(Distance <= T(0)) return a;
		if(Distance >= LineLength) return b;
		return a + LineDirection * Distance;
	}
}//namespace glm

#endif//glm_gtx_closest_point
