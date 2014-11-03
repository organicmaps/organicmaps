///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2011-09-01
// Updated : 2011-09-01
// Licence : This source is under MIT licence
// File    : test/gtx/string_cast.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <limits>

int test_string_cast_scalar()
{
	int Error = 0;	
	
	float B1(1.0);
	std::string B2 = glm::to_string(B1);
	Error += B2 != std::string("float(1.000000)") ? 1 : 0;
	
	double C1(1.0);
	std::string C2 = glm::to_string(C1);
	Error += C2 != std::string("double(1.000000)") ? 1 : 0;
	
	return Error;
}

int test_string_cast_vector()
{
	int Error = 0;
	
	glm::vec2 A1(1, 2);
	std::string A2 = glm::to_string(A1);
	Error += A2 != std::string("fvec2(1.000000, 2.000000)") ? 1 : 0;
	
	glm::vec3 B1(1, 2, 3);
	std::string B2 = glm::to_string(B1);
	Error += B2 != std::string("fvec3(1.000000, 2.000000, 3.000000)") ? 1 : 0;

	glm::vec4 C1(1, 2, 3, 4);
	std::string C2 = glm::to_string(C1);
	Error += C2 != std::string("fvec4(1.000000, 2.000000, 3.000000, 4.000000)") ? 1 : 0;
	
	glm::ivec2 D1(1, 2);
	std::string D2 = glm::to_string(D1);
	Error += D2 != std::string("ivec2(1, 2)") ? 1 : 0;
	
	glm::ivec3 E1(1, 2, 3);
	std::string E2 = glm::to_string(E1);
	Error += E2 != std::string("ivec3(1, 2, 3)") ? 1 : 0;
	
	glm::ivec4 F1(1, 2, 3, 4);
	std::string F2 = glm::to_string(F1);
	Error += F2 != std::string("ivec4(1, 2, 3, 4)") ? 1 : 0;
	
	glm::dvec2 J1(1, 2);
	std::string J2 = glm::to_string(J1);
	Error += J2 != std::string("dvec2(1.000000, 2.000000)") ? 1 : 0;
	
	glm::dvec3 K1(1, 2, 3);
	std::string K2 = glm::to_string(K1);
	Error += K2 != std::string("dvec3(1.000000, 2.000000, 3.000000)") ? 1 : 0;
	
	glm::dvec4 L1(1, 2, 3, 4);
	std::string L2 = glm::to_string(L1);
	Error += L2 != std::string("dvec4(1.000000, 2.000000, 3.000000, 4.000000)") ? 1 : 0;
	
	return Error;
}

int test_string_cast_matrix()
{
	int Error = 0;
	
	return Error;
}

int main()
{
	int Error = 0;
	Error += test_string_cast_scalar();
	Error += test_string_cast_vector();
	Error += test_string_cast_matrix();
	return Error;
}


