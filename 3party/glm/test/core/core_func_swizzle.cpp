///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2011-10-16
// Updated : 2011-10-16
// Licence : This source is under MIT License
// File    : test/core/core_func_swizzle.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#define GLM_FORCE_RADIANS
#define GLM_MESSAGES
#define GLM_SWIZZLE
#include <glm/glm.hpp>

int test_ivec2_swizzle()
{
	int Error = 0;

	glm::ivec2 A(1, 2);
	glm::ivec2 B = A.yx();
	glm::ivec2 C = B.yx();

	Error += A != B ? 0 : 1;
	Error += A == C ? 0 : 1;

	return Error;
}

int test_ivec3_swizzle()
{
	int Error = 0;

	glm::ivec3 A(1, 2, 3);
	glm::ivec3 B = A.zyx();
	glm::ivec3 C = B.zyx();

	Error += A != B ? 0 : 1;
	Error += A == C ? 0 : 1;

	return Error;
}

int test_ivec4_swizzle()
{
	int Error = 0;

	glm::ivec4 A(1, 2, 3, 4);
	glm::ivec4 B = A.wzyx();
	glm::ivec4 C = B.wzyx();

	Error += A != B ? 0 : 1;
	Error += A == C ? 0 : 1;

	return Error;
}

int test_vec4_swizzle()
{
	int Error = 0;

	glm::vec4 A(1, 2, 3, 4);
	glm::vec4 B = A.wzyx();
	glm::vec4 C = B.wzyx();

	float f = glm::dot(C.wzyx(), C.xyzw());

	Error += A != B ? 0 : 1;
	Error += A == C ? 0 : 1;

	return Error;
}

int main()
{
	int Error = 0;

	Error += test_ivec2_swizzle();
	Error += test_ivec3_swizzle();
	Error += test_ivec4_swizzle();

	Error += test_vec4_swizzle();

	return Error;
}



