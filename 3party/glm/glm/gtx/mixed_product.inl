///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2007-04-03
// Updated : 2008-09-17
// Licence : This source is under MIT License
// File    : glm/gtx/mixed_product.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace glm
{
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER T mixedProduct
	(
		detail::tvec3<T, P> const & v1,
		detail::tvec3<T, P> const & v2,
		detail::tvec3<T, P> const & v3
	)
	{
		return dot(cross(v1, v2), v3);
	}
}//namespace glm
