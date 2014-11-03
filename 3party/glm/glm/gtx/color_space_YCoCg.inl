///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2008-10-28
// Updated : 2008-10-28
// Licence : This source is under MIT License
// File    : glm/gtx/color_space_YCoCg.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace glm
{
	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> rgb2YCoCg
	(
		detail::tvec3<T, P> const & rgbColor
	)
	{
		detail::tvec3<T, P> result;
		result.x/*Y */ =   rgbColor.r / T(4) + rgbColor.g / T(2) + rgbColor.b / T(4);
		result.y/*Co*/ =   rgbColor.r / T(2) + rgbColor.g * T(0) - rgbColor.b / T(2);
		result.z/*Cg*/ = - rgbColor.r / T(4) + rgbColor.g / T(2) - rgbColor.b / T(4);
		return result;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> rgb2YCoCgR
	(
		detail::tvec3<T, P> const & rgbColor
	)
	{
		detail::tvec3<T, P> result;
		result.x/*Y */ = rgbColor.g / T(2) + (rgbColor.r + rgbColor.b) / T(4);
		result.y/*Co*/ = rgbColor.r - rgbColor.b;
		result.z/*Cg*/ = rgbColor.g - (rgbColor.r + rgbColor.b) / T(2);
		return result;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> YCoCg2rgb
	(
		detail::tvec3<T, P> const & YCoCgColor
	)
	{
		detail::tvec3<T, P> result;
		result.r = YCoCgColor.x + YCoCgColor.y - YCoCgColor.z;
		result.g = YCoCgColor.x                + YCoCgColor.z;
		result.b = YCoCgColor.x - YCoCgColor.y - YCoCgColor.z;
		return result;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> YCoCgR2rgb
	(
		detail::tvec3<T, P> const & YCoCgRColor
	)
	{
		detail::tvec3<T, P> result;
		T tmp = YCoCgRColor.x - (YCoCgRColor.z / T(2));
		result.g = YCoCgRColor.z + tmp;
		result.b = tmp - (YCoCgRColor.y / T(2));
		result.r = result.b + YCoCgRColor.y;
		return result;
	}
}//namespace glm
