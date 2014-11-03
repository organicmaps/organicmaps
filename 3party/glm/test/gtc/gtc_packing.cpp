///////////////////////////////////////////////////////////////////////////////////
/// OpenGL Mathematics (glm.g-truc.net)
///
/// Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
/// 
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
/// 
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
/// THE SOFTWARE.
///
/// @ref test
/// @file test/gtc/packing.cpp
/// @date 2013-08-09 / 2013-08-09
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#define GLM_FORCE_RADIANS
#include <glm/gtc/packing.hpp>
#include <cstdio>
#include <vector>

void print_bits(float const & s)
{
	union
	{
		float f;
		unsigned int i;
	} uif;

	uif.f = s;

	printf("f32: ");
	for(std::size_t j = sizeof(s) * 8; j > 0; --j)
	{
		if(j == 23 || j == 31)
			printf(" ");
		printf("%d", (uif.i & (1 << (j - 1))) ? 1 : 0);
	}
}

void print_10bits(glm::uint const & s)
{
	printf("10b: ");
	for(std::size_t j = 10; j > 0; --j)
	{
		if(j == 5)
			printf(" ");
		printf("%d", (s & (1 << (j - 1))) ? 1 : 0);
	}
}

void print_11bits(glm::uint const & s)
{
	printf("11b: ");
	for(std::size_t j = 11; j > 0; --j)
	{
		if(j == 6)
			printf(" ");
		printf("%d", (s & (1 << (j - 1))) ? 1 : 0);
	}
}

void print_value(float const & s)
{
	printf("%2.5f, ", s);
	print_bits(s);
	printf(", ");
//	print_11bits(detail::floatTo11bit(s));
//	printf(", ");
//	print_10bits(detail::floatTo10bit(s));
	printf("\n");
}

int test_Half1x16()
{
	int Error = 0;

	std::vector<float> Tests;
	Tests.push_back(0.0f);
	Tests.push_back(1.0f);
	Tests.push_back(-1.0f);
	Tests.push_back(2.0f);
	Tests.push_back(-2.0f);
	Tests.push_back(1.9f);

	for(std::size_t i = 0; i < Tests.size(); ++i)
	{
		glm::uint32 p0 = glm::packHalf1x16(Tests[i]);
		float v0 = glm::unpackHalf1x16(p0);
		glm::uint32 p1 = glm::packHalf1x16(v0);
		float v1 = glm::unpackHalf1x16(p1);
		Error += (v0 == v1) ? 0 : 1;
	}

	return Error;
}

int test_Half4x16()
{
	int Error = 0;

	std::vector<glm::vec4> Tests;
	Tests.push_back(glm::vec4(1.0f));
	Tests.push_back(glm::vec4(0.0f));
	Tests.push_back(glm::vec4(2.0f));
	Tests.push_back(glm::vec4(0.1f));
	Tests.push_back(glm::vec4(0.5f));
	Tests.push_back(glm::vec4(-0.9f));

	for(std::size_t i = 0; i < Tests.size(); ++i)
	{
		glm::uint64 p0 = glm::packHalf4x16(Tests[i]);
		glm::vec4 v0 = glm::unpackHalf4x16(p0);
		glm::uint64 p1 = glm::packHalf4x16(v0);
		glm::vec4 v1 = glm::unpackHalf4x16(p1);
		Error += glm::all(glm::equal(v0, v1)) ? 0 : 1;
	}

	return Error;
}

int test_I3x10_1x2()
{
	int Error = 0;

	std::vector<glm::ivec4> Tests;
	Tests.push_back(glm::ivec4(0));
	Tests.push_back(glm::ivec4(1));
	Tests.push_back(glm::ivec4(-1));
	Tests.push_back(glm::ivec4(2));
	Tests.push_back(glm::ivec4(-2));
	Tests.push_back(glm::ivec4(3));

	for(std::size_t i = 0; i < Tests.size(); ++i)
	{
		glm::uint32 p0 = glm::packI3x10_1x2(Tests[i]);
		glm::ivec4 v0 = glm::unpackI3x10_1x2(p0);
		glm::uint32 p1 = glm::packI3x10_1x2(v0);
		glm::ivec4 v1 = glm::unpackI3x10_1x2(p1);
		Error += glm::all(glm::equal(v0, v1)) ? 0 : 1;
	}

	return Error;
}

int test_U3x10_1x2()
{
	int Error = 0;

	std::vector<glm::uvec4> Tests;
	Tests.push_back(glm::uvec4(0));
	Tests.push_back(glm::uvec4(1));
	Tests.push_back(glm::uvec4(2));
	Tests.push_back(glm::uvec4(3));
	Tests.push_back(glm::uvec4(4));
	Tests.push_back(glm::uvec4(5));

	for(std::size_t i = 0; i < Tests.size(); ++i)
	{
		glm::uint32 p0 = glm::packU3x10_1x2(Tests[i]);
		glm::uvec4 v0 = glm::unpackU3x10_1x2(p0);
		glm::uint32 p1 = glm::packU3x10_1x2(v0);
		glm::uvec4 v1 = glm::unpackU3x10_1x2(p1);
		Error += glm::all(glm::equal(v0, v1)) ? 0 : 1;
	}

	return Error;
}

int test_Snorm3x10_1x2()
{
	int Error = 0;

	std::vector<glm::vec4> Tests;
	Tests.push_back(glm::vec4(1.0f));
	Tests.push_back(glm::vec4(0.0f));
	Tests.push_back(glm::vec4(2.0f));
	Tests.push_back(glm::vec4(0.1f));
	Tests.push_back(glm::vec4(0.5f));
	Tests.push_back(glm::vec4(0.9f));

	for(std::size_t i = 0; i < Tests.size(); ++i)
	{
		glm::uint32 p0 = glm::packSnorm3x10_1x2(Tests[i]);
		glm::vec4 v0 = glm::unpackSnorm3x10_1x2(p0);
		glm::uint32 p1 = glm::packSnorm3x10_1x2(v0);
		glm::vec4 v1 = glm::unpackSnorm3x10_1x2(p1);
		Error += glm::all(glm::equal(v0, v1)) ? 0 : 1;
	}

	return Error;
}

int test_Unorm3x10_1x2()
{
	int Error = 0;

	std::vector<glm::vec4> Tests;
	Tests.push_back(glm::vec4(1.0f));
	Tests.push_back(glm::vec4(0.0f));
	Tests.push_back(glm::vec4(2.0f));
	Tests.push_back(glm::vec4(0.1f));
	Tests.push_back(glm::vec4(0.5f));
	Tests.push_back(glm::vec4(0.9f));

	for(std::size_t i = 0; i < Tests.size(); ++i)
	{
		glm::uint32 p0 = glm::packSnorm3x10_1x2(Tests[i]);
		glm::vec4 v0 = glm::unpackSnorm3x10_1x2(p0);
		glm::uint32 p1 = glm::packSnorm3x10_1x2(v0);
		glm::vec4 v1 = glm::unpackSnorm3x10_1x2(p1);
		Error += glm::all(glm::equal(v0, v1)) ? 0 : 1;
	}

	return Error;
}

int test_F2x11_1x10()
{
	int Error = 0;

	std::vector<glm::vec3> Tests;
	Tests.push_back(glm::vec3(1.0f));
	Tests.push_back(glm::vec3(0.0f));
	Tests.push_back(glm::vec3(2.0f));
	Tests.push_back(glm::vec3(0.1f));
	Tests.push_back(glm::vec3(0.5f));
	Tests.push_back(glm::vec3(0.9f));

	for(std::size_t i = 0; i < Tests.size(); ++i)
	{
		glm::uint32 p0 = glm::packF2x11_1x10(Tests[i]);
		glm::vec3 v0 = glm::unpackF2x11_1x10(p0);
		glm::uint32 p1 = glm::packF2x11_1x10(v0);
		glm::vec3 v1 = glm::unpackF2x11_1x10(p1);
		Error += glm::all(glm::equal(v0, v1)) ? 0 : 1;
	}

	return Error;
}

int main()
{
	int Error(0);

	Error += test_F2x11_1x10();
	Error += test_Snorm3x10_1x2();
	Error += test_Unorm3x10_1x2();
	Error += test_I3x10_1x2();
	Error += test_U3x10_1x2();
	Error += test_Half1x16();
	Error += test_U3x10_1x2();

	return Error;
}
