///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2007-05-21
// Updated : 2010-02-12
// Licence : This source is under MIT License
// File    : gtx_component_wise.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace glm
{
	template <typename T, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER T compAdd(vecType<T, P> const & v)
	{
		T result(0);
		for(length_t i = 0; i < v.length(); ++i)
			result += v[i];
		return result;
	}

	template <typename T, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER T compMul(vecType<T, P> const & v)
	{
		T result(1);
		for(length_t i = 0; i < v.length(); ++i)
			result *= v[i];
		return result;
	}

	template <typename T, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER T compMin(vecType<T, P> const & v)
	{
		T result(v[0]);
		for(length_t i = 1; i < v.length(); ++i)
			result = min(result, v[i]);
		return result;
	}

	template <typename T, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER T compMax(vecType<T, P> const & v)
	{
		T result(v[0]);
		for(length_t i = 1; i < v.length(); ++i)
			result = max(result, v[i]);
		return result;
	}
}//namespace glm
