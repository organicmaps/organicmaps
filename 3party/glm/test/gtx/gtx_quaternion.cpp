///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2011-05-25
// Updated : 2011-05-31
// Licence : This source is under MIT licence
// File    : test/gtx/quaternion.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#define GLM_FORCE_RADIANS
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/ext.hpp>

int test_quat_fastMix()
{
	int Error = 0;

	glm::quat A = glm::angleAxis(0.0f, glm::vec3(0, 0, 1));
	glm::quat B = glm::angleAxis(glm::pi<float>() * 0.5f, glm::vec3(0, 0, 1));
	glm::quat C = glm::fastMix(A, B, 0.5f);
	glm::quat D = glm::angleAxis(glm::pi<float>() * 0.25f, glm::vec3(0, 0, 1));

	Error += glm::epsilonEqual(C.x, D.x, 0.01f) ? 0 : 1;
	Error += glm::epsilonEqual(C.y, D.y, 0.01f) ? 0 : 1;
	Error += glm::epsilonEqual(C.z, D.z, 0.01f) ? 0 : 1;
	Error += glm::epsilonEqual(C.w, D.w, 0.01f) ? 0 : 1;

	return Error;
}

int test_quat_shortMix()
{
	int Error(0);

	glm::quat A = glm::angleAxis(0.0f, glm::vec3(0, 0, 1));
	glm::quat B = glm::angleAxis(glm::pi<float>() * 0.5f, glm::vec3(0, 0, 1));
	glm::quat C = glm::shortMix(A, B, 0.5f);
	glm::quat D = glm::angleAxis(glm::pi<float>() * 0.25f, glm::vec3(0, 0, 1));

	Error += glm::epsilonEqual(C.x, D.x, 0.01f) ? 0 : 1;
	Error += glm::epsilonEqual(C.y, D.y, 0.01f) ? 0 : 1;
	Error += glm::epsilonEqual(C.z, D.z, 0.01f) ? 0 : 1;
	Error += glm::epsilonEqual(C.w, D.w, 0.01f) ? 0 : 1;

	return Error;
}

int test_orientation()
{
	int Error(0);

	{
		glm::quat q(1.0f, 0.0f, 0.0f, 1.0f);
		float p = glm::roll(q);
	}

	{
		glm::quat q(1.0f, 0.0f, 0.0f, 1.0f);
		float p = glm::pitch(q);
	}

	{
		glm::quat q(1.0f, 0.0f, 0.0f, 1.0f);
		float p = glm::yaw(q);
	}

	return Error;
}

int test_rotation()
{
	int Error(0);

	glm::vec3 v(1, 0, 0);
	glm::vec3 u(0, 1, 0);

	glm::quat Rotation = glm::rotation(v, u);

	float Angle = glm::angle(Rotation);

	Error += glm::abs(Angle - glm::pi<float>() * 0.5f) < glm::epsilon<float>() ? 0 : 1;

	return Error;
}

int test_log()
{
	int Error(0);
	
	glm::quat q;
	glm::quat p = glm::log(q);
	glm::quat r = glm::exp(p);

	return Error;
}

int main()
{
	int Error(0);

	Error += test_log();
	Error += test_rotation();
	Error += test_quat_fastMix();
	Error += test_quat_shortMix();

	return Error;
}
