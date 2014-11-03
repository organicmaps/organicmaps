///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2009-10-26
// Updated : 2011-06-07
// Licence : This source is under MIT License
// File    : glm/gtx/multiple.inl
///////////////////////////////////////////////////////////////////////////////////////////////////
// Dependency:
// - GLM core
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace glm{
namespace detail
{
	template <bool Signed>
	struct higherMultiple
	{
		template <typename genType>
		GLM_FUNC_QUALIFIER genType operator()
		(
			genType const & Source,
			genType const & Multiple
		)
		{
			if (Source > genType(0))
			{
				genType Tmp = Source - genType(1);
				return Tmp + (Multiple - (Tmp % Multiple));
			}
			else
				return Source + (-Source % Multiple);
		}
	};

	template <>
	struct higherMultiple<false>
	{
		template <typename genType>
		GLM_FUNC_QUALIFIER genType operator()
		(
			genType const & Source,
			genType const & Multiple
		)
		{
			genType Tmp = Source - genType(1);
			return Tmp + (Multiple - (Tmp % Multiple));
		}
	};
}//namespace detail

	//////////////////////
	// higherMultiple

	template <typename genType>
	GLM_FUNC_QUALIFIER genType higherMultiple
	(
		genType const & Source,
		genType const & Multiple
	)
	{
		detail::higherMultiple<std::numeric_limits<genType>::is_signed> Compute;
		return Compute(Source, Multiple);
	}

	template <>
	GLM_FUNC_QUALIFIER float higherMultiple
	(	
		float const & Source,
		float const & Multiple
	)
	{
		if (Source > float(0))
		{
			float Tmp = Source - float(1);
			return Tmp + (Multiple - std::fmod(Tmp, Multiple));
		}
		else
			return Source + std::fmod(-Source, Multiple);
	}

	template <>
	GLM_FUNC_QUALIFIER double higherMultiple
	(
		double const & Source,
		double const & Multiple
	)
	{
		if (Source > double(0))
		{
			double Tmp = Source - double(1);
			return Tmp + (Multiple - std::fmod(Tmp, Multiple));
		}
		else
			return Source + std::fmod(-Source, Multiple);
	}

	VECTORIZE_VEC_VEC(higherMultiple)

	//////////////////////
	// lowerMultiple

	template <typename genType>
	GLM_FUNC_QUALIFIER genType lowerMultiple
	(
		genType const & Source,
		genType const & Multiple
	)
	{
		if (Source >= genType(0))
			return Source - Source % Multiple;
		else
		{
			genType Tmp = Source + genType(1);
			return Tmp - Tmp % Multiple - Multiple;
		}
	}

	template <>
	GLM_FUNC_QUALIFIER float lowerMultiple
	(
		float const & Source,
		float const & Multiple
	)
	{
		if (Source >= float(0))
			return Source - std::fmod(Source, Multiple);
		else
		{
			float Tmp = Source + float(1);
			return Tmp - std::fmod(Tmp, Multiple) - Multiple;
		}
	}

	template <>
	GLM_FUNC_QUALIFIER double lowerMultiple
	(
		double const & Source,
		double const & Multiple
	)
	{
		if (Source >= double(0))
			return Source - std::fmod(Source, Multiple);
		else
		{
			double Tmp = Source + double(1);
			return Tmp - std::fmod(Tmp, Multiple) - Multiple;
		}
	}

	VECTORIZE_VEC_VEC(lowerMultiple)
}//namespace glm
