///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2008-03-10
// Updated : 2008-03-15
// Licence : This source is under MIT License
// File    : gtx_associated_min_max.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace glm{

// Min comparison between 2 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER U associatedMin(T x, U a, T y, U b)
{
	return x < y ? a : b;
}

template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec2<U, P> associatedMin
(
	const detail::tvec2<T, P>& x, const detail::tvec2<U, P>& a,
	const detail::tvec2<T, P>& y, const detail::tvec2<U, P>& b
)
{
	detail::tvec2<U, P> Result;
	//Result.x = x[0] < y[0] ? a[0] : b[0];
	//Result.y = x[1] < y[1] ? a[1] : b[1];
	for(typename detail::tvec2<U, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x[i] < y[i] ? a[i] : b[i];
	return Result;
}

template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec3<U, P> associatedMin
(
	const detail::tvec3<T, P>& x, const detail::tvec3<U, P>& a,
	const detail::tvec3<T, P>& y, const detail::tvec3<U, P>& b
)
{
	detail::tvec3<U, P> Result;
	for(typename detail::tvec3<U, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x[i] < y[i] ? a[i] : b[i];
	return Result;
}

template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec4<U, P> associatedMin
(
	const detail::tvec4<T, P>& x, const detail::tvec4<U, P>& a,
	const detail::tvec4<T, P>& y, const detail::tvec4<U, P>& b
)
{
	detail::tvec4<U, P> Result;
	for(typename detail::tvec4<U, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x[i] < y[i] ? a[i] : b[i];
	return Result;
}

template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec2<U, P> associatedMin
(
	T x, const detail::tvec2<U, P>& a,
	T y, const detail::tvec2<U, P>& b
)
{
	detail::tvec2<U, P> Result;
	for(typename detail::tvec2<U, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x < y ? a[i] : b[i];
	return Result;
}

template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec3<U, P> associatedMin
(
	T x, const detail::tvec3<U, P>& a,
	T y, const detail::tvec3<U, P>& b
)
{
	detail::tvec3<U, P> Result;
	for(typename detail::tvec3<U, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x < y ? a[i] : b[i];
	return Result;
}

template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec4<U, P> associatedMin
(
	T x, const detail::tvec4<U, P>& a,
	T y, const detail::tvec4<U, P>& b
)
{
	detail::tvec4<U, P> Result;
	for(typename detail::tvec4<U, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x < y ? a[i] : b[i];
	return Result;
}

template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec2<U, P> associatedMin
(
	detail::tvec2<T, P> const & x, U a,
	detail::tvec2<T, P> const & y, U b
)
{
	detail::tvec2<U, P> Result;
	for(typename detail::tvec2<U, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x[i] < y[i] ? a : b;
	return Result;
}

template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec3<U, P> associatedMin
(
	const detail::tvec3<T, P>& x, U a,
	const detail::tvec3<T, P>& y, U b
)
{
	detail::tvec3<U, P> Result;
	for(typename detail::tvec3<U, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x[i] < y[i] ? a : b;
	return Result;
}

template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec4<U, P> associatedMin
(
	const detail::tvec4<T, P>& x, U a,
	const detail::tvec4<T, P>& y, U b
)
{
	detail::tvec4<U, P> Result;
	for(typename detail::tvec4<U, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x[i] < y[i] ? a : b;
	return Result;
}

// Min comparison between 3 variables
template<typename T, typename U>
GLM_FUNC_QUALIFIER U associatedMin
(
	T x, U a,
	T y, U b,
	T z, U c
)
{
	U Result = x < y ? (x < z ? a : c) : (y < z ? b : c);
	return Result;
}

template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec2<U, P> associatedMin
(
	const detail::tvec2<T, P>& x, const detail::tvec2<U, P>& a,
	const detail::tvec2<T, P>& y, const detail::tvec2<U, P>& b,
	const detail::tvec2<T, P>& z, const detail::tvec2<U, P>& c
)
{
	detail::tvec2<U, P> Result;
	for(typename detail::tvec2<U, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x[i] < y[i] ? (x[i] < z[i] ? a[i] : c[i]) : (y[i] < z[i] ? b[i] : c[i]);
	return Result;
}

template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec3<U, P> associatedMin
(
	const detail::tvec3<T, P>& x, const detail::tvec3<U, P>& a,
	const detail::tvec3<T, P>& y, const detail::tvec3<U, P>& b,
	const detail::tvec3<T, P>& z, const detail::tvec3<U, P>& c
)
{
	detail::tvec3<U, P> Result;
	for(typename detail::tvec3<U, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x[i] < y[i] ? (x[i] < z[i] ? a[i] : c[i]) : (y[i] < z[i] ? b[i] : c[i]);
	return Result;
}

template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec4<U, P> associatedMin
(
	const detail::tvec4<T, P>& x, const detail::tvec4<U, P>& a,
	const detail::tvec4<T, P>& y, const detail::tvec4<U, P>& b,
	const detail::tvec4<T, P>& z, const detail::tvec4<U, P>& c
)
{
	detail::tvec4<U, P> Result;
	for(typename detail::tvec4<U, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x[i] < y[i] ? (x[i] < z[i] ? a[i] : c[i]) : (y[i] < z[i] ? b[i] : c[i]);
	return Result;
}

// Min comparison between 4 variables
template<typename T, typename U>
GLM_FUNC_QUALIFIER U associatedMin
(
	T x, U a,
	T y, U b,
	T z, U c,
	T w, U d
)
{
	T Test1 = min(x, y);
	T Test2 = min(z, w);;
	U Result1 = x < y ? a : b;
	U Result2 = z < w ? c : d;
	U Result = Test1 < Test2 ? Result1 : Result2;
	return Result;
}

// Min comparison between 4 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec2<U, P> associatedMin
(
	const detail::tvec2<T, P>& x, const detail::tvec2<U, P>& a,
	const detail::tvec2<T, P>& y, const detail::tvec2<U, P>& b,
	const detail::tvec2<T, P>& z, const detail::tvec2<U, P>& c,
	const detail::tvec2<T, P>& w, const detail::tvec2<U, P>& d
)
{
	detail::tvec2<U, P> Result;
	for(typename detail::tvec2<U, P>::size_type i = 0; i < Result.length(); ++i)
	{
		T Test1 = min(x[i], y[i]);
		T Test2 = min(z[i], w[i]);
		U Result1 = x[i] < y[i] ? a[i] : b[i];
		U Result2 = z[i] < w[i] ? c[i] : d[i];
		Result[i] = Test1 < Test2 ? Result1 : Result2;
	}
	return Result;
}

// Min comparison between 4 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec3<U, P> associatedMin
(
	const detail::tvec3<T, P>& x, const detail::tvec3<U, P>& a,
	const detail::tvec3<T, P>& y, const detail::tvec3<U, P>& b,
	const detail::tvec3<T, P>& z, const detail::tvec3<U, P>& c,
	const detail::tvec3<T, P>& w, const detail::tvec3<U, P>& d
)
{
	detail::tvec3<U, P> Result;
	for(typename detail::tvec3<U, P>::size_type i = 0; i < Result.length(); ++i)
	{
		T Test1 = min(x[i], y[i]);
		T Test2 = min(z[i], w[i]);
		U Result1 = x[i] < y[i] ? a[i] : b[i];
		U Result2 = z[i] < w[i] ? c[i] : d[i];
		Result[i] = Test1 < Test2 ? Result1 : Result2;
	}
	return Result;
}

// Min comparison between 4 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec4<U, P> associatedMin
(
	const detail::tvec4<T, P>& x, const detail::tvec4<U, P>& a,
	const detail::tvec4<T, P>& y, const detail::tvec4<U, P>& b,
	const detail::tvec4<T, P>& z, const detail::tvec4<U, P>& c,
	const detail::tvec4<T, P>& w, const detail::tvec4<U, P>& d
)
{
	detail::tvec4<U, P> Result;
	for(typename detail::tvec4<U, P>::size_type i = 0; i < Result.length(); ++i)
	{
		T Test1 = min(x[i], y[i]);
		T Test2 = min(z[i], w[i]);
		U Result1 = x[i] < y[i] ? a[i] : b[i];
		U Result2 = z[i] < w[i] ? c[i] : d[i];
		Result[i] = Test1 < Test2 ? Result1 : Result2;
	}
	return Result;
}

// Min comparison between 4 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec2<U, P> associatedMin
(
	T x, const detail::tvec2<U, P>& a,
	T y, const detail::tvec2<U, P>& b,
	T z, const detail::tvec2<U, P>& c,
	T w, const detail::tvec2<U, P>& d
)
{
	T Test1 = min(x, y);
	T Test2 = min(z, w);

	detail::tvec2<U, P> Result;
	for(typename detail::tvec2<U, P>::size_type i = 0; i < Result.length(); ++i)
	{
		U Result1 = x < y ? a[i] : b[i];
		U Result2 = z < w ? c[i] : d[i];
		Result[i] = Test1 < Test2 ? Result1 : Result2;
	}
	return Result;
}

// Min comparison between 4 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec3<U, P> associatedMin
(
	T x, const detail::tvec3<U, P>& a,
	T y, const detail::tvec3<U, P>& b,
	T z, const detail::tvec3<U, P>& c,
	T w, const detail::tvec3<U, P>& d
)
{
	T Test1 = min(x, y);
	T Test2 = min(z, w);

	detail::tvec3<U, P> Result;
	for(typename detail::tvec3<U, P>::size_type i = 0; i < detail::tvec3<U, P>::value_size; ++i)
	{
		U Result1 = x < y ? a[i] : b[i];
		U Result2 = z < w ? c[i] : d[i];
		Result[i] = Test1 < Test2 ? Result1 : Result2;
	}
	return Result;
}

// Min comparison between 4 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec4<U, P> associatedMin
(
	T x, const detail::tvec4<U, P>& a,
	T y, const detail::tvec4<U, P>& b,
	T z, const detail::tvec4<U, P>& c,
	T w, const detail::tvec4<U, P>& d
)
{
	T Test1 = min(x, y);
	T Test2 = min(z, w);

	detail::tvec4<U, P> Result;
	for(typename detail::tvec4<U, P>::size_type i = 0; i < detail::tvec4<U, P>::value_size; ++i)
	{
		U Result1 = x < y ? a[i] : b[i];
		U Result2 = z < w ? c[i] : d[i];
		Result[i] = Test1 < Test2 ? Result1 : Result2;
	}
	return Result;
}

// Min comparison between 4 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec2<U, P> associatedMin
(
	const detail::tvec2<T, P>& x, U a,
	const detail::tvec2<T, P>& y, U b,
	const detail::tvec2<T, P>& z, U c,
	const detail::tvec2<T, P>& w, U d
)
{
	detail::tvec2<U, P> Result;
	for(typename detail::tvec2<T, P>::size_type i = 0; i < detail::tvec2<T, P>::value_size(); ++i)
	{
		T Test1 = min(x[i], y[i]);
		T Test2 = min(z[i], w[i]);;
		U Result1 = x[i] < y[i] ? a : b;
		U Result2 = z[i] < w[i] ? c : d;
		Result[i] = Test1 < Test2 ? Result1 : Result2;
	}
	return Result;
}

// Min comparison between 4 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec3<U, P> associatedMin
(
	const detail::tvec3<T, P>& x, U a,
	const detail::tvec3<T, P>& y, U b,
	const detail::tvec3<T, P>& z, U c,
	const detail::tvec3<T, P>& w, U d
)
{
	detail::tvec3<U, P> Result;
	for(typename detail::tvec3<T, P>::size_type i = 0; i < detail::tvec3<T, P>::value_size(); ++i)
	{
		T Test1 = min(x[i], y[i]);
		T Test2 = min(z[i], w[i]);;
		U Result1 = x[i] < y[i] ? a : b;
		U Result2 = z[i] < w[i] ? c : d;
		Result[i] = Test1 < Test2 ? Result1 : Result2;
	}
	return Result;
}

// Min comparison between 4 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec4<U, P> associatedMin
(
	const detail::tvec4<T, P>& x, U a,
	const detail::tvec4<T, P>& y, U b,
	const detail::tvec4<T, P>& z, U c,
	const detail::tvec4<T, P>& w, U d
)
{
	detail::tvec4<U, P> Result;
	for(typename detail::tvec4<T, P>::size_type i = 0; i < detail::tvec4<T, P>::value_size(); ++i)
	{
		T Test1 = min(x[i], y[i]);
		T Test2 = min(z[i], w[i]);;
		U Result1 = x[i] < y[i] ? a : b;
		U Result2 = z[i] < w[i] ? c : d;
		Result[i] = Test1 < Test2 ? Result1 : Result2;
	}
	return Result;
}

// Max comparison between 2 variables
template<typename T, typename U>
GLM_FUNC_QUALIFIER U associatedMax(T x, U a, T y, U b)
{
	return x > y ? a : b;
}

// Max comparison between 2 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec2<U, P> associatedMax
(
	const detail::tvec2<T, P>& x, const detail::tvec2<U, P>& a,
	const detail::tvec2<T, P>& y, const detail::tvec2<U, P>& b
)
{
	detail::tvec2<U, P> Result;
	for(typename detail::tvec2<U, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x[i] > y[i] ? a[i] : b[i];
	return Result;
}

// Max comparison between 2 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec3<U, P> associatedMax
(
	const detail::tvec3<T, P>& x, const detail::tvec3<U, P>& a,
	const detail::tvec3<T, P>& y, const detail::tvec3<U, P>& b
)
{
	detail::tvec3<U, P> Result;
	for(typename detail::tvec3<U, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x[i] > y[i] ? a[i] : b[i];
	return Result;
}

// Max comparison between 2 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec4<U, P> associatedMax
(
	const detail::tvec4<T, P>& x, const detail::tvec4<U, P>& a,
	const detail::tvec4<T, P>& y, const detail::tvec4<U, P>& b
)
{
	detail::tvec4<U, P> Result;
	for(typename detail::tvec4<U, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x[i] > y[i] ? a[i] : b[i];
	return Result;
}

// Max comparison between 2 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec2<U, P> associatedMax
(
	T x, const detail::tvec2<U, P>& a,
	T y, const detail::tvec2<U, P>& b
)
{
	detail::tvec2<U, P> Result;
	for(typename detail::tvec2<U, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x > y ? a[i] : b[i];
	return Result;
}

// Max comparison between 2 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec3<U, P> associatedMax
(
	T x, const detail::tvec3<U, P>& a,
	T y, const detail::tvec3<U, P>& b
)
{
	detail::tvec3<U, P> Result;
	for(typename detail::tvec3<U, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x > y ? a[i] : b[i];
	return Result;
}

// Max comparison between 2 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec4<U, P> associatedMax
(
	T x, const detail::tvec4<U, P>& a,
	T y, const detail::tvec4<U, P>& b
)
{
	detail::tvec4<U, P> Result;
	for(typename detail::tvec4<U, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x > y ? a[i] : b[i];
	return Result;
}

// Max comparison between 2 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec2<U, P> associatedMax
(
	const detail::tvec2<T, P>& x, U a,
	const detail::tvec2<T, P>& y, U b
)
{
	detail::tvec2<U, P> Result;
	for(typename detail::tvec2<T, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x[i] > y[i] ? a : b;
	return Result;
}

// Max comparison between 2 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec3<U, P> associatedMax
(
	const detail::tvec3<T, P>& x, U a,
	const detail::tvec3<T, P>& y, U b
)
{
	detail::tvec3<U, P> Result;
	for(typename detail::tvec3<T, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x[i] > y[i] ? a : b;
	return Result;
}

// Max comparison between 2 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec4<U, P> associatedMax
(
	const detail::tvec4<T, P>& x, U a,
	const detail::tvec4<T, P>& y, U b
)
{
	detail::tvec4<U, P> Result;
	for(typename detail::tvec4<T, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x[i] > y[i] ? a : b;
	return Result;
}

// Max comparison between 3 variables
template<typename T, typename U>
GLM_FUNC_QUALIFIER U associatedMax
(
	T x, U a,
	T y, U b,
	T z, U c
)
{
	U Result = x > y ? (x > z ? a : c) : (y > z ? b : c);
	return Result;
}

// Max comparison between 3 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec2<U, P> associatedMax
(
	const detail::tvec2<T, P>& x, const detail::tvec2<U, P>& a,
	const detail::tvec2<T, P>& y, const detail::tvec2<U, P>& b,
	const detail::tvec2<T, P>& z, const detail::tvec2<U, P>& c
)
{
	detail::tvec2<U, P> Result;
	for(typename detail::tvec2<U, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x[i] > y[i] ? (x[i] > z[i] ? a[i] : c[i]) : (y[i] > z[i] ? b[i] : c[i]);
	return Result;
}

// Max comparison between 3 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec3<U, P> associatedMax
(
	const detail::tvec3<T, P>& x, const detail::tvec3<U, P>& a,
	const detail::tvec3<T, P>& y, const detail::tvec3<U, P>& b,
	const detail::tvec3<T, P>& z, const detail::tvec3<U, P>& c
)
{
	detail::tvec3<U, P> Result;
	for(typename detail::tvec3<U, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x[i] > y[i] ? (x[i] > z[i] ? a[i] : c[i]) : (y[i] > z[i] ? b[i] : c[i]);
	return Result;
}

// Max comparison between 3 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec4<U, P> associatedMax
(
	const detail::tvec4<T, P>& x, const detail::tvec4<U, P>& a,
	const detail::tvec4<T, P>& y, const detail::tvec4<U, P>& b,
	const detail::tvec4<T, P>& z, const detail::tvec4<U, P>& c
)
{
	detail::tvec4<U, P> Result;
	for(typename detail::tvec4<U, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x[i] > y[i] ? (x[i] > z[i] ? a[i] : c[i]) : (y[i] > z[i] ? b[i] : c[i]);
	return Result;
}

// Max comparison between 3 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec2<U, P> associatedMax
(
	T x, const detail::tvec2<U, P>& a,
	T y, const detail::tvec2<U, P>& b,
	T z, const detail::tvec2<U, P>& c
)
{
	detail::tvec2<U, P> Result;
	for(typename detail::tvec2<U, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x > y ? (x > z ? a[i] : c[i]) : (y > z ? b[i] : c[i]);
	return Result;
}

// Max comparison between 3 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec3<U, P> associatedMax
(
	T x, const detail::tvec3<U, P>& a,
	T y, const detail::tvec3<U, P>& b,
	T z, const detail::tvec3<U, P>& c
)
{
	detail::tvec3<U, P> Result;
	for(typename detail::tvec3<U, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x > y ? (x > z ? a[i] : c[i]) : (y > z ? b[i] : c[i]);
	return Result;
}

// Max comparison between 3 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec4<U, P> associatedMax
(
	T x, const detail::tvec4<U, P>& a,
	T y, const detail::tvec4<U, P>& b,
	T z, const detail::tvec4<U, P>& c
)
{
	detail::tvec4<U, P> Result;
	for(typename detail::tvec4<U, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x > y ? (x > z ? a[i] : c[i]) : (y > z ? b[i] : c[i]);
	return Result;
}

// Max comparison between 3 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec2<U, P> associatedMax
(
	const detail::tvec2<T, P>& x, U a,
	const detail::tvec2<T, P>& y, U b,
	const detail::tvec2<T, P>& z, U c
)
{
	detail::tvec2<U, P> Result;
	for(typename detail::tvec2<T, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x[i] > y[i] ? (x[i] > z[i] ? a : c) : (y[i] > z[i] ? b : c);
	return Result;
}

// Max comparison between 3 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec3<U, P> associatedMax
(
	const detail::tvec3<T, P>& x, U a,
	const detail::tvec3<T, P>& y, U b,
	const detail::tvec3<T, P>& z, U c
)
{
	detail::tvec3<U, P> Result;
	for(typename detail::tvec3<T, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x[i] > y[i] ? (x[i] > z[i] ? a : c) : (y[i] > z[i] ? b : c);
	return Result;
}

// Max comparison between 3 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec4<U, P> associatedMax
(
	const detail::tvec4<T, P>& x, U a,
	const detail::tvec4<T, P>& y, U b,
	const detail::tvec4<T, P>& z, U c
)
{
	detail::tvec4<U, P> Result;
	for(typename detail::tvec4<T, P>::size_type i = 0; i < Result.length(); ++i)
		Result[i] = x[i] > y[i] ? (x[i] > z[i] ? a : c) : (y[i] > z[i] ? b : c);
	return Result;
}

// Max comparison between 4 variables
template<typename T, typename U>
GLM_FUNC_QUALIFIER U associatedMax
(
	T x, U a,
	T y, U b,
	T z, U c,
	T w, U d
)
{
	T Test1 = max(x, y);
	T Test2 = max(z, w);;
	U Result1 = x > y ? a : b;
	U Result2 = z > w ? c : d;
	U Result = Test1 > Test2 ? Result1 : Result2;
	return Result;
}

// Max comparison between 4 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec2<U, P> associatedMax
(
	const detail::tvec2<T, P>& x, const detail::tvec2<U, P>& a,
	const detail::tvec2<T, P>& y, const detail::tvec2<U, P>& b,
	const detail::tvec2<T, P>& z, const detail::tvec2<U, P>& c,
	const detail::tvec2<T, P>& w, const detail::tvec2<U, P>& d
)
{
	detail::tvec2<U, P> Result;
	for(typename detail::tvec2<U, P>::size_type i = 0; i < Result.length(); ++i)
	{
		T Test1 = max(x[i], y[i]);
		T Test2 = max(z[i], w[i]);
		U Result1 = x[i] > y[i] ? a[i] : b[i];
		U Result2 = z[i] > w[i] ? c[i] : d[i];
		Result[i] = Test1 > Test2 ? Result1 : Result2;
	}
	return Result;
}

// Max comparison between 4 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec3<U, P> associatedMax
(
	const detail::tvec3<T, P>& x, const detail::tvec3<U, P>& a,
	const detail::tvec3<T, P>& y, const detail::tvec3<U, P>& b,
	const detail::tvec3<T, P>& z, const detail::tvec3<U, P>& c,
	const detail::tvec3<T, P>& w, const detail::tvec3<U, P>& d
)
{
	detail::tvec3<U, P> Result;
	for(typename detail::tvec3<U, P>::size_type i = 0; i < Result.length(); ++i)
	{
		T Test1 = max(x[i], y[i]);
		T Test2 = max(z[i], w[i]);
		U Result1 = x[i] > y[i] ? a[i] : b[i];
		U Result2 = z[i] > w[i] ? c[i] : d[i];
		Result[i] = Test1 > Test2 ? Result1 : Result2;
	}
	return Result;
}

// Max comparison between 4 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec4<U, P> associatedMax
(
	const detail::tvec4<T, P>& x, const detail::tvec4<U, P>& a,
	const detail::tvec4<T, P>& y, const detail::tvec4<U, P>& b,
	const detail::tvec4<T, P>& z, const detail::tvec4<U, P>& c,
	const detail::tvec4<T, P>& w, const detail::tvec4<U, P>& d
)
{
	detail::tvec4<U, P> Result;
	for(typename detail::tvec4<U, P>::size_type i = 0; i < Result.length(); ++i)
	{
		T Test1 = max(x[i], y[i]);
		T Test2 = max(z[i], w[i]);
		U Result1 = x[i] > y[i] ? a[i] : b[i];
		U Result2 = z[i] > w[i] ? c[i] : d[i];
		Result[i] = Test1 > Test2 ? Result1 : Result2;
	}
	return Result;
}

// Max comparison between 4 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec2<U, P> associatedMax
(
	T x, const detail::tvec2<U, P>& a,
	T y, const detail::tvec2<U, P>& b,
	T z, const detail::tvec2<U, P>& c,
	T w, const detail::tvec2<U, P>& d
)
{
	T Test1 = max(x, y);
	T Test2 = max(z, w);

	detail::tvec2<U, P> Result;
	for(typename detail::tvec2<U, P>::size_type i = 0; i < Result.length(); ++i)
	{
		U Result1 = x > y ? a[i] : b[i];
		U Result2 = z > w ? c[i] : d[i];
		Result[i] = Test1 > Test2 ? Result1 : Result2;
	}
	return Result;
}

// Max comparison between 4 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec3<U, P> associatedMax
(
	T x, const detail::tvec3<U, P>& a,
	T y, const detail::tvec3<U, P>& b,
	T z, const detail::tvec3<U, P>& c,
	T w, const detail::tvec3<U, P>& d
)
{
	T Test1 = max(x, y);
	T Test2 = max(z, w);

	detail::tvec3<U, P> Result;
	for(typename detail::tvec3<U, P>::size_type i = 0; i < Result.length(); ++i)
	{
		U Result1 = x > y ? a[i] : b[i];
		U Result2 = z > w ? c[i] : d[i];
		Result[i] = Test1 > Test2 ? Result1 : Result2;
	}
	return Result;
}

// Max comparison between 4 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec4<U, P> associatedMax
(
	T x, const detail::tvec4<U, P>& a,
	T y, const detail::tvec4<U, P>& b,
	T z, const detail::tvec4<U, P>& c,
	T w, const detail::tvec4<U, P>& d
)
{
	T Test1 = max(x, y);
	T Test2 = max(z, w);

	detail::tvec4<U, P> Result;
	for(typename detail::tvec4<U, P>::size_type i = 0; i < Result.length(); ++i)
	{
		U Result1 = x > y ? a[i] : b[i];
		U Result2 = z > w ? c[i] : d[i];
		Result[i] = Test1 > Test2 ? Result1 : Result2;
	}
	return Result;
}

// Max comparison between 4 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec2<U, P> associatedMax
(
	const detail::tvec2<T, P>& x, U a,
	const detail::tvec2<T, P>& y, U b,
	const detail::tvec2<T, P>& z, U c,
	const detail::tvec2<T, P>& w, U d
)
{
	detail::tvec2<U, P> Result;
	for(typename detail::tvec2<T, P>::size_type i = 0; i < Result.length(); ++i)
	{
		T Test1 = max(x[i], y[i]);
		T Test2 = max(z[i], w[i]);;
		U Result1 = x[i] > y[i] ? a : b;
		U Result2 = z[i] > w[i] ? c : d;
		Result[i] = Test1 > Test2 ? Result1 : Result2;
	}
	return Result;
}

// Max comparison between 4 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec3<U, P> associatedMax
(
	const detail::tvec3<T, P>& x, U a,
	const detail::tvec3<T, P>& y, U b,
	const detail::tvec3<T, P>& z, U c,
	const detail::tvec3<T, P>& w, U d
)
{
	detail::tvec3<U, P> Result;
	for(typename detail::tvec3<T, P>::size_type i = 0; i < Result.length(); ++i)
	{
		T Test1 = max(x[i], y[i]);
		T Test2 = max(z[i], w[i]);;
		U Result1 = x[i] > y[i] ? a : b;
		U Result2 = z[i] > w[i] ? c : d;
		Result[i] = Test1 > Test2 ? Result1 : Result2;
	}
	return Result;
}

// Max comparison between 4 variables
template<typename T, typename U, precision P>
GLM_FUNC_QUALIFIER detail::tvec4<U, P> associatedMax
(
	const detail::tvec4<T, P>& x, U a,
	const detail::tvec4<T, P>& y, U b,
	const detail::tvec4<T, P>& z, U c,
	const detail::tvec4<T, P>& w, U d
)
{
	detail::tvec4<U, P> Result;
	for(typename detail::tvec4<T, P>::size_type i = 0; i < Result.length(); ++i)
	{
		T Test1 = max(x[i], y[i]);
		T Test2 = max(z[i], w[i]);;
		U Result1 = x[i] > y[i] ? a : b;
		U Result2 = z[i] > w[i] ? c : d;
		Result[i] = Test1 > Test2 ? Result1 : Result2;
	}
	return Result;
}

}//namespace glm
