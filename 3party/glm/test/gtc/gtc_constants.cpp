///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2012-09-19
// Updated : 2012-12-13
// Licence : This source is under MIT licence
// File    : test/gtc/constants.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#define GLM_FORCE_RADIANS
#include <glm/gtc/constants.hpp>

int test_epsilon()
{
	int Error(0);

	{
		float Test = glm::epsilon<float>();
	}

	{
		double Test = glm::epsilon<double>();
	}

	return Error;
}

int main()
{
	int Error(0);

	//float MinHalf = 0.0f;
	//while (glm::half(MinHalf) == glm::half(0.0f))
	//	MinHalf += std::numeric_limits<float>::epsilon();
	Error += test_epsilon();
	
	return Error;
}
