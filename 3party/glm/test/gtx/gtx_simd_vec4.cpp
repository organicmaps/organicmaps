///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2010-09-16
// Updated : 2010-09-16
// Licence : This source is under MIT licence
// File    : test/gtx/simd-vec4.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/simd_vec4.hpp>
#include <cstdio>

#if(GLM_ARCH != GLM_ARCH_PURE)

int main()
{
	glm::simdVec4 A1(0.0f, 0.1f, 0.2f, 0.3f);
	glm::simdVec4 B1(0.4f, 0.5f, 0.6f, 0.7f);
	glm::simdVec4 C1 = A1 + B1;
	glm::simdVec4 D1 = A1.swizzle<glm::X, glm::Z, glm::Y, glm::W>();
	glm::simdVec4 E1(glm::vec4(1.0f));
	glm::vec4 F1 = glm::vec4_cast(E1);
	//glm::vec4 G1(E1);

	//printf("A1(%2.3f, %2.3f, %2.3f, %2.3f)\n", A1.x, A1.y, A1.z, A1.w);
	//printf("B1(%2.3f, %2.3f, %2.3f, %2.3f)\n", B1.x, B1.y, B1.z, B1.w);
	//printf("C1(%2.3f, %2.3f, %2.3f, %2.3f)\n", C1.x, C1.y, C1.z, C1.w);
	//printf("D1(%2.3f, %2.3f, %2.3f, %2.3f)\n", D1.x, D1.y, D1.z, D1.w);

	__m128 value = _mm_set1_ps(0.0f);
	__m128 data = _mm_cmpeq_ps(value, value);
	__m128 add0 = _mm_add_ps(data, data);

	glm::simdVec4 GNI(add0);

	return 0;
}

#else

int main()
{
	int Error = 0;

	return Error;
}

#endif//(GLM_ARCH != GLM_ARCH_PURE)
