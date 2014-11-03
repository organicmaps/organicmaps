///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2007-03-06
// Updated : 2009-05-01
// Licence : This source is under MIT License
// File    : glm/gtx/polar_coordinates.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace glm
{
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> polar
	(
		detail::tvec3<T, P> const & euclidean
	)
	{
		T const Length(length(euclidean));
		detail::tvec3<T, P> const tmp(euclidean / Length);
		T const xz_dist(sqrt(tmp.x * tmp.x + tmp.z * tmp.z));

#ifdef GLM_FORCE_RADIANS
		return detail::tvec3<T, P>(
			atan(xz_dist, tmp.y),	// latitude
			atan(tmp.x, tmp.z),		// longitude
			xz_dist);				// xz distance
#else
#		pragma message("GLM: polar function returning degrees is deprecated. #define GLM_FORCE_RADIANS before including GLM headers to remove this message.")
		return detail::tvec3<T, P>(
			degrees(atan(xz_dist, tmp.y)),	// latitude
			degrees(atan(tmp.x, tmp.z)),	// longitude
			xz_dist);						// xz distance
#endif
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> euclidean
	(
		detail::tvec2<T, P> const & polar
	)
	{
#ifdef GLM_FORCE_RADIANS
		T const latitude(polar.x);
		T const longitude(polar.y);
#else
#		pragma message("GLM: euclidean function taking degrees as parameters is deprecated. #define GLM_FORCE_RADIANS before including GLM headers to remove this message.")
		T const latitude(radians(polar.x));
		T const longitude(radians(polar.y));
#endif

		return detail::tvec3<T, P>(
			cos(latitude) * sin(longitude),
			sin(latitude),
			cos(latitude) * cos(longitude));
	}

}//namespace glm
