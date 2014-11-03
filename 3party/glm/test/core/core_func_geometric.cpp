///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2011-01-15
// Updated : 2011-11-14
// Licence : This source is under MIT licence
// File    : test/core/func_geometric.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#define GLM_FORCE_RADIANS
#include <glm/geometric.hpp>
#include <glm/gtc/epsilon.hpp>

int test_reflect()
{
	int Error = 0;

	{
		glm::vec2 A(1.0f,-1.0f);
		glm::vec2 B(0.0f, 1.0f);
		glm::vec2 C = glm::reflect(A, B);
		Error += C == glm::vec2(1.0, 1.0) ? 0 : 1;
	}

	{
		glm::dvec2 A(1.0f,-1.0f);
		glm::dvec2 B(0.0f, 1.0f);
		glm::dvec2 C = glm::reflect(A, B);
		Error += C == glm::dvec2(1.0, 1.0) ? 0 : 1;
	}

	return Error;
}

int test_refract()
{
	int Error = 0;

	{
		float A(-1.0f);
		float B(1.0f);
		float C = glm::refract(A, B, 0.5f);
		Error += C == -1.0f ? 0 : 1;
	}

	{
		glm::vec2 A(0.0f,-1.0f);
		glm::vec2 B(0.0f, 1.0f);
		glm::vec2 C = glm::refract(A, B, 0.5f);
		Error += glm::all(glm::epsilonEqual(C, glm::vec2(0.0, -1.0), 0.0001f)) ? 0 : 1;
	}

	{
		glm::dvec2 A(0.0f,-1.0f);
		glm::dvec2 B(0.0f, 1.0f);
		glm::dvec2 C = glm::refract(A, B, 0.5);
		Error += C == glm::dvec2(0.0, -1.0) ? 0 : 1;
	}

	return Error;
}

int main()
{
	int Error(0);

	Error += test_reflect();
	Error += test_refract();

	return Error;
}

