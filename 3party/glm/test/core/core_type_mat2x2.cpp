///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2008-08-31
// Updated : 2008-08-31
// Licence : This source is under MIT License
// File    : test/core/type_mat2x2.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#define GLM_FORCE_RADIANS
#include <glm/gtc/epsilon.hpp>
#include <glm/matrix.hpp>
#include <glm/vector_relational.hpp>
#include <glm/mat2x2.hpp>
#include <vector>

int test_operators()
{
	glm::mat2x2 l(1.0f);
	glm::mat2x2 m(1.0f);
	glm::vec2 u(1.0f);
	glm::vec2 v(1.0f);
	float x = 1.0f;
	glm::vec2 a = m * u;
	glm::vec2 b = v * m;
	glm::mat2x2 n = x / m;
	glm::mat2x2 o = m / x;
	glm::mat2x2 p = x * m;
	glm::mat2x2 q = m * x;
	bool R = m != q;
	bool S = m == l;

	return (S && !R) ? 0 : 1;
}

int test_inverse()
{
	int Error(0);

	{
		glm::mat2 const Matrix(1, 2, 3, 4);
		glm::mat2 const Inverse = glm::inverse(Matrix);
		glm::mat2 const Identity = Matrix * Inverse;

		Error += glm::all(glm::epsilonEqual(Identity[0], glm::vec2(1.0f, 0.0f), glm::vec2(0.01f))) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Identity[1], glm::vec2(0.0f, 1.0f), glm::vec2(0.01f))) ? 0 : 1;
	}

	{
		glm::mat2 const Matrix(1, 2, 3, 4);
		glm::mat2 const Identity = Matrix / Matrix;

		Error += glm::all(glm::epsilonEqual(Identity[0], glm::vec2(1.0f, 0.0f), glm::vec2(0.01f))) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Identity[1], glm::vec2(0.0f, 1.0f), glm::vec2(0.01f))) ? 0 : 1;
	}

	return Error;
}

int test_ctr()
{
	int Error(0);

#if(GLM_HAS_INITIALIZER_LISTS)
	glm::mat2x2 m0(
		glm::vec2(0, 1), 
		glm::vec2(2, 3));

	glm::mat2x2 m1{0, 1, 2, 3};

	glm::mat2x2 m2{
		{0, 1},
		{2, 3}};

	for(glm::length_t i = 0; i < m0.length(); ++i)
		Error += glm::all(glm::equal(m0[i], m2[i])) ? 0 : 1;

	for(glm::length_t i = 0; i < m1.length(); ++i)
		Error += glm::all(glm::equal(m1[i], m2[i])) ? 0 : 1;

	std::vector<glm::mat2x2> v1{
		{0, 1, 2, 3},
		{0, 1, 2, 3}
	};

	std::vector<glm::mat2x2> v2{
		{
			{ 0, 1},
			{ 4, 5}
		},
		{
			{ 0, 1},
			{ 4, 5}
		}
	};

#endif//GLM_HAS_INITIALIZER_LISTS

	return Error;
}

int main()
{
	int Error(0);

	Error += test_ctr();
	Error += test_operators();
	Error += test_inverse();

	return Error;
}
