///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2013-10-25
// Updated : 2013-10-25
// Licence : This source is under MIT licence
// File    : test/gtx/color_space.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#define GLM_FORCE_RADIANS
#include <glm/gtc/type_precision.hpp>
#include <glm/gtx/color_space.hpp>

int test_saturation()
{
	int Error(0);
	
	glm::vec4 Color = glm::saturation(1.0f, glm::vec4(1.0, 0.5, 0.0, 1.0));

	return Error;
}

int main()
{
	int Error(0);

	Error += test_saturation();

	return Error;
}
