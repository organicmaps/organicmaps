///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2011-05-16
// Updated : 2011-05-16
// Licence : This source is under MIT licence
// File    : test/gtx/rotate_vector.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#define GLM_FORCE_RADIANS
#include <glm/gtc/constants.hpp>
#include <glm/gtx/rotate_vector.hpp>

int test_rotate()
{
	int Error = 0;

	glm::vec2 A = glm::rotate(glm::vec2(1, 0), glm::pi<float>() * 0.5f);
	glm::vec3 B = glm::rotate(glm::vec3(1, 0, 0), glm::pi<float>() * 0.5f, glm::vec3(0, 0, 1));
	glm::vec4 C = glm::rotate(glm::vec4(1, 0, 0, 1), glm::pi<float>() * 0.5f, glm::vec3(0, 0, 1));
	glm::vec3 D = glm::rotateX(glm::vec3(1, 0, 0), glm::pi<float>() * 0.5f);
	glm::vec4 E = glm::rotateX(glm::vec4(1, 0, 0, 1), glm::pi<float>() * 0.5f);
	glm::vec3 F = glm::rotateY(glm::vec3(1, 0, 0), glm::pi<float>() * 0.5f);
	glm::vec4 G = glm::rotateY(glm::vec4(1, 0, 0, 1), glm::pi<float>() * 0.5f);
	glm::vec3 H = glm::rotateZ(glm::vec3(1, 0, 0), glm::pi<float>() * 0.5f);
	glm::vec4 I = glm::rotateZ(glm::vec4(1, 0, 0,1 ), glm::pi<float>() * 0.5f);
	glm::mat4 O = glm::orientation(glm::normalize(glm::vec3(1)), glm::vec3(0, 0, 1));

	return Error;
}

int test_rotateX()
{
	int Error = 0;

	glm::vec3 D = glm::rotateX(glm::vec3(1, 0, 0), glm::pi<float>() * 0.5f);
	glm::vec4 E = glm::rotateX(glm::vec4(1, 0, 0, 1), glm::pi<float>() * 0.5f);

	return Error;
}

int test_rotateY()
{
	int Error = 0;

	glm::vec3 F = glm::rotateY(glm::vec3(1, 0, 0), glm::pi<float>() * 0.5f);
	glm::vec4 G = glm::rotateY(glm::vec4(1, 0, 0, 1), glm::pi<float>() * 0.5f);

	return Error;
}


int test_rotateZ()
{
	int Error = 0;

	glm::vec3 H = glm::rotateZ(glm::vec3(1, 0, 0), glm::pi<float>() * 0.5f);
	glm::vec4 I = glm::rotateZ(glm::vec4(1, 0, 0,1 ), glm::pi<float>() * 0.5f);

	return Error;
}

int test_orientation()
{
	int Error = 0;

	glm::mat4 O = glm::orientation(glm::normalize(glm::vec3(1)), glm::vec3(0, 0, 1));

	return Error;
}

int main()
{
	int Error = 0;

	Error += test_rotate();
	Error += test_rotateX();
	Error += test_rotateY();
	Error += test_rotateZ();
	Error += test_orientation();

	return Error;
}


