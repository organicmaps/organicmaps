///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2006-01-07
// Updated : 2008-10-05
// Licence : This source is under MIT License
// File    : glm/gtx/extend.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace glm
{
	template <typename genType>
	GLM_FUNC_QUALIFIER genType extend
	(
		genType const & Origin, 
		genType const & Source, 
		genType const & Distance
	)
	{
		return Origin + (Source - Origin) * Distance;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec2<T, P> extend
	(
		detail::tvec2<T, P> const & Origin,
		detail::tvec2<T, P> const & Source,
		T const & Distance
	)
	{
		return Origin + (Source - Origin) * Distance;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> extend
	(
		detail::tvec3<T, P> const & Origin,
		detail::tvec3<T, P> const & Source,
		T const & Distance
	)
	{
		return Origin + (Source - Origin) * Distance;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec4<T, P> extend
	(
		detail::tvec4<T, P> const & Origin,
		detail::tvec4<T, P> const & Source,
		T const & Distance
	)
	{
		return Origin + (Source - Origin) * Distance;
	}
}//namespace glm
