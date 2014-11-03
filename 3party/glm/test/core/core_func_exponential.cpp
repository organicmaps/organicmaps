///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2011-01-15
// Updated : 2011-09-13
// Licence : This source is under MIT licence
// File    : test/core/func_exponential.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#define GLM_FORCE_RADIANS
#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/gtc/ulp.hpp>

namespace inversesqrt
{
	int test()
	{
		int Error(0);

		glm::uint ulp(0);
		float diff(0.0f);

		for(float f = 0.001f; f < 10.f; f *= 1.001f)
		{
			glm::lowp_fvec1 lowp_v = glm::inversesqrt(glm::lowp_fvec1(f));
			float defaultp_v = glm::inversesqrt(f);

			ulp = glm::max(glm::float_distance(lowp_v.x, defaultp_v), ulp);
			diff = glm::abs(lowp_v.x - defaultp_v);
		}

		return Error;
	}
}//namespace inversesqrt

int main()
{
	int Error(0);

	Error += inversesqrt::test();

	return Error;
}

