///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2006-01-04
// Updated : 2011-10-14
// Licence : This source is under MIT License
// File    : glm/gtx/fast_square_root.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace glm
{
	// fastSqrt
	template <typename genType>
	GLM_FUNC_QUALIFIER genType fastSqrt
	(
		genType const & x
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<genType>::is_iec559, "'fastSqrt' only accept floating-point input");

		return genType(1) / fastInverseSqrt(x);
	}

	VECTORIZE_VEC(fastSqrt)

	// fastInversesqrt
	template <>
	GLM_FUNC_QUALIFIER float fastInverseSqrt<float>(float const & x)
	{
#		ifdef __CUDACC__ // Wordaround for a CUDA compiler bug up to CUDA6
			detail::tvec1<T, P> tmp(detail::compute_inversesqrt<detail::tvec1, float, lowp>::call(detail::tvec1<float, lowp>(x)));
			return tmp.x;
#		else
			return detail::compute_inversesqrt<detail::tvec1, float, lowp>::call(detail::tvec1<float, lowp>(x)).x;
#		endif
	}

	template <>
	GLM_FUNC_QUALIFIER double fastInverseSqrt<double>(double const & x)
	{
#		ifdef __CUDACC__ // Wordaround for a CUDA compiler bug up to CUDA6
			detail::tvec1<T, P> tmp(detail::compute_inversesqrt<detail::tvec1, double, lowp>::call(detail::tvec1<double, lowp>(x)));
			return tmp.x;
#		else
			return detail::compute_inversesqrt<detail::tvec1, double, lowp>::call(detail::tvec1<double, lowp>(x)).x;
#		endif
	}

	template <template <class, precision> class vecType, typename T, precision P>
	GLM_FUNC_QUALIFIER vecType<T, P> fastInverseSqrt
	(
		vecType<T, P> const & x
	)
	{
		return detail::compute_inversesqrt<vecType, T, P>::call(x);
	}

	VECTORIZE_VEC(fastInverseSqrt)

	// fastLength
	template <typename genType>
	GLM_FUNC_QUALIFIER genType fastLength
	(
		genType const & x
	)
	{
		return abs(x);
	}

	template <typename valType, precision P>
	GLM_FUNC_QUALIFIER valType fastLength
	(
		detail::tvec2<valType, P> const & x
	)
	{
		valType sqr = x.x * x.x + x.y * x.y;
		return fastSqrt(sqr);
	}

	template <typename valType, precision P>
	GLM_FUNC_QUALIFIER valType fastLength
	(
		detail::tvec3<valType, P> const & x
	)
	{
		valType sqr = x.x * x.x + x.y * x.y + x.z * x.z;
		return fastSqrt(sqr);
	}

	template <typename valType, precision P>
	GLM_FUNC_QUALIFIER valType fastLength
	(
		detail::tvec4<valType, P> const & x
	)
	{
		valType sqr = x.x * x.x + x.y * x.y + x.z * x.z + x.w * x.w;
		return fastSqrt(sqr);
	}

	// fastDistance
	template <typename genType>
	GLM_FUNC_QUALIFIER genType fastDistance
	(
		genType const & x,
		genType const & y
	)
	{
		return fastLength(y - x);
	}

	template <typename valType, precision P>
	GLM_FUNC_QUALIFIER valType fastDistance
	(
		detail::tvec2<valType, P> const & x,
		detail::tvec2<valType, P> const & y
	)
	{
		return fastLength(y - x);
	}

	template <typename valType, precision P>
	GLM_FUNC_QUALIFIER valType fastDistance
	(
		detail::tvec3<valType, P> const & x,
		detail::tvec3<valType, P> const & y
	)
	{
		return fastLength(y - x);
	}

	template <typename valType, precision P>
	GLM_FUNC_QUALIFIER valType fastDistance
	(
		detail::tvec4<valType, P> const & x,
		detail::tvec4<valType, P> const & y
	)
	{
		return fastLength(y - x);
	}

	// fastNormalize
	template <typename genType>
	GLM_FUNC_QUALIFIER genType fastNormalize
	(
		genType const & x
	)
	{
		return x > genType(0) ? genType(1) : -genType(1);
	}

	template <typename valType, precision P>
	GLM_FUNC_QUALIFIER detail::tvec2<valType, P> fastNormalize
	(
		detail::tvec2<valType, P> const & x
	)
	{
		valType sqr = x.x * x.x + x.y * x.y;
		return x * fastInverseSqrt(sqr);
	}

	template <typename valType, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<valType, P> fastNormalize
	(
		detail::tvec3<valType, P> const & x
	)
	{
		valType sqr = x.x * x.x + x.y * x.y + x.z * x.z;
		return x * fastInverseSqrt(sqr);
	}

	template <typename valType, precision P>
	GLM_FUNC_QUALIFIER detail::tvec4<valType, P> fastNormalize
	(
		detail::tvec4<valType, P> const & x
	)
	{
		valType sqr = x.x * x.x + x.y * x.y + x.z * x.z + x.w * x.w;
		return x * fastInverseSqrt(sqr);
	}
}//namespace glm
