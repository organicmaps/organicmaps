///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2008-08-31
// Updated : 2013-12-24
// Licence : This source is under MIT License
// File    : test/core/type_vec4.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#define GLM_FORCE_RADIANS
#include <glm/vector_relational.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <cstdio>
#include <ctime>
#include <vector>

template <int Value>
struct mask
{
	enum{value = Value};
};

enum comp
{
	X,
	Y,
	Z,
	W
};

//template<comp X, comp Y, comp Z, comp W>
//__m128 swizzle(glm::vec4 const & v)
//{
//	__m128 Src = _mm_set_ps(v.w, v.z, v.y, v.x);
//	return _mm_shuffle_ps(Src, Src, mask<(int(W) << 6) | (int(Z) << 4) | (int(Y) << 2) | (int(X) << 0)>::value);
//}


int test_vec4_ctor()
{
	int Error = 0;
	
#if(GLM_HAS_INITIALIZER_LISTS)
	{
		glm::vec4 a{ 0, 1, 2, 3 };
		std::vector<glm::vec4> v = {
			{0, 1, 2, 3},
			{4, 5, 6, 7},
			{8, 9, 0, 1}};
	}

	{
		glm::dvec4 a{ 0, 1, 2, 3 };
		std::vector<glm::dvec4> v = {
			{0, 1, 2, 3},
			{4, 5, 6, 7},
			{8, 9, 0, 1}};
	}
#endif

	{
		glm::vec4 A(1);
		glm::vec4 B(1, 1, 1, 1);
		
		Error += A == B ? 0 : 1;
	}
	
	{
		std::vector<glm::vec4> Tests;
		Tests.push_back(glm::vec4(glm::vec2(1, 2), 3, 4));
		Tests.push_back(glm::vec4(1, glm::vec2(2, 3), 4));
		Tests.push_back(glm::vec4(1, 2, glm::vec2(3, 4)));
		Tests.push_back(glm::vec4(glm::vec3(1, 2, 3), 4));
		Tests.push_back(glm::vec4(1, glm::vec3(2, 3, 4)));
		Tests.push_back(glm::vec4(glm::vec2(1, 2), glm::vec2(3, 4)));
		Tests.push_back(glm::vec4(1, 2, 3, 4));
		Tests.push_back(glm::vec4(glm::vec4(1, 2, 3, 4)));
		
		for(std::size_t i = 0; i < Tests.size(); ++i)
			Error += Tests[i] == glm::vec4(1, 2, 3, 4) ? 0 : 1;
	}
	
	return Error;
}

int test_vec4_operators()
{
	int Error = 0;
	
	{
		glm::vec4 A(1.0f);
		glm::vec4 B(1.0f);
		bool R = A != B;
		bool S = A == B;

		Error += (S && !R) ? 0 : 1;
	}

	{
		glm::vec4 A(1.0f, 2.0f, 3.0f, 4.0f);
		glm::vec4 B(4.0f, 5.0f, 6.0f, 7.0f);

		glm::vec4 C = A + B;
		Error += C == glm::vec4(5, 7, 9, 11) ? 0 : 1;

		glm::vec4 D = B - A;
		Error += D == glm::vec4(3, 3, 3, 3) ? 0 : 1;

		glm::vec4 E = A * B;
		Error += E == glm::vec4(4, 10, 18, 28) ? 0 : 1;

		glm::vec4 F = B / A;
		Error += F == glm::vec4(4, 2.5, 2, 7.0f / 4.0f) ? 0 : 1;

		glm::vec4 G = A + 1.0f;
		Error += G == glm::vec4(2, 3, 4, 5) ? 0 : 1;

		glm::vec4 H = B - 1.0f;
		Error += H == glm::vec4(3, 4, 5, 6) ? 0 : 1;

		glm::vec4 I = A * 2.0f;
		Error += I == glm::vec4(2, 4, 6, 8) ? 0 : 1;

		glm::vec4 J = B / 2.0f;
		Error += J == glm::vec4(2, 2.5, 3, 3.5) ? 0 : 1;

		glm::vec4 K = 1.0f + A;
		Error += K == glm::vec4(2, 3, 4, 5) ? 0 : 1;

		glm::vec4 L = 1.0f - B;
		Error += L == glm::vec4(-3, -4, -5, -6) ? 0 : 1;

		glm::vec4 M = 2.0f * A;
		Error += M == glm::vec4(2, 4, 6, 8) ? 0 : 1;

		glm::vec4 N = 2.0f / B;
		Error += N == glm::vec4(0.5, 2.0 / 5.0, 2.0 / 6.0, 2.0 / 7.0) ? 0 : 1;
	}

	{
		glm::vec4 A(1.0f, 2.0f, 3.0f, 4.0f);
		glm::vec4 B(4.0f, 5.0f, 6.0f, 7.0f);

		A += B;
		Error += A == glm::vec4(5, 7, 9, 11) ? 0 : 1;

		A += 1.0f;
		Error += A == glm::vec4(6, 8, 10, 12) ? 0 : 1;
	}
	{
		glm::vec4 A(1.0f, 2.0f, 3.0f, 4.0f);
		glm::vec4 B(4.0f, 5.0f, 6.0f, 7.0f);

		B -= A;
		Error += B == glm::vec4(3, 3, 3, 3) ? 0 : 1;

		B -= 1.0f;
		Error += B == glm::vec4(2, 2, 2, 2) ? 0 : 1;
	}
	{
		glm::vec4 A(1.0f, 2.0f, 3.0f, 4.0f);
		glm::vec4 B(4.0f, 5.0f, 6.0f, 7.0f);

		A *= B;
		Error += A == glm::vec4(4, 10, 18, 28) ? 0 : 1;

		A *= 2.0f;
		Error += A == glm::vec4(8, 20, 36, 56) ? 0 : 1;
	}
	{
		glm::vec4 A(1.0f, 2.0f, 3.0f, 4.0f);
		glm::vec4 B(4.0f, 5.0f, 6.0f, 7.0f);

		B /= A;
		Error += B == glm::vec4(4, 2.5, 2, 7.0f / 4.0f) ? 0 : 1;

		B /= 2.0f;
		Error += B == glm::vec4(2, 1.25, 1, 7.0f / 4.0f / 2.0f) ? 0 : 1;
	}
	{
		glm::vec4 B(2.0f);

		B /= B.y;
		Error += B == glm::vec4(1.0f) ? 0 : 1;
	}

	{
		glm::vec4 A(1.0f, 2.0f, 3.0f, 4.0f);
		glm::vec4 B = -A;
		Error += B == glm::vec4(-1.0f, -2.0f, -3.0f, -4.0f) ? 0 : 1;
	}

	{
		glm::vec4 A(1.0f, 2.0f, 3.0f, 4.0f);
		glm::vec4 B = --A;
		Error += B == glm::vec4(0.0f, 1.0f, 2.0f, 3.0f) ? 0 : 1;
	}

	{
		glm::vec4 A(1.0f, 2.0f, 3.0f, 4.0f);
		glm::vec4 B = A--;
		Error += B == glm::vec4(1.0f, 2.0f, 3.0f, 4.0f) ? 0 : 1;
		Error += A == glm::vec4(0.0f, 1.0f, 2.0f, 3.0f) ? 0 : 1;
	}

	{
		glm::vec4 A(1.0f, 2.0f, 3.0f, 4.0f);
		glm::vec4 B = ++A;
		Error += B == glm::vec4(2.0f, 3.0f, 4.0f, 5.0f) ? 0 : 1;
	}

	{
		glm::vec4 A(1.0f, 2.0f, 3.0f, 4.0f);
		glm::vec4 B = A++;
		Error += B == glm::vec4(1.0f, 2.0f, 3.0f, 4.0f) ? 0 : 1;
		Error += A == glm::vec4(2.0f, 3.0f, 4.0f, 5.0f) ? 0 : 1;
	}

	return Error;
}

int test_vec4_size()
{
	int Error = 0;
	
	Error += sizeof(glm::vec4) == sizeof(glm::lowp_vec4) ? 0 : 1;
	Error += sizeof(glm::vec4) == sizeof(glm::mediump_vec4) ? 0 : 1;
	Error += sizeof(glm::vec4) == sizeof(glm::highp_vec4) ? 0 : 1;
	Error += 16 == sizeof(glm::mediump_vec4) ? 0 : 1;
	Error += sizeof(glm::dvec4) == sizeof(glm::lowp_dvec4) ? 0 : 1;
	Error += sizeof(glm::dvec4) == sizeof(glm::mediump_dvec4) ? 0 : 1;
	Error += sizeof(glm::dvec4) == sizeof(glm::highp_dvec4) ? 0 : 1;
	Error += 32 == sizeof(glm::highp_dvec4) ? 0 : 1;
	Error += glm::vec4().length() == 4 ? 0 : 1;
	Error += glm::dvec4().length() == 4 ? 0 : 1;
	
	return Error;
}

int test_vec4_swizzle_partial()
{
	int Error = 0;

	glm::vec4 A(1, 2, 3, 4);

#	if(GLM_HAS_ANONYMOUS_UNION && defined(GLM_SWIZZLE_RELAX))
	{
		glm::vec4 B(A.xy, A.zw);
		Error += A == B ? 0 : 1;
	}
	{
		glm::vec4 B(A.xy, 3.0f, 4.0f);
		Error += A == B ? 0 : 1;
	}
	{
		glm::vec4 B(1.0f, A.yz, 4.0f);
		Error += A == B ? 0 : 1;
	}
	{
		glm::vec4 B(1.0f, 2.0f, A.zw);
		Error += A == B ? 0 : 1;
	}

	{
		glm::vec4 B(A.xyz, 4.0f);
		Error += A == B ? 0 : 1;
	}
	{
		glm::vec4 B(1.0f, A.yzw);
		Error += A == B ? 0 : 1;
	}
#	endif

	return Error;
}

int test_operator_increment()
{
	int Error(0);

	glm::ivec4 v0(1);
	glm::ivec4 v1(v0);
	glm::ivec4 v2(v0);
	glm::ivec4 v3 = ++v1;
	glm::ivec4 v4 = v2++;

	Error += glm::all(glm::equal(v0, v4)) ? 0 : 1;
	Error += glm::all(glm::equal(v1, v2)) ? 0 : 1;
	Error += glm::all(glm::equal(v1, v3)) ? 0 : 1;

	int i0(1);
	int i1(i0);
	int i2(i0);
	int i3 = ++i1;
	int i4 = i2++;

	Error += i0 == i4 ? 0 : 1;
	Error += i1 == i2 ? 0 : 1;
	Error += i1 == i3 ? 0 : 1;

	return Error;
}

struct AoS
{
	glm::vec4 A;
	glm::vec3 B;
	glm::vec3 C;
	glm::vec2 D;
};

int test_vec4_perf_AoS(std::size_t Size)
{
	int Error(0);

	std::vector<AoS> In;
	std::vector<AoS> Out;
	In.resize(Size);
	Out.resize(Size);

	std::clock_t StartTime = std::clock();

	for(std::size_t i = 0; i < In.size(); ++i)
		Out[i] = In[i];

	std::clock_t EndTime = std::clock();

  std::printf("AoS: %d\n", EndTime - StartTime);

	return Error;
}

int test_vec4_perf_SoA(std::size_t Size)
{
	int Error(0);

	std::vector<glm::vec4> InA;
	std::vector<glm::vec3> InB;
	std::vector<glm::vec3> InC;
	std::vector<glm::vec2> InD;
	std::vector<glm::vec4> OutA;
	std::vector<glm::vec3> OutB;
	std::vector<glm::vec3> OutC;
	std::vector<glm::vec2> OutD;

	InA.resize(Size);
	InB.resize(Size);
	InC.resize(Size);
	InD.resize(Size);
	OutA.resize(Size);
	OutB.resize(Size);
	OutC.resize(Size);
	OutD.resize(Size);

	std::clock_t StartTime = std::clock();

	for(std::size_t i = 0; i < InA.size(); ++i)
	{
		OutA[i] = InA[i];
		OutB[i] = InB[i];
		OutC[i] = InC[i];
		OutD[i] = InD[i];
	}

	std::clock_t EndTime = std::clock();

	std::printf("SoA: %d\n", EndTime - StartTime);

	return Error;
}

int main()
{
	int Error(0);

	std::size_t const Size(1000000);

	Error += test_vec4_perf_AoS(Size);
	Error += test_vec4_perf_SoA(Size);
	Error += test_vec4_ctor();
	Error += test_vec4_size();
	Error += test_vec4_operators();
	Error += test_vec4_swizzle_partial();
	Error += test_operator_increment();

	return Error;
}

