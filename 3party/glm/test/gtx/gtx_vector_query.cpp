///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2011-11-23
// Updated : 2011-11-23
// Licence : This source is under MIT licence
// File    : test/gtx/vector_query.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/vector_query.hpp>

int test_areCollinear()
{
	int Error(0);

	{
		bool TestA = glm::areCollinear(glm::vec2(-1), glm::vec2(1), 0.00001f);
		Error += TestA ? 0 : 1;
	}

	{
		bool TestA = glm::areCollinear(glm::vec3(-1), glm::vec3(1), 0.00001f);
		Error += TestA ? 0 : 1;
	}

	{
		bool TestA = glm::areCollinear(glm::vec4(-1), glm::vec4(1), 0.00001f);
		Error += TestA ? 0 : 1;
	}

	return Error;
}

int test_areOrthogonal()
{
	int Error(0);
	
	bool TestA = glm::areOrthogonal(glm::vec2(1, 0), glm::vec2(0, 1), 0.00001f);
	Error += TestA ? 0 : 1;

	return Error;
}

int test_isNormalized()
{
	int Error(0);
	
	bool TestA = glm::isNormalized(glm::vec4(1, 0, 0, 0), 0.00001f);
	Error += TestA ? 0 : 1;

	return Error;
}

int test_isNull()
{
	int Error(0);
	
	bool TestA = glm::isNull(glm::vec4(0), 0.00001f);
	Error += TestA ? 0 : 1;

	return Error;
}

int test_areOrthonormal()
{
	int Error(0);

	bool TestA = glm::areOrthonormal(glm::vec2(1, 0), glm::vec2(0, 1), 0.00001f);
	Error += TestA ? 0 : 1;

	return Error;
}

int main()
{
	int Error(0);

	Error += test_areCollinear();
	Error += test_areOrthogonal();
	Error += test_isNormalized();
	Error += test_isNull();
	Error += test_areOrthonormal();

	return Error;
}


