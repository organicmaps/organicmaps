///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2007-03-05
// Updated : 2007-03-05
// Licence : This source is under MIT License
// File    : glm/gtx/matrix_query.inl
///////////////////////////////////////////////////////////////////////////////////////////////////
// Dependency:
// - GLM core
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace glm
{
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER bool isNull(detail::tmat2x2<T, P> const & m, T const & epsilon)
	{
		bool result = true;
		for(length_t i = 0; result && i < 2 ; ++i)
			result = isNull(m[i], epsilon);
		return result;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER bool isNull(detail::tmat3x3<T, P> const & m, T const & epsilon)
	{
		bool result = true;
		for(length_t i = 0; result && i < 3 ; ++i)
			result = isNull(m[i], epsilon);
		return result;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER bool isNull(detail::tmat4x4<T, P> const & m, T const & epsilon)
	{
		bool result = true;
		for(length_t i = 0; result && i < 4 ; ++i)
			result = isNull(m[i], epsilon);
		return result;
	}

	template<typename T, precision P, template <typename, precision> class matType>
	GLM_FUNC_QUALIFIER bool isIdentity(matType<T, P> const & m, T const & epsilon)
	{
		bool result = true;
		for(length_t i(0); result && i < m[0].length(); ++i)
		{
			for(length_t j(0); result && j < i ; ++j)
				result = abs(m[i][j]) <= epsilon;
			if(result)
				result = abs(m[i][i] - 1) <= epsilon;
			for(length_t j(i + 1); result && j < m.length(); ++j)
				result = abs(m[i][j]) <= epsilon;
		}
		return result;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER bool isNormalized(detail::tmat2x2<T, P> const & m, T const & epsilon)
	{
		bool result(true);
		for(length_t i(0); result && i < m.length(); ++i)
			result = isNormalized(m[i], epsilon);
		for(length_t i(0); result && i < m.length(); ++i)
		{
			typename detail::tmat2x2<T, P>::col_type v;
			for(length_t j(0); j < m.length(); ++j)
				v[j] = m[j][i];
			result = isNormalized(v, epsilon);
		}
		return result;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER bool isNormalized(detail::tmat3x3<T, P> const & m, T const & epsilon)
	{
		bool result(true);
		for(length_t i(0); result && i < m.length(); ++i)
			result = isNormalized(m[i], epsilon);
		for(length_t i(0); result && i < m.length(); ++i)
		{
			typename detail::tmat3x3<T, P>::col_type v;
			for(length_t j(0); j < m.length(); ++j)
				v[j] = m[j][i];
			result = isNormalized(v, epsilon);
		}
		return result;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER bool isNormalized(detail::tmat4x4<T, P> const & m, T const & epsilon)
	{
		bool result(true);
		for(length_t i(0); result && i < m.length(); ++i)
			result = isNormalized(m[i], epsilon);
		for(length_t i(0); result && i < m.length(); ++i)
		{
			typename detail::tmat4x4<T, P>::col_type v;
			for(length_t j(0); j < m.length(); ++j)
				v[j] = m[j][i];
			result = isNormalized(v, epsilon);
		}
		return result;
	}

	template<typename T, precision P, template <typename, precision> class matType>
	GLM_FUNC_QUALIFIER bool isOrthogonal(matType<T, P> const & m, T const & epsilon)
	{
		bool result(true);
		for(length_t i(0); result && i < m.length() - 1; ++i)
		for(length_t j(i + 1); result && j < m.length(); ++j)
			result = areOrthogonal(m[i], m[j], epsilon);

		if(result)
		{
			matType<T, P> tmp = transpose(m);
			for(length_t i(0); result && i < m.length() - 1 ; ++i)
			for(length_t j(i + 1); result && j < m.length(); ++j)
				result = areOrthogonal(tmp[i], tmp[j], epsilon);
		}
		return result;
	}
}//namespace glm
