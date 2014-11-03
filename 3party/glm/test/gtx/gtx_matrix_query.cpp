///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2011-11-22
// Updated : 2011-11-22
// Licence : This source is under MIT licence
// File    : test/gtx/matrix_query.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#define GLM_FORCE_RADIANS
#include <glm/gtx/matrix_query.hpp>

int test_isNull()
{
	int Error(0);
	
	bool TestA = glm::isNull(glm::mat4(0), 0.00001f);
	Error += TestA ? 0 : 1;

	return Error;
}

int test_isIdentity()
{
	int Error(0);
	
	{
		bool TestA = glm::isIdentity(glm::mat2(1), 0.00001f);
		Error += TestA ? 0 : 1;
	}
	{
		bool TestA = glm::isIdentity(glm::mat3(1), 0.00001f);
		Error += TestA ? 0 : 1;
	}
	{
		bool TestA = glm::isIdentity(glm::mat4(1), 0.00001f);
		Error += TestA ? 0 : 1;
	}

	return Error;
}

int test_isNormalized()
{
	int Error(0);

	bool TestA = glm::isNormalized(glm::mat4(1), 0.00001f);
	Error += TestA ? 0 : 1;

	return Error;
}

int test_isOrthogonal()
{
	int Error(0);

	bool TestA = glm::isOrthogonal(glm::mat4(1), 0.00001f);
	Error += TestA ? 0 : 1;

	return Error;
}

int main()
{
	int Error(0);

	Error += test_isNull();
	Error += test_isIdentity();
	Error += test_isNormalized();
	Error += test_isOrthogonal();

	return Error;
}


