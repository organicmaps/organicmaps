///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2008-08-31
// Updated : 2011-05-06
// Licence : This source is under MIT License
// File    : test/core/type_int.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

int test_int_size()
{
	return
		sizeof(glm::int_t) != sizeof(glm::lowp_int) &&
		sizeof(glm::int_t) != sizeof(glm::mediump_int) && 
		sizeof(glm::int_t) != sizeof(glm::highp_int);
}

int test_uint_size()
{
	return
		sizeof(glm::uint_t) != sizeof(glm::lowp_uint) &&
		sizeof(glm::uint_t) != sizeof(glm::mediump_uint) && 
		sizeof(glm::uint_t) != sizeof(glm::highp_uint);
}

int test_int_precision()
{
	return (
		sizeof(glm::lowp_int) <= sizeof(glm::mediump_int) && 
		sizeof(glm::mediump_int) <= sizeof(glm::highp_int)) ? 0 : 1;
}

int test_uint_precision()
{
	return (
		sizeof(glm::lowp_uint) <= sizeof(glm::mediump_uint) && 
		sizeof(glm::mediump_uint) <= sizeof(glm::highp_uint)) ? 0 : 1;
}

int main()
{
	int Error = 0;

	Error += test_int_size();
	Error += test_int_precision();
	Error += test_uint_size();
	Error += test_uint_precision();

	return Error;
}
