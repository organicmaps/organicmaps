///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2008-08-31
// Updated : 2008-08-31
// Licence : This source is under MIT License
// File    : test/core/type_mat4x2.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#define GLM_FORCE_RADIANS
#include <glm/vector_relational.hpp>
#include <glm/mat4x2.hpp>
#include <vector>

static int test_operators()
{
	glm::mat4x2 l(1.0f);
	glm::mat4x2 m(1.0f);
	glm::vec4 u(1.0f);
	glm::vec2 v(1.0f);
	float x = 1.0f;
	glm::vec2 a = m * u;
	glm::vec4 b = v * m;
	glm::mat4x2 n = x / m;
	glm::mat4x2 o = m / x;
	glm::mat4x2 p = x * m;
	glm::mat4x2 q = m * x;
	bool R = m != q;
	bool S = m == l;

	return (S && !R) ? 0 : 1;
}

int test_ctr()
{
	int Error(0);

#if(GLM_HAS_INITIALIZER_LISTS)
	glm::mat4x2 m0(
		glm::vec2(0, 1), 
		glm::vec2(2, 3),
		glm::vec2(4, 5),
		glm::vec2(6, 7));

	glm::mat4x2 m1{0, 1, 2, 3, 4, 5, 6, 7};

	glm::mat4x2 m2{
		{0, 1},
		{2, 3},
		{4, 5},
		{6, 7}};

	for(glm::length_t i = 0; i < m0.length(); ++i)
		Error += glm::all(glm::equal(m0[i], m2[i])) ? 0 : 1;

	for(glm::length_t i = 0; i < m1.length(); ++i)
		Error += glm::all(glm::equal(m1[i], m2[i])) ? 0 : 1;

	std::vector<glm::mat4x2> v1{
		{0, 1, 2, 3, 4, 5, 6, 7},
		{0, 1, 2, 3, 4, 5, 6, 7}
	};

	std::vector<glm::mat4x2> v2{
		{
			{ 0, 1},
			{ 4, 5},
			{ 8, 9},
			{ 12, 13}
		},
		{
			{ 0, 1},
			{ 4, 5},
			{ 8, 9},
			{ 12, 13}
		}
	};

#endif//GLM_HAS_INITIALIZER_LISTS

	return Error;
}

int main()
{
	int Error = 0;

	Error += test_ctr();
	Error += test_operators();

	return Error;
}

