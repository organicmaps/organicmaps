///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2011-04-26
// Updated : 2011-04-26
// Licence : This source is under MIT licence
// File    : test/gtc/ulp.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#define GLM_FORCE_RADIANS
#include <glm/gtc/ulp.hpp>
#include <iostream>
#include <limits>

int test_ulp_float_dist()
{
	int Error = 0;

	float A = 1.0f;

	float B = glm::next_float(A);
	Error += A != B ? 0 : 1;
	float C = glm::prev_float(B);
	Error += A == C ? 0 : 1;

	int D = glm::float_distance(A, B);
	Error += D == 1 ? 0 : 1;
	int E = glm::float_distance(A, C);
	Error += E == 0 ? 0 : 1;

	return Error;
}

int test_ulp_float_step()
{
	int Error = 0;

	float A = 1.0f;

	for(int i = 10; i < 1000; i *= 10)
	{
		float B = glm::next_float(A, i);
		Error += A != B ? 0 : 1;
		float C = glm::prev_float(B, i);
		Error += A == C ? 0 : 1;

		int D = glm::float_distance(A, B);
		Error += D == i ? 0 : 1;
		int E = glm::float_distance(A, C);
		Error += E == 0 ? 0 : 1;
	}

	return Error;
}

int test_ulp_double_dist()
{
	int Error = 0;

	double A = 1.0;

	double B = glm::next_float(A);
	Error += A != B ? 0 : 1;
	double C = glm::prev_float(B);
	Error += A == C ? 0 : 1;

	int D = glm::float_distance(A, B);
	Error += D == 1 ? 0 : 1;
	int E = glm::float_distance(A, C);
	Error += E == 0 ? 0 : 1;

	return Error;
}

int test_ulp_double_step()
{
	int Error = 0;

	double A = 1.0;

	for(int i = 10; i < 1000; i *= 10)
	{
		double B = glm::next_float(A, i);
		Error += A != B ? 0 : 1;
		double C = glm::prev_float(B, i);
		Error += A == C ? 0 : 1;

		int D = glm::float_distance(A, B);
		Error += D == i ? 0 : 1;
		int E = glm::float_distance(A, C);
		Error += E == 0 ? 0 : 1;
	}

	return Error;
}

int main()
{
	int Error = 0;
	Error += test_ulp_float_dist();
	Error += test_ulp_float_step();
	Error += test_ulp_double_dist();
	Error += test_ulp_double_step();
	return Error;
}


