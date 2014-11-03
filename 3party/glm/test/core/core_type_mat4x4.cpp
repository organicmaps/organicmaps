///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2008-08-31
// Updated : 2008-08-31
// Licence : This source is under MIT License
// File    : test/core/type_mat4x4.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#define GLM_FORCE_RADIANS
#include <glm/gtc/epsilon.hpp>
#include <glm/matrix.hpp>
#include <glm/mat4x4.hpp>
#include <cstdio>
#include <vector>

void print(glm::dmat4 const & Mat0)
{
	printf("mat4(\n");
	printf("\tvec4(%2.9f, %2.9f, %2.9f, %2.9f)\n", Mat0[0][0], Mat0[0][1], Mat0[0][2], Mat0[0][3]);
	printf("\tvec4(%2.9f, %2.9f, %2.9f, %2.9f)\n", Mat0[1][0], Mat0[1][1], Mat0[1][2], Mat0[1][3]);
	printf("\tvec4(%2.9f, %2.9f, %2.9f, %2.9f)\n", Mat0[2][0], Mat0[2][1], Mat0[2][2], Mat0[2][3]);
	printf("\tvec4(%2.9f, %2.9f, %2.9f, %2.9f))\n\n", Mat0[3][0], Mat0[3][1], Mat0[3][2], Mat0[3][3]);
}

void print(glm::mat4 const & Mat0)
{
	printf("mat4(\n");
	printf("\tvec4(%2.9f, %2.9f, %2.9f, %2.9f)\n", Mat0[0][0], Mat0[0][1], Mat0[0][2], Mat0[0][3]);
	printf("\tvec4(%2.9f, %2.9f, %2.9f, %2.9f)\n", Mat0[1][0], Mat0[1][1], Mat0[1][2], Mat0[1][3]);
	printf("\tvec4(%2.9f, %2.9f, %2.9f, %2.9f)\n", Mat0[2][0], Mat0[2][1], Mat0[2][2], Mat0[2][3]);
	printf("\tvec4(%2.9f, %2.9f, %2.9f, %2.9f))\n\n", Mat0[3][0], Mat0[3][1], Mat0[3][2], Mat0[3][3]);
}

int test_inverse_mat4x4()
{
	glm::mat4 Mat0(
		glm::vec4(0.6f, 0.2f, 0.3f, 0.4f), 
		glm::vec4(0.2f, 0.7f, 0.5f, 0.3f), 
		glm::vec4(0.3f, 0.5f, 0.7f, 0.2f), 
		glm::vec4(0.4f, 0.3f, 0.2f, 0.6f));
	glm::mat4 Inv0 = glm::inverse(Mat0);
	glm::mat4 Res0 = Mat0 * Inv0;

	print(Mat0);
	print(Inv0);
	print(Res0);

	return 0;
}

int test_inverse_dmat4x4()
{
	glm::dmat4 Mat0(
		glm::dvec4(0.6f, 0.2f, 0.3f, 0.4f), 
		glm::dvec4(0.2f, 0.7f, 0.5f, 0.3f), 
		glm::dvec4(0.3f, 0.5f, 0.7f, 0.2f), 
		glm::dvec4(0.4f, 0.3f, 0.2f, 0.6f));
	glm::dmat4 Inv0 = glm::inverse(Mat0);
	glm::dmat4 Res0 = Mat0 * Inv0;

	print(Mat0);
	print(Inv0);
	print(Res0);

	return 0;
}

static bool test_operators()
{
	glm::mat4x4 l(1.0f);
	glm::mat4x4 m(1.0f);
	glm::vec4 u(1.0f);
	glm::vec4 v(1.0f);
	float x = 1.0f;
	glm::vec4 a = m * u;
	glm::vec4 b = v * m;
	glm::mat4x4 n = x / m;
	glm::mat4x4 o = m / x;
	glm::mat4x4 p = x * m;
	glm::mat4x4 q = m * x;
	bool R = m != q;
	bool S = m == l;

	return (S && !R) ? 0 : 1;
}

int test_inverse()
{
	int Error(0);

	{
		glm::mat4 const Matrix(
			glm::vec4(0.6f, 0.2f, 0.3f, 0.4f), 
			glm::vec4(0.2f, 0.7f, 0.5f, 0.3f), 
			glm::vec4(0.3f, 0.5f, 0.7f, 0.2f), 
			glm::vec4(0.4f, 0.3f, 0.2f, 0.6f));
		glm::mat4 const Inverse = glm::inverse(Matrix);
		glm::mat4 const Identity = Matrix * Inverse;

		print(Matrix);
		print(Inverse);
		print(Identity);

		Error += glm::all(glm::epsilonEqual(Identity[0], glm::vec4(1.0f, 0.0f, 0.0f, 0.0f), glm::vec4(0.01f))) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Identity[1], glm::vec4(0.0f, 1.0f, 0.0f, 0.0f), glm::vec4(0.01f))) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Identity[2], glm::vec4(0.0f, 0.0f, 1.0f, 0.0f), glm::vec4(0.01f))) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Identity[3], glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(0.01f))) ? 0 : 1;
	}

	{
		glm::highp_mat4 const Matrix(
			glm::highp_vec4(0.6f, 0.2f, 0.3f, 0.4f), 
			glm::highp_vec4(0.2f, 0.7f, 0.5f, 0.3f), 
			glm::highp_vec4(0.3f, 0.5f, 0.7f, 0.2f), 
			glm::highp_vec4(0.4f, 0.3f, 0.2f, 0.6f));
		glm::highp_mat4 const Inverse = glm::inverse(Matrix);
		glm::highp_mat4 const Identity = Matrix * Inverse;

		printf("highp_mat4 inverse\n");
		print(Matrix);
		print(Inverse);
		print(Identity);

		Error += glm::all(glm::epsilonEqual(Identity[0], glm::highp_vec4(1.0f, 0.0f, 0.0f, 0.0f), glm::highp_vec4(0.01f))) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Identity[1], glm::highp_vec4(0.0f, 1.0f, 0.0f, 0.0f), glm::highp_vec4(0.01f))) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Identity[2], glm::highp_vec4(0.0f, 0.0f, 1.0f, 0.0f), glm::highp_vec4(0.01f))) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Identity[3], glm::highp_vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::highp_vec4(0.01f))) ? 0 : 1;
	}

	{
		glm::mediump_mat4 const Matrix(
			glm::mediump_vec4(0.6f, 0.2f, 0.3f, 0.4f), 
			glm::mediump_vec4(0.2f, 0.7f, 0.5f, 0.3f), 
			glm::mediump_vec4(0.3f, 0.5f, 0.7f, 0.2f), 
			glm::mediump_vec4(0.4f, 0.3f, 0.2f, 0.6f));
		glm::mediump_mat4 const Inverse = glm::inverse(Matrix);
		glm::mediump_mat4 const Identity = Matrix * Inverse;

		printf("mediump_mat4 inverse\n");
		print(Matrix);
		print(Inverse);
		print(Identity);

		Error += glm::all(glm::epsilonEqual(Identity[0], glm::mediump_vec4(1.0f, 0.0f, 0.0f, 0.0f), glm::mediump_vec4(0.01f))) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Identity[1], glm::mediump_vec4(0.0f, 1.0f, 0.0f, 0.0f), glm::mediump_vec4(0.01f))) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Identity[2], glm::mediump_vec4(0.0f, 0.0f, 1.0f, 0.0f), glm::mediump_vec4(0.01f))) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Identity[3], glm::mediump_vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::mediump_vec4(0.01f))) ? 0 : 1;
	}

	{
		glm::lowp_mat4 const Matrix(
			glm::lowp_vec4(0.6f, 0.2f, 0.3f, 0.4f), 
			glm::lowp_vec4(0.2f, 0.7f, 0.5f, 0.3f), 
			glm::lowp_vec4(0.3f, 0.5f, 0.7f, 0.2f), 
			glm::lowp_vec4(0.4f, 0.3f, 0.2f, 0.6f));
		glm::lowp_mat4 const Inverse = glm::inverse(Matrix);
		glm::lowp_mat4 const Identity = Matrix * Inverse;

		printf("lowp_mat4 inverse\n");
		print(Matrix);
		print(Inverse);
		print(Identity);

		Error += glm::all(glm::epsilonEqual(Identity[0], glm::lowp_vec4(1.0f, 0.0f, 0.0f, 0.0f), glm::lowp_vec4(0.01f))) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Identity[1], glm::lowp_vec4(0.0f, 1.0f, 0.0f, 0.0f), glm::lowp_vec4(0.01f))) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Identity[2], glm::lowp_vec4(0.0f, 0.0f, 1.0f, 0.0f), glm::lowp_vec4(0.01f))) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Identity[3], glm::lowp_vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::lowp_vec4(0.01f))) ? 0 : 1;
	}

	{
		glm::mat4 const Matrix(
			glm::vec4(0.6f, 0.2f, 0.3f, 0.4f), 
			glm::vec4(0.2f, 0.7f, 0.5f, 0.3f), 
			glm::vec4(0.3f, 0.5f, 0.7f, 0.2f), 
			glm::vec4(0.4f, 0.3f, 0.2f, 0.6f));
		glm::mat4 const Identity = Matrix / Matrix;

		Error += glm::all(glm::epsilonEqual(Identity[0], glm::vec4(1.0f, 0.0f, 0.0f, 0.0f), glm::vec4(0.01f))) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Identity[1], glm::vec4(0.0f, 1.0f, 0.0f, 0.0f), glm::vec4(0.01f))) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Identity[2], glm::vec4(0.0f, 0.0f, 1.0f, 0.0f), glm::vec4(0.01f))) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Identity[3], glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(0.01f))) ? 0 : 1;
	}

	return Error;
}

int test_ctr()
{
	int Error(0);

#if(GLM_HAS_INITIALIZER_LISTS)
	glm::mat4 m0(
		glm::vec4(0, 1, 2, 3), 
		glm::vec4(4, 5, 6, 7),
		glm::vec4(8, 9, 10, 11),
		glm::vec4(12, 13, 14, 15));

	assert(sizeof(m0) == 4 * 4 * 4);

	glm::vec4 V{0, 1, 2, 3};

	glm::mat4 m1{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

	glm::mat4 m2{
		{0, 1, 2, 3},
		{4, 5, 6, 7},
		{8, 9, 10, 11},
		{12, 13, 14, 15}};

	for(glm::length_t i = 0; i < m0.length(); ++i)
		Error += glm::all(glm::equal(m0[i], m2[i])) ? 0 : 1;

	for(glm::length_t i = 0; i < m1.length(); ++i)
		Error += glm::all(glm::equal(m1[i], m2[i])) ? 0 : 1;

	std::vector<glm::mat4> m3{
		{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
		{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
		{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
		{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}};

	std::vector<glm::mat4> v1{
		{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
		{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}};

	std::vector<glm::mat4> v2{
		{
			{ 0, 1, 2, 3 },
			{ 4, 5, 6, 7 },
			{ 8, 9, 10, 11 },
			{ 12, 13, 14, 15 }
		},
		{
			{ 0, 1, 2, 3 },
			{ 4, 5, 6, 7 },
			{ 8, 9, 10, 11 },
			{ 12, 13, 14, 15 }
		}};

#endif//GLM_HAS_INITIALIZER_LISTS

	return Error;
}

int main()
{
	int Error = 0;

	Error += test_ctr();
	Error += test_inverse_dmat4x4();
	Error += test_inverse_mat4x4();
	Error += test_operators();
	Error += test_inverse();

	return Error;
}
