///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2010-09-16
// Updated : 2010-09-16
// Licence : This source is under MIT licence
// File    : test/gtc/matrix_transform.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

int test_perspective()
{
	int Error = 0;

	glm::mat4 Projection = glm::perspective(glm::pi<float>() * 0.25f, 4.0f / 3.0f, 0.1f, 100.0f);

	return Error;
}

int test_pick()
{
	int Error = 0;

	glm::mat4 Pick = glm::pickMatrix(glm::vec2(1, 2), glm::vec2(3, 4), glm::ivec4(0, 0, 320, 240));

	return Error;
}

int test_tweakedInfinitePerspective()
{
	int Error = 0;

	glm::mat4 ProjectionA = glm::tweakedInfinitePerspective(45.f, 640.f/480.f, 1.0f);
	glm::mat4 ProjectionB = glm::tweakedInfinitePerspective(45.f, 640.f/480.f, 1.0f, 0.001f);


	return Error;
}

int test_translate()
{
	int Error = 0;

	glm::lowp_vec3 v(1.0);
	glm::lowp_mat4 m(0);
	glm::lowp_mat4 t = glm::translate(m, v);

	return Error;
}

int main()
{
	int Error = 0;

	Error += test_translate();
	Error += test_tweakedInfinitePerspective();
	Error += test_pick();
	Error += test_perspective();

	return Error;
}
