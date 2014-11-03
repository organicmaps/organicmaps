///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2008-08-31
// Updated : 2013-10-04
// Licence : This source is under MIT License
// File    : test/core/type_mat2x3.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#define GLM_FORCE_RADIANS
#include <glm/vector_relational.hpp>
#include <glm/mat2x3.hpp>
#include <vector>

static int test_operators()
{
	glm::mat2x3 l(1.0f);
	glm::mat2x3 m(1.0f);
	glm::vec2 u(1.0f);
	glm::vec3 v(1.0f);
	float x = 1.0f;
	glm::vec3 a = m * u;
	glm::vec2 b = v * m;
	glm::mat2x3 n = x / m;
	glm::mat2x3 o = m / x;
	glm::mat2x3 p = x * m;
	glm::mat2x3 q = m * x;
	bool R = m != q;
	bool S = m == l;

	return (S && !R) ? 0 : 1;
}

int test_ctr()
{
	int Error(0);
	
#if(GLM_HAS_INITIALIZER_LISTS)
	glm::mat2x3 m0(
		glm::vec3(0, 1, 2),
		glm::vec3(3, 4, 5));
	
	glm::mat2x3 m1{0, 1, 2, 3, 4, 5};
	
	glm::mat2x3 m2{
		{0, 1, 2},
		{3, 4, 5}};
	
	for(glm::length_t i = 0; i < m0.length(); ++i)
		Error += glm::all(glm::equal(m0[i], m2[i])) ? 0 : 1;
	
	for(glm::length_t i = 0; i < m1.length(); ++i)
		Error += glm::all(glm::equal(m1[i], m2[i])) ? 0 : 1;
	
	std::vector<glm::mat2x3> v1{
		{0, 1, 2, 3, 4, 5},
		{0, 1, 2, 3, 4, 5}
	};
	
	std::vector<glm::mat2x3> v2{
		{
			{ 0, 1, 2},
			{ 4, 5, 6}
		},
		{
			{ 0, 1, 2},
			{ 4, 5, 6}
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
