///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2013-10-25
// Updated : 2014-01-11
// Licence : This source is under MIT licence
// File    : test/gtx/euler_angle.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

// Code sample from Filippo Ramaciotti

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <iostream>

namespace test_eulerAngleX
{
	int test()
	{
		int Error = 0;

		float const Angle(glm::pi<float>() * 0.5f);
		glm::vec3 const X(1.0f, 0.0f, 0.0f);

		glm::vec4 const Y(0.0f, 1.0f, 0.0f, 1.0f);
		glm::vec4 const Y1 = glm::rotate(glm::mat4(1.0f), Angle, X) * Y;
		glm::vec4 const Y2 = glm::eulerAngleX(Angle) * Y;
		glm::vec4 const Y3 = glm::eulerAngleXY(Angle, 0.0f) * Y;
		glm::vec4 const Y4 = glm::eulerAngleYX(0.0f, Angle) * Y;
		glm::vec4 const Y5 = glm::eulerAngleXZ(Angle, 0.0f) * Y;
		glm::vec4 const Y6 = glm::eulerAngleZX(0.0f, Angle) * Y;
		glm::vec4 const Y7 = glm::eulerAngleYXZ(0.0f, Angle, 0.0f) * Y;
		Error += glm::all(glm::epsilonEqual(Y1, Y2, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Y1, Y3, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Y1, Y4, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Y1, Y5, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Y1, Y6, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Y1, Y7, 0.00001f)) ? 0 : 1;

		glm::vec4 const Z(0.0f, 0.0f, 1.0f, 1.0f);
		glm::vec4 const Z1 = glm::rotate(glm::mat4(1.0f), Angle, X) * Z;
		glm::vec4 const Z2 = glm::eulerAngleX(Angle) * Z;
		glm::vec4 const Z3 = glm::eulerAngleXY(Angle, 0.0f) * Z;
		glm::vec4 const Z4 = glm::eulerAngleYX(0.0f, Angle) * Z;
		glm::vec4 const Z5 = glm::eulerAngleXZ(Angle, 0.0f) * Z;
		glm::vec4 const Z6 = glm::eulerAngleZX(0.0f, Angle) * Z;
		glm::vec4 const Z7 = glm::eulerAngleYXZ(0.0f, Angle, 0.0f) * Z;
		Error += glm::all(glm::epsilonEqual(Z1, Z2, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Z1, Z3, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Z1, Z4, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Z1, Z5, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Z1, Z6, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Z1, Z7, 0.00001f)) ? 0 : 1;

		return Error;
	}
}//namespace test_eulerAngleX

namespace test_eulerAngleY
{
	int test()
	{
		int Error = 0;

		float const Angle(glm::pi<float>() * 0.5f);
		glm::vec3 const Y(0.0f, 1.0f, 0.0f);

		glm::vec4 const X(1.0f, 0.0f, 0.0f, 1.0f);
		glm::vec4 const X1 = glm::rotate(glm::mat4(1.0f), Angle, Y) * X;
		glm::vec4 const X2 = glm::eulerAngleY(Angle) * X;
		glm::vec4 const X3 = glm::eulerAngleYX(Angle, 0.0f) * X;
		glm::vec4 const X4 = glm::eulerAngleXY(0.0f, Angle) * X;
		glm::vec4 const X5 = glm::eulerAngleYZ(Angle, 0.0f) * X;
		glm::vec4 const X6 = glm::eulerAngleZY(0.0f, Angle) * X;
		glm::vec4 const X7 = glm::eulerAngleYXZ(Angle, 0.0f, 0.0f) * X;
		Error += glm::all(glm::epsilonEqual(X1, X2, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(X1, X3, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(X1, X4, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(X1, X5, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(X1, X6, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(X1, X7, 0.00001f)) ? 0 : 1;

		glm::vec4 const Z(0.0f, 0.0f, 1.0f, 1.0f);
		glm::vec4 const Z1 = glm::eulerAngleY(Angle) * Z;
		glm::vec4 const Z2 = glm::rotate(glm::mat4(1.0f), Angle, Y) * Z;
		glm::vec4 const Z3 = glm::eulerAngleYX(Angle, 0.0f) * Z;
		glm::vec4 const Z4 = glm::eulerAngleXY(0.0f, Angle) * Z;
		glm::vec4 const Z5 = glm::eulerAngleYZ(Angle, 0.0f) * Z;
		glm::vec4 const Z6 = glm::eulerAngleZY(0.0f, Angle) * Z;
		glm::vec4 const Z7 = glm::eulerAngleYXZ(Angle, 0.0f, 0.0f) * Z;
		Error += glm::all(glm::epsilonEqual(Z1, Z2, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Z1, Z3, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Z1, Z4, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Z1, Z5, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Z1, Z6, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Z1, Z7, 0.00001f)) ? 0 : 1;

		return Error;
	}
}//namespace test_eulerAngleY

namespace test_eulerAngleZ
{
	int test()
	{
		int Error = 0;

		float const Angle(glm::pi<float>() * 0.5f);
		glm::vec3 const Z(0.0f, 0.0f, 1.0f);

		glm::vec4 const X(1.0f, 0.0f, 0.0f, 1.0f);
		glm::vec4 const X1 = glm::rotate(glm::mat4(1.0f), Angle, Z) * X;
		glm::vec4 const X2 = glm::eulerAngleZ(Angle) * X;
		glm::vec4 const X3 = glm::eulerAngleZX(Angle, 0.0f) * X;
		glm::vec4 const X4 = glm::eulerAngleXZ(0.0f, Angle) * X;
		glm::vec4 const X5 = glm::eulerAngleZY(Angle, 0.0f) * X;
		glm::vec4 const X6 = glm::eulerAngleYZ(0.0f, Angle) * X;
		glm::vec4 const X7 = glm::eulerAngleYXZ(0.0f, 0.0f, Angle) * X;
		Error += glm::all(glm::epsilonEqual(X1, X2, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(X1, X3, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(X1, X4, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(X1, X5, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(X1, X6, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(X1, X7, 0.00001f)) ? 0 : 1;

		glm::vec4 const Y(1.0f, 0.0f, 0.0f, 1.0f);
		glm::vec4 const Z1 = glm::rotate(glm::mat4(1.0f), Angle, Z) * Y;
		glm::vec4 const Z2 = glm::eulerAngleZ(Angle) * Y;
		glm::vec4 const Z3 = glm::eulerAngleZX(Angle, 0.0f) * Y;
		glm::vec4 const Z4 = glm::eulerAngleXZ(0.0f, Angle) * Y;
		glm::vec4 const Z5 = glm::eulerAngleZY(Angle, 0.0f) * Y;
		glm::vec4 const Z6 = glm::eulerAngleYZ(0.0f, Angle) * Y;
		glm::vec4 const Z7 = glm::eulerAngleYXZ(0.0f, 0.0f, Angle) * Y;
		Error += glm::all(glm::epsilonEqual(Z1, Z2, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Z1, Z3, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Z1, Z4, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Z1, Z5, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Z1, Z6, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Z1, Z7, 0.00001f)) ? 0 : 1;

		return Error;
	}
}//namespace test_eulerAngleZ

namespace test_eulerAngleXY
{
	int test()
	{
		int Error = 0;

		glm::vec4 const V(1.0f);

		float const AngleX(glm::pi<float>() * 0.5f);
		float const AngleY(glm::pi<float>() * 0.25f);

		glm::vec3 const axisX(1.0f, 0.0f, 0.0f);
		glm::vec3 const axisY(0.0f, 1.0f, 0.0f);

		glm::vec4 const V1 = (glm::rotate(glm::mat4(1.0f), AngleX, axisX) * glm::rotate(glm::mat4(1.0f), AngleY, axisY)) * V;
		glm::vec4 const V2 = glm::eulerAngleXY(AngleX, AngleY) * V;
		glm::vec4 const V3 = glm::eulerAngleX(AngleX) * glm::eulerAngleY(AngleY) * V;
		Error += glm::all(glm::epsilonEqual(V1, V2, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(V1, V3, 0.00001f)) ? 0 : 1;

		return Error;
	}
}//namespace test_eulerAngleXY

namespace test_eulerAngleYX
{
	int test()
	{
		int Error = 0;

		glm::vec4 const V(1.0f);

		float const AngleX(glm::pi<float>() * 0.5f);
		float const AngleY(glm::pi<float>() * 0.25f);

		glm::vec3 const axisX(1.0f, 0.0f, 0.0f);
		glm::vec3 const axisY(0.0f, 1.0f, 0.0f);

		glm::vec4 const V1 = (glm::rotate(glm::mat4(1.0f), AngleY, axisY) * glm::rotate(glm::mat4(1.0f), AngleX, axisX)) * V;
		glm::vec4 const V2 = glm::eulerAngleYX(AngleY, AngleX) * V;
		glm::vec4 const V3 = glm::eulerAngleY(AngleY) * glm::eulerAngleX(AngleX) * V;
		Error += glm::all(glm::epsilonEqual(V1, V2, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(V1, V3, 0.00001f)) ? 0 : 1;

		return Error;
	}
}//namespace test_eulerAngleYX

namespace test_eulerAngleXZ
{
	int test()
	{
		int Error = 0;

		glm::vec4 const V(1.0f);

		float const AngleX(glm::pi<float>() * 0.5f);
		float const AngleZ(glm::pi<float>() * 0.25f);

		glm::vec3 const axisX(1.0f, 0.0f, 0.0f);
		glm::vec3 const axisZ(0.0f, 0.0f, 1.0f);

		glm::vec4 const V1 = (glm::rotate(glm::mat4(1.0f), AngleX, axisX) * glm::rotate(glm::mat4(1.0f), AngleZ, axisZ)) * V;
		glm::vec4 const V2 = glm::eulerAngleXZ(AngleX, AngleZ) * V;
		glm::vec4 const V3 = glm::eulerAngleX(AngleX) * glm::eulerAngleZ(AngleZ) * V;
		Error += glm::all(glm::epsilonEqual(V1, V2, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(V1, V3, 0.00001f)) ? 0 : 1;

		return Error;
	}
}//namespace test_eulerAngleXZ

namespace test_eulerAngleZX
{
	int test()
	{
		int Error = 0;

		glm::vec4 const V(1.0f);

		float const AngleX(glm::pi<float>() * 0.5f);
		float const AngleZ(glm::pi<float>() * 0.25f);

		glm::vec3 const axisX(1.0f, 0.0f, 0.0f);
		glm::vec3 const axisZ(0.0f, 0.0f, 1.0f);

		glm::vec4 const V1 = (glm::rotate(glm::mat4(1.0f), AngleZ, axisZ) * glm::rotate(glm::mat4(1.0f), AngleX, axisX)) * V;
		glm::vec4 const V2 = glm::eulerAngleZX(AngleZ, AngleX) * V;
		glm::vec4 const V3 = glm::eulerAngleZ(AngleZ) * glm::eulerAngleX(AngleX) * V;
		Error += glm::all(glm::epsilonEqual(V1, V2, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(V1, V3, 0.00001f)) ? 0 : 1;

		return Error;
	}
}//namespace test_eulerAngleZX

namespace test_eulerAngleYZ
{
	int test()
	{
		int Error = 0;

		glm::vec4 const V(1.0f);

		float const AngleY(glm::pi<float>() * 0.5f);
		float const AngleZ(glm::pi<float>() * 0.25f);

		glm::vec3 const axisX(1.0f, 0.0f, 0.0f);
		glm::vec3 const axisY(0.0f, 1.0f, 0.0f);
		glm::vec3 const axisZ(0.0f, 0.0f, 1.0f);

		glm::vec4 const V1 = (glm::rotate(glm::mat4(1.0f), AngleY, axisY) * glm::rotate(glm::mat4(1.0f), AngleZ, axisZ)) * V;
		glm::vec4 const V2 = glm::eulerAngleYZ(AngleY, AngleZ) * V;
		glm::vec4 const V3 = glm::eulerAngleY(AngleY) * glm::eulerAngleZ(AngleZ) * V;
		Error += glm::all(glm::epsilonEqual(V1, V2, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(V1, V3, 0.00001f)) ? 0 : 1;

		return Error;
	}
}//namespace test_eulerAngleYZ

namespace test_eulerAngleZY
{
	int test()
	{
		int Error = 0;

		glm::vec4 const V(1.0f);

		float const AngleY(glm::pi<float>() * 0.5f);
		float const AngleZ(glm::pi<float>() * 0.25f);

		glm::vec3 const axisX(1.0f, 0.0f, 0.0f);
		glm::vec3 const axisY(0.0f, 1.0f, 0.0f);
		glm::vec3 const axisZ(0.0f, 0.0f, 1.0f);

		glm::vec4 const V1 = (glm::rotate(glm::mat4(1.0f), AngleZ, axisZ) * glm::rotate(glm::mat4(1.0f), AngleY, axisY)) * V;
		glm::vec4 const V2 = glm::eulerAngleZY(AngleZ, AngleY) * V;
		glm::vec4 const V3 = glm::eulerAngleZ(AngleZ) * glm::eulerAngleY(AngleY) * V;
		Error += glm::all(glm::epsilonEqual(V1, V2, 0.00001f)) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(V1, V3, 0.00001f)) ? 0 : 1;

		return Error;
	}
}//namespace test_eulerAngleZY

namespace test_eulerAngleYXZ
{
	int test()
	{
		glm::f32 first =  1.046f;
		glm::f32 second = 0.52f;
		glm::f32 third = -0.785f;

		glm::fmat4 rotationEuler = glm::eulerAngleYXZ(first, second, third); 

		glm::fmat4 rotationInvertedY  = glm::eulerAngleY(-1.f*first) * glm::eulerAngleX(second) * glm::eulerAngleZ(third); 
		glm::fmat4 rotationDumb = glm::fmat4(); 
		rotationDumb = glm::rotate(rotationDumb, first, glm::fvec3(0,1,0)); 
		rotationDumb = glm::rotate(rotationDumb, second, glm::fvec3(1,0,0)); 
		rotationDumb = glm::rotate(rotationDumb, third, glm::fvec3(0,0,1)); 

		std::cout << glm::to_string(glm::fmat3(rotationEuler)) << std::endl; 
		std::cout << glm::to_string(glm::fmat3(rotationDumb)) << std::endl; 
		std::cout << glm::to_string(glm::fmat3(rotationInvertedY )) << std::endl; 

		std::cout <<"\nRESIDUAL\n"; 
		std::cout << glm::to_string(glm::fmat3(rotationEuler-(rotationDumb))) << std::endl; 
		std::cout << glm::to_string(glm::fmat3(rotationEuler-(rotationInvertedY ))) << std::endl;

		return 0;
	}
}//namespace eulerAngleYXZ

int main()
{ 
	int Error = 0;

	Error += test_eulerAngleX::test();
	Error += test_eulerAngleY::test();
	Error += test_eulerAngleZ::test();
	Error += test_eulerAngleXY::test();
	Error += test_eulerAngleYX::test();
	Error += test_eulerAngleXZ::test();
	Error += test_eulerAngleZX::test();
	Error += test_eulerAngleYZ::test();
	Error += test_eulerAngleZY::test();
	Error += test_eulerAngleYXZ::test();

	return Error; 
}
