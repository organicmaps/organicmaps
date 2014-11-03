///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2009-11-25
// Updated : 2010-02-13
// Licence : This source is under MIT License
// File    : glm/gtx/wrap.inl
///////////////////////////////////////////////////////////////////////////////////////////////////
// Dependency:
// - GLM core
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace glm
{
	template <typename genType> 
	GLM_FUNC_QUALIFIER genType clamp
	(
		genType const & Texcoord
	)
	{
		return glm::clamp(Texcoord, genType(0), genType(1));
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER detail::tvec2<T, P> clamp
	(
		detail::tvec2<T, P> const & Texcoord
	)
	{
		detail::tvec2<T, P> Result;
		for(typename detail::tvec2<T, P>::size_type i = 0; i < detail::tvec2<T, P>::value_size(); ++i)
			Result[i] = clamp(Texcoord[i]);
		return Result;
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> clamp
	(
		detail::tvec3<T, P> const & Texcoord
	)
	{
		detail::tvec3<T, P> Result;
		for(typename detail::tvec3<T, P>::size_type i = 0; i < detail::tvec3<T, P>::value_size(); ++i)
			Result[i] = clamp(Texcoord[i]);
		return Result;
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER detail::tvec4<T, P> clamp
	(
		detail::tvec4<T, P> const & Texcoord
	)
	{
		detail::tvec4<T, P> Result;
		for(typename detail::tvec4<T, P>::size_type i = 0; i < detail::tvec4<T, P>::value_size(); ++i)
			Result[i] = clamp(Texcoord[i]);
		return Result;
	}

	////////////////////////
	// repeat

	template <typename genType> 
	GLM_FUNC_QUALIFIER genType repeat
	(
		genType const & Texcoord
	)
	{
		return glm::fract(Texcoord);
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER detail::tvec2<T, P> repeat
	(
		detail::tvec2<T, P> const & Texcoord
	)
	{
		detail::tvec2<T, P> Result;
		for(typename detail::tvec2<T, P>::size_type i = 0; i < detail::tvec2<T, P>::value_size(); ++i)
			Result[i] = repeat(Texcoord[i]);
		return Result;
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> repeat
	(
		detail::tvec3<T, P> const & Texcoord
	)
	{
		detail::tvec3<T, P> Result;
		for(typename detail::tvec3<T, P>::size_type i = 0; i < detail::tvec3<T, P>::value_size(); ++i)
			Result[i] = repeat(Texcoord[i]);
		return Result;
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER detail::tvec4<T, P> repeat
	(
		detail::tvec4<T, P> const & Texcoord
	)
	{
		detail::tvec4<T, P> Result;
		for(typename detail::tvec4<T, P>::size_type i = 0; i < detail::tvec4<T, P>::value_size(); ++i)
			Result[i] = repeat(Texcoord[i]);
		return Result;
	}

	////////////////////////
	// mirrorRepeat

	template <typename genType, precision P> 
	GLM_FUNC_QUALIFIER genType mirrorRepeat
	(
		genType const & Texcoord
	)
	{
		genType const Clamp = genType(int(glm::floor(Texcoord)) % 2);
		genType const Floor = glm::floor(Texcoord);
		genType const Rest = Texcoord - Floor;
		genType const Mirror = Clamp + Rest;

		genType Out;
		if(Mirror >= genType(1))
			Out = genType(1) - Rest;
		else
			Out = Rest;
		return Out;
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER detail::tvec2<T, P> mirrorRepeat
	(
		detail::tvec2<T, P> const & Texcoord
	)
	{
		detail::tvec2<T, P> Result;
		for(typename detail::tvec2<T, P>::size_type i = 0; i < detail::tvec2<T, P>::value_size(); ++i)
			Result[i] = mirrorRepeat(Texcoord[i]);
		return Result;
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> mirrorRepeat
	(
		detail::tvec3<T, P> const & Texcoord
	)
	{
		detail::tvec3<T, P> Result;
		for(typename detail::tvec3<T, P>::size_type i = 0; i < detail::tvec3<T, P>::value_size(); ++i)
			Result[i] = mirrorRepeat(Texcoord[i]);
		return Result;
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER detail::tvec4<T, P> mirrorRepeat
	(
		detail::tvec4<T, P> const & Texcoord
	)
	{
		detail::tvec4<T, P> Result;
		for(typename detail::tvec4<T, P>::size_type i = 0; i < detail::tvec4<T, P>::value_size(); ++i)
			Result[i] = mirrorRepeat(Texcoord[i]);
		return Result;
	}
}//namespace glm
