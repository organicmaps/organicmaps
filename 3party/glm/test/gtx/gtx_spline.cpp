///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2013-10-25
// Updated : 2013-10-25
// Licence : This source is under MIT licence
// File    : test/gtx/associated_min_max.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_precision.hpp>
#include <glm/gtx/spline.hpp>

namespace catmullRom
{
	int test()
	{
		int Error(0);

		glm::vec2 Result2 = glm::catmullRom(
			glm::vec2(0.0f, 0.0f),
			glm::vec2(1.0f, 0.0f),
			glm::vec2(1.0f, 1.0f),
			glm::vec2(0.0f, 1.0f), 0.5f);

		glm::vec3 Result3 = glm::catmullRom(
			glm::vec3(0.0f, 0.0f, 0.0f), 
			glm::vec3(1.0f, 0.0f, 0.0f), 
			glm::vec3(1.0f, 1.0f, 0.0f), 
			glm::vec3(0.0f, 1.0f, 0.0f), 0.5f);

		glm::vec4 Result4 = glm::catmullRom(
			glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 
			glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), 
			glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), 
			glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), 0.5f);

		return Error;
	}
}//catmullRom

namespace hermite
{
	int test()
	{
		int Error(0);

		glm::vec2 Result2 = glm::hermite(
			glm::vec2(0.0f, 0.0f),
			glm::vec2(1.0f, 0.0f),
			glm::vec2(1.0f, 1.0f),
			glm::vec2(0.0f, 1.0f), 0.5f);

		glm::vec3 Result3 = glm::hermite(
			glm::vec3(0.0f, 0.0f, 0.0f), 
			glm::vec3(1.0f, 0.0f, 0.0f), 
			glm::vec3(1.0f, 1.0f, 0.0f), 
			glm::vec3(0.0f, 1.0f, 0.0f), 0.5f);

		glm::vec4 Result4 = glm::hermite(
			glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 
			glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), 
			glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), 
			glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), 0.5f);

		return Error;
	}
}//catmullRom

namespace cubic
{
	int test()
	{
		int Error(0);

		glm::vec2 Result2 = glm::cubic(
			glm::vec2(0.0f, 0.0f),
			glm::vec2(1.0f, 0.0f),
			glm::vec2(1.0f, 1.0f),
			glm::vec2(0.0f, 1.0f), 0.5f);

		glm::vec3 Result3 = glm::cubic(
			glm::vec3(0.0f, 0.0f, 0.0f), 
			glm::vec3(1.0f, 0.0f, 0.0f), 
			glm::vec3(1.0f, 1.0f, 0.0f), 
			glm::vec3(0.0f, 1.0f, 0.0f), 0.5f);

		glm::vec4 Result = glm::cubic(
			glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 
			glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), 
			glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), 
			glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), 0.5f);

		return Error;
	}
}//catmullRom

int main()
{
	int Error(0);

	Error += catmullRom::test();
	Error += hermite::test();
	Error += cubic::test();

	return Error;
}
