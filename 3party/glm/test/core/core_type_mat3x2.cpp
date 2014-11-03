///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2008-08-31
// Updated : 2008-08-31
// Licence : This source is under MIT License
// File    : test/core/type_mat3x2.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#define GLM_FORCE_RADIANS
#include <glm/vector_relational.hpp>
#include <glm/mat3x2.hpp>
#include <vector>

static bool test_operators()
{
	glm::mat3x2 l(1.0f);
	glm::mat3x2 m(1.0f);
	glm::vec3 u(1.0f);
	glm::vec2 v(1.0f);
	float x = 1.0f;
	glm::vec2 a = m * u;
	glm::vec3 b = v * m;
	glm::mat3x2 n = x / m;
	glm::mat3x2 o = m / x;
	glm::mat3x2 p = x * m;
	glm::mat3x2 q = m * x;
	bool R = m != q;
	bool S = m == l;

	return (S && !R) ? 0 : 1;
}

int test_ctr()
{
	int Error(0);
	
#if(GLM_HAS_INITIALIZER_LISTS)
	glm::mat3x2 m0(
		glm::vec2(0, 1),
		glm::vec2(2, 3),
		glm::vec2(4, 5));
	
	glm::mat3x2 m1{0, 1, 2, 3, 4, 5};
	
	glm::mat3x2 m2{
		{0, 1},
		{2, 3},
		{4, 5}};
	
	for(glm::length_t i = 0; i < m0.length(); ++i)
		Error += glm::all(glm::equal(m0[i], m2[i])) ? 0 : 1;
	
	for(glm::length_t i = 0; i < m1.length(); ++i)
		Error += glm::all(glm::equal(m1[i], m2[i])) ? 0 : 1;
	
	std::vector<glm::mat3x2> v1{
		{0, 1, 2, 3, 4, 5},
		{0, 1, 2, 3, 4, 5}
	};
	
	std::vector<glm::mat3x2> v2{
		{
			{ 0, 1},
			{ 2, 3},
			{ 4, 5}
		},
		{
			{ 0, 1},
			{ 2, 3},
			{ 4, 5}
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


