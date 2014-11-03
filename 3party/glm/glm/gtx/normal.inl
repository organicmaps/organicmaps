///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2005-12-21
// Updated : 2011-06-07
// Licence : This source is under MIT License
// File    : glm/gtx/normal.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace glm
{
	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> triangleNormal
	(
		detail::tvec3<T, P> const & p1, 
		detail::tvec3<T, P> const & p2, 
		detail::tvec3<T, P> const & p3
	)
	{
		return normalize(cross(p1 - p2, p1 - p3));
	}
}//namespace glm
