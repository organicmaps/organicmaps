///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2011-01-15
// Updated : 2011-09-13
// Licence : This source is under MIT licence
// File    : test/core/func_common.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

//#include <boost/array.hpp>
//#include <boost/date_time/posix_time/posix_time.hpp>
//#include <boost/thread/thread.hpp>
#define GLM_FORCE_RADIANS
#include <glm/gtc/constants.hpp>
#include <glm/gtc/epsilon.hpp>
#include <cstdio>
#include <cmath>

int test_modf()
{
	int Error(0);

	{
		float X(1.5f);
		float I(0.0f);
		float A = glm::modf(X, I);

		Error += I == 1.0f ? 0 : 1;
		Error += A == 0.5f ? 0 : 1;
	}

	{
		glm::vec4 X(1.1f, 1.2f, 1.5f, 1.7f);
		glm::vec4 I(0.0f);
		glm::vec4 A = glm::modf(X, I);

		Error += I == glm::vec4(1.0f) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(A, glm::vec4(0.1f, 0.2f, 0.5f, 0.7f), 0.00001f)) ? 0 : 1;
	}

	{
		glm::dvec4 X(1.1, 1.2, 1.5, 1.7);
		glm::dvec4 I(0.0);
		glm::dvec4 A = glm::modf(X, I);

		Error += I == glm::dvec4(1.0) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(A, glm::dvec4(0.1, 0.2, 0.5, 0.7), 0.000000001)) ? 0 : 1;
	}

	{
		double X(1.5);
		double I(0.0);
		double A = glm::modf(X, I);

		Error += I == 1.0 ? 0 : 1;
		Error += A == 0.5 ? 0 : 1;
	}

	return Error;
}

int test_floatBitsToInt()
{
	int Error = 0;
	
	{
		float A = 1.0f;
		int B = glm::floatBitsToInt(A);
		float C = glm::intBitsToFloat(B);
		int D = *(int*)&A;
		Error += B == D ? 0 : 1;
		Error += A == C ? 0 : 1;
	}

	{
		glm::vec2 A(1.0f, 2.0f);
		glm::ivec2 B = glm::floatBitsToInt(A);
		glm::vec2 C = glm::intBitsToFloat(B);
		Error += B.x == *(int*)&(A.x) ? 0 : 1;
		Error += B.y == *(int*)&(A.y) ? 0 : 1;
		Error += A == C? 0 : 1;
	}

	{
		glm::vec3 A(1.0f, 2.0f, 3.0f);
		glm::ivec3 B = glm::floatBitsToInt(A);
		glm::vec3 C = glm::intBitsToFloat(B);
		Error += B.x == *(int*)&(A.x) ? 0 : 1;
		Error += B.y == *(int*)&(A.y) ? 0 : 1;
		Error += B.z == *(int*)&(A.z) ? 0 : 1;
		Error += A == C? 0 : 1;
	}
	
	{
		glm::vec4 A(1.0f, 2.0f, 3.0f, 4.0f);
		glm::ivec4 B = glm::floatBitsToInt(A);
		glm::vec4 C = glm::intBitsToFloat(B);
		Error += B.x == *(int*)&(A.x) ? 0 : 1;
		Error += B.y == *(int*)&(A.y) ? 0 : 1;
		Error += B.z == *(int*)&(A.z) ? 0 : 1;
		Error += B.w == *(int*)&(A.w) ? 0 : 1;
		Error += A == C? 0 : 1;
	}
	
	return Error;
}

int test_floatBitsToUint()
{
	int Error = 0;
	
	{
		float A = 1.0f;
		glm::uint B = glm::floatBitsToUint(A);
		float C = glm::intBitsToFloat(B);
		Error += B == *(glm::uint*)&A ? 0 : 1;
		Error += A == C? 0 : 1;
	}
	
	{
		glm::vec2 A(1.0f, 2.0f);
		glm::uvec2 B = glm::floatBitsToUint(A);
		glm::vec2 C = glm::uintBitsToFloat(B);
		Error += B.x == *(glm::uint*)&(A.x) ? 0 : 1;
		Error += B.y == *(glm::uint*)&(A.y) ? 0 : 1;
		Error += A == C ? 0 : 1;
	}
	
	{
		glm::vec3 A(1.0f, 2.0f, 3.0f);
		glm::uvec3 B = glm::floatBitsToUint(A);
		glm::vec3 C = glm::uintBitsToFloat(B);
		Error += B.x == *(glm::uint*)&(A.x) ? 0 : 1;
		Error += B.y == *(glm::uint*)&(A.y) ? 0 : 1;
		Error += B.z == *(glm::uint*)&(A.z) ? 0 : 1;
		Error += A == C? 0 : 1;
	}
	
	{
		glm::vec4 A(1.0f, 2.0f, 3.0f, 4.0f);
		glm::uvec4 B = glm::floatBitsToUint(A);
		glm::vec4 C = glm::uintBitsToFloat(B);
		Error += B.x == *(glm::uint*)&(A.x) ? 0 : 1;
		Error += B.y == *(glm::uint*)&(A.y) ? 0 : 1;
		Error += B.z == *(glm::uint*)&(A.z) ? 0 : 1;
		Error += B.w == *(glm::uint*)&(A.w) ? 0 : 1;
		Error += A == C? 0 : 1;
	}
	
	return Error;
}

namespace test_mix
{
	template <typename T, typename B>
	struct test
	{
		T x;
		T y;
		B a;
		T Result;
	};

	test<float, bool> TestBool[] = 
	{
		{0.0f, 1.0f, false, 0.0f},
		{0.0f, 1.0f, true, 1.0f},
		{-1.0f, 1.0f, false, -1.0f},
		{-1.0f, 1.0f, true, 1.0f}
	};

	test<float, float> TestFloat[] = 
	{
		{0.0f, 1.0f, 0.0f, 0.0f},
		{0.0f, 1.0f, 1.0f, 1.0f},
		{-1.0f, 1.0f, 0.0f, -1.0f},
		{-1.0f, 1.0f, 1.0f, 1.0f}
	};

	test<glm::vec2, bool> TestVec2Bool[] = 
	{
		{glm::vec2(0.0f), glm::vec2(1.0f), false, glm::vec2(0.0f)},
		{glm::vec2(0.0f), glm::vec2(1.0f), true, glm::vec2(1.0f)},
		{glm::vec2(-1.0f), glm::vec2(1.0f), false, glm::vec2(-1.0f)},
		{glm::vec2(-1.0f), glm::vec2(1.0f), true, glm::vec2(1.0f)}
	};

	test<glm::vec2, glm::bvec2> TestBVec2[] = 
	{
		{glm::vec2(0.0f), glm::vec2(1.0f), glm::bvec2(false), glm::vec2(0.0f)},
		{glm::vec2(0.0f), glm::vec2(1.0f), glm::bvec2(true), glm::vec2(1.0f)},
		{glm::vec2(-1.0f), glm::vec2(1.0f), glm::bvec2(false), glm::vec2(-1.0f)},
		{glm::vec2(-1.0f), glm::vec2(1.0f), glm::bvec2(true), glm::vec2(1.0f)},
		{glm::vec2(-1.0f), glm::vec2(1.0f), glm::bvec2(true, false), glm::vec2(1.0f, -1.0f)}
	};

	test<glm::vec3, bool> TestVec3Bool[] = 
	{
		{glm::vec3(0.0f), glm::vec3(1.0f), false, glm::vec3(0.0f)},
		{glm::vec3(0.0f), glm::vec3(1.0f), true, glm::vec3(1.0f)},
		{glm::vec3(-1.0f), glm::vec3(1.0f), false, glm::vec3(-1.0f)},
		{glm::vec3(-1.0f), glm::vec3(1.0f), true, glm::vec3(1.0f)}
	};

	test<glm::vec3, glm::bvec3> TestBVec3[] = 
	{
		{glm::vec3(0.0f), glm::vec3(1.0f), glm::bvec3(false), glm::vec3(0.0f)},
		{glm::vec3(0.0f), glm::vec3(1.0f), glm::bvec3(true), glm::vec3(1.0f)},
		{glm::vec3(-1.0f), glm::vec3(1.0f), glm::bvec3(false), glm::vec3(-1.0f)},
		{glm::vec3(-1.0f), glm::vec3(1.0f), glm::bvec3(true), glm::vec3(1.0f)},
		{glm::vec3(1.0f, 2.0f, 3.0f), glm::vec3(4.0f, 5.0f, 6.0f), glm::bvec3(true, false, true), glm::vec3(4.0f, 2.0f, 6.0f)}
	};

	test<glm::vec4, bool> TestVec4Bool[] = 
	{
		{glm::vec4(0.0f), glm::vec4(1.0f), false, glm::vec4(0.0f)},
		{glm::vec4(0.0f), glm::vec4(1.0f), true, glm::vec4(1.0f)},
		{glm::vec4(-1.0f), glm::vec4(1.0f), false, glm::vec4(-1.0f)},
		{glm::vec4(-1.0f), glm::vec4(1.0f), true, glm::vec4(1.0f)}
	};

	test<glm::vec4, glm::bvec4> TestBVec4[] = 
	{
		{glm::vec4(0.0f), glm::vec4(1.0f), glm::bvec4(false), glm::vec4(0.0f)},
		{glm::vec4(0.0f), glm::vec4(1.0f), glm::bvec4(true), glm::vec4(1.0f)},
		{glm::vec4(-1.0f), glm::vec4(1.0f), glm::bvec4(false), glm::vec4(-1.0f)},
		{glm::vec4(-1.0f), glm::vec4(1.0f), glm::bvec4(true), glm::vec4(1.0f)},
		{glm::vec4(1.0f, 2.0f, 3.0f, 4.0f), glm::vec4(5.0f, 6.0f, 7.0f, 8.0f), glm::bvec4(true, false, true, false), glm::vec4(5.0f, 2.0f, 7.0f, 4.0f)}
	};

	int run()
	{
		int Error = 0;

		// Float with bool
		{
			for(std::size_t i = 0; i < sizeof(TestBool) / sizeof(test<float, bool>); ++i)
			{
				float Result = glm::mix(TestBool[i].x, TestBool[i].y, TestBool[i].a);
				Error += glm::epsilonEqual(Result, TestBool[i].Result, glm::epsilon<float>()) ? 0 : 1;
			}
		}

		// Float with float
		{
			for(std::size_t i = 0; i < sizeof(TestFloat) / sizeof(test<float, float>); ++i)
			{
				float Result = glm::mix(TestFloat[i].x, TestFloat[i].y, TestFloat[i].a);
				Error += glm::epsilonEqual(Result, TestFloat[i].Result, glm::epsilon<float>()) ? 0 : 1;
			}
		}

		// vec2 with bool
		{
			for(std::size_t i = 0; i < sizeof(TestVec2Bool) / sizeof(test<glm::vec2, bool>); ++i)
			{
				glm::vec2 Result = glm::mix(TestVec2Bool[i].x, TestVec2Bool[i].y, TestVec2Bool[i].a);
				Error += glm::epsilonEqual(Result.x, TestVec2Bool[i].Result.x, glm::epsilon<float>()) ? 0 : 1;
				Error += glm::epsilonEqual(Result.y, TestVec2Bool[i].Result.y, glm::epsilon<float>()) ? 0 : 1;
			}
		}

		// vec2 with bvec2
		{
			for(std::size_t i = 0; i < sizeof(TestBVec2) / sizeof(test<glm::vec2, glm::bvec2>); ++i)
			{
				glm::vec2 Result = glm::mix(TestBVec2[i].x, TestBVec2[i].y, TestBVec2[i].a);
				Error += glm::epsilonEqual(Result.x, TestBVec2[i].Result.x, glm::epsilon<float>()) ? 0 : 1;
				Error += glm::epsilonEqual(Result.y, TestBVec2[i].Result.y, glm::epsilon<float>()) ? 0 : 1;
			}
		}

		// vec3 with bool
		{
			for(std::size_t i = 0; i < sizeof(TestVec3Bool) / sizeof(test<glm::vec3, bool>); ++i)
			{
				glm::vec3 Result = glm::mix(TestVec3Bool[i].x, TestVec3Bool[i].y, TestVec3Bool[i].a);
				Error += glm::epsilonEqual(Result.x, TestVec3Bool[i].Result.x, glm::epsilon<float>()) ? 0 : 1;
				Error += glm::epsilonEqual(Result.y, TestVec3Bool[i].Result.y, glm::epsilon<float>()) ? 0 : 1;
				Error += glm::epsilonEqual(Result.z, TestVec3Bool[i].Result.z, glm::epsilon<float>()) ? 0 : 1;
			}
		}

		// vec3 with bvec3
		{
			for(std::size_t i = 0; i < sizeof(TestBVec3) / sizeof(test<glm::vec3, glm::bvec3>); ++i)
			{
				glm::vec3 Result = glm::mix(TestBVec3[i].x, TestBVec3[i].y, TestBVec3[i].a);
				Error += glm::epsilonEqual(Result.x, TestBVec3[i].Result.x, glm::epsilon<float>()) ? 0 : 1;
				Error += glm::epsilonEqual(Result.y, TestBVec3[i].Result.y, glm::epsilon<float>()) ? 0 : 1;
				Error += glm::epsilonEqual(Result.z, TestBVec3[i].Result.z, glm::epsilon<float>()) ? 0 : 1;
			}
		}

		// vec4 with bool
		{
			for(std::size_t i = 0; i < sizeof(TestVec4Bool) / sizeof(test<glm::vec4, bool>); ++i)
			{
				glm::vec4 Result = glm::mix(TestVec4Bool[i].x, TestVec4Bool[i].y, TestVec4Bool[i].a);
				Error += glm::epsilonEqual(Result.x, TestVec4Bool[i].Result.x, glm::epsilon<float>()) ? 0 : 1;
				Error += glm::epsilonEqual(Result.y, TestVec4Bool[i].Result.y, glm::epsilon<float>()) ? 0 : 1;
				Error += glm::epsilonEqual(Result.z, TestVec4Bool[i].Result.z, glm::epsilon<float>()) ? 0 : 1;
				Error += glm::epsilonEqual(Result.w, TestVec4Bool[i].Result.w, glm::epsilon<float>()) ? 0 : 1;
			}
		}

		// vec4 with bvec4
		{
			for(std::size_t i = 0; i < sizeof(TestBVec4) / sizeof(test<glm::vec4, glm::bvec4>); ++i)
			{
				glm::vec4 Result = glm::mix(TestBVec4[i].x, TestBVec4[i].y, TestBVec4[i].a);
				Error += glm::epsilonEqual(Result.x, TestBVec4[i].Result.x, glm::epsilon<float>()) ? 0 : 1;
				Error += glm::epsilonEqual(Result.y, TestBVec4[i].Result.y, glm::epsilon<float>()) ? 0 : 1;
				Error += glm::epsilonEqual(Result.z, TestBVec4[i].Result.z, glm::epsilon<float>()) ? 0 : 1;
				Error += glm::epsilonEqual(Result.w, TestBVec4[i].Result.w, glm::epsilon<float>()) ? 0 : 1;
			}
		}

		return Error;
	}
}//namespace test_mix

namespace test_step
{
	template <typename EDGE, typename VEC>
	struct test
	{
		EDGE edge;
		VEC x;
		VEC result;
	};

	test<float, glm::vec4> TestVec4Scalar [] =
	{
		{ 0.0f, glm::vec4(1.0f, 2.0f, 3.0f, 4.0f), glm::vec4(1.0f) },
		{ 1.0f, glm::vec4(1.0f, 2.0f, 3.0f, 4.0f), glm::vec4(1.0f) },
		{ 0.0f, glm::vec4(-1.0f, -2.0f, -3.0f, -4.0f), glm::vec4(0.0f) }
	};

	test<glm::vec4, glm::vec4> TestVec4Vector [] =
	{
		{ glm::vec4(-1.0f, -2.0f, -3.0f, -4.0f), glm::vec4(-2.0f, -3.0f, -4.0f, -5.0f), glm::vec4(0.0f) },
		{ glm::vec4( 0.0f, 1.0f, 2.0f, 3.0f), glm::vec4( 1.0f, 2.0f, 3.0f, 4.0f), glm::vec4(1.0f) },
		{ glm::vec4( 2.0f, 3.0f, 4.0f, 5.0f), glm::vec4( 1.0f, 2.0f, 3.0f, 4.0f), glm::vec4(0.0f) },
		{ glm::vec4( 0.0f, 1.0f, 2.0f, 3.0f), glm::vec4(-1.0f,-2.0f,-3.0f,-4.0f), glm::vec4(0.0f) }
	};

	int run()
	{
		int Error = 0;

		// vec4 and float
		{
			for (std::size_t i = 0; i < sizeof(TestVec4Scalar) / sizeof(test<float, glm::vec4>); ++i)
			{
				glm::vec4 Result = glm::step(TestVec4Scalar[i].edge, TestVec4Scalar[i].x);
				Error += glm::all(glm::epsilonEqual(Result, TestVec4Scalar[i].result, glm::epsilon<float>())) ? 0 : 1;
			}
		}

		// vec4 and vec4
		{
			for (std::size_t i = 0; i < sizeof(TestVec4Vector) / sizeof(test<glm::vec4, glm::vec4>); ++i)
			{
				glm::vec4 Result = glm::step(TestVec4Vector[i].edge, TestVec4Vector[i].x);
				Error += glm::all(glm::epsilonEqual(Result, TestVec4Vector[i].result, glm::epsilon<float>())) ? 0 : 1;
			}
		}

		return Error;
	}
}//namespace test_step

int test_round()
{
	int Error = 0;
	
	{
		float A = glm::round(0.0f);
		Error += A == 0.0f ? 0 : 1;
		float B = glm::round(0.5f);
		Error += B == 1.0f ? 0 : 1;
		float C = glm::round(1.0f);
		Error += C == 1.0f ? 0 : 1;
		float D = glm::round(0.1f);
		Error += D == 0.0f ? 0 : 1;
		float E = glm::round(0.9f);
		Error += E == 1.0f ? 0 : 1;
		float F = glm::round(1.5f);
		Error += F == 2.0f ? 0 : 1;
		float G = glm::round(1.9f);
		Error += G == 2.0f ? 0 : 1;

#if GLM_LANG >= GLM_LANG_CXX11
		float A1 = glm::round(0.0f);
		Error += A1 == A ? 0 : 1;
		float B1 = glm::round(0.5f);
		Error += B1 == B ? 0 : 1;
		float C1 = glm::round(1.0f);
		Error += C1 == C ? 0 : 1;
		float D1 = glm::round(0.1f);
		Error += D1 == D ? 0 : 1;
		float E1 = glm::round(0.9f);
		Error += E1 == E ? 0 : 1;
		float F1 = glm::round(1.5f);
		Error += F == F ? 0 : 1;
		float G1 = glm::round(1.9f);
		Error += G1 == G ? 0 : 1;
#endif // GLM_LANG >= GLM_CXX0X
	}
	
	{
		float A = glm::round(-0.0f);
		Error += A ==  0.0f ? 0 : 1;
		float B = glm::round(-0.5f);
		Error += B == -1.0f ? 0 : 1;
		float C = glm::round(-1.0f);
		Error += C == -1.0f ? 0 : 1;
		float D = glm::round(-0.1f);
		Error += D ==  0.0f ? 0 : 1;
		float E = glm::round(-0.9f);
		Error += E == -1.0f ? 0 : 1;
		float F = glm::round(-1.5f);
		Error += F == -2.0f ? 0 : 1;
		float G = glm::round(-1.9f);
		Error += G == -2.0f ? 0 : 1;
	}
	
	return Error;
}

int test_roundEven()
{
	int Error = 0;

	{
		float A = glm::roundEven(-1.5f);
		Error += glm::epsilonEqual(A, -2.0f, 0.0001f) ? 0 : 1;
		Error += 0;
	}
	{
		float A = glm::roundEven(1.5f);
		Error += glm::epsilonEqual(A, 2.0f, 0.0001f) ? 0 : 1;
		Error += 0;
	}

	{
		float A = glm::roundEven(-3.5f);
		Error += glm::epsilonEqual(A, -4.0f, 0.0001f) ? 0 : 1;
		Error += 0;
	}
	{
		float A = glm::roundEven(3.5f);
		Error += glm::epsilonEqual(A, 4.0f, 0.0001f) ? 0 : 1;
		Error += 0;
	}

	{
		float A = glm::roundEven(-2.5f);
		Error += glm::epsilonEqual(A, -2.0f, 0.0001f) ? 0 : 1;
		Error += 0;
	}
	{
		float A = glm::roundEven(2.5f);
		Error += glm::epsilonEqual(A, 2.0f, 0.0001f) ? 0 : 1;
		Error += 0;
	}

	{
		float A = glm::roundEven(-2.4f);
		Error += glm::epsilonEqual(A, -2.0f, 0.0001f) ? 0 : 1;
		Error += 0;
	}
	{
		float A = glm::roundEven(2.4f);
		Error += glm::epsilonEqual(A, 2.0f, 0.0001f) ? 0 : 1;
		Error += 0;
	}

	{
		float A = glm::roundEven(-2.6f);
		Error += glm::epsilonEqual(A, -3.0f, 0.0001f) ? 0 : 1;
		Error += 0;
	}
	{
		float A = glm::roundEven(2.6f);
		Error += glm::epsilonEqual(A, 3.0f, 0.0001f) ? 0 : 1;
		Error += 0;
	}

	{
		float A = glm::roundEven(-2.0f);
		Error += glm::epsilonEqual(A, -2.0f, 0.0001f) ? 0 : 1;
		Error += 0;
	}
	{
		float A = glm::roundEven(2.0f);
		Error += glm::epsilonEqual(A, 2.0f, 0.0001f) ? 0 : 1;
		Error += 0;
	}

	{
		float A = glm::roundEven(0.0f);
		Error += A == 0.0f ? 0 : 1;
		float B = glm::roundEven(0.5f);
		Error += B == 0.0f ? 0 : 1;
		float C = glm::roundEven(1.0f);
		Error += C == 1.0f ? 0 : 1;
		float D = glm::roundEven(0.1f);
		Error += D == 0.0f ? 0 : 1;
		float E = glm::roundEven(0.9f);
		Error += E == 1.0f ? 0 : 1;
		float F = glm::roundEven(1.5f);
		Error += F == 2.0f ? 0 : 1;
		float G = glm::roundEven(1.9f);
		Error += G == 2.0f ? 0 : 1;
	}

	{
		float A = glm::roundEven(-0.0f);
		Error += A ==  0.0f ? 0 : 1;
		float B = glm::roundEven(-0.5f);
		Error += B == -0.0f ? 0 : 1;
		float C = glm::roundEven(-1.0f);
		Error += C == -1.0f ? 0 : 1;
		float D = glm::roundEven(-0.1f);
		Error += D ==  0.0f ? 0 : 1;
		float E = glm::roundEven(-0.9f);
		Error += E == -1.0f ? 0 : 1;
		float F = glm::roundEven(-1.5f);
		Error += F == -2.0f ? 0 : 1;
		float G = glm::roundEven(-1.9f);
		Error += G == -2.0f ? 0 : 1;
	}

	{
		float A = glm::roundEven(1.5f);
		Error += A == 2.0f ? 0 : 1;
		float B = glm::roundEven(2.5f);
		Error += B == 2.0f ? 0 : 1;
		float C = glm::roundEven(3.5f);
		Error += C == 4.0f ? 0 : 1;
		float D = glm::roundEven(4.5f);
		Error += D == 4.0f ? 0 : 1;
		float E = glm::roundEven(5.5f);
		Error += E == 6.0f ? 0 : 1;
		float F = glm::roundEven(6.5f);
		Error += F == 6.0f ? 0 : 1;
		float G = glm::roundEven(7.5f);
		Error += G == 8.0f ? 0 : 1;
	}
	
	{
		float A = glm::roundEven(-1.5f);
		Error += A == -2.0f ? 0 : 1;
		float B = glm::roundEven(-2.5f);
		Error += B == -2.0f ? 0 : 1;
		float C = glm::roundEven(-3.5f);
		Error += C == -4.0f ? 0 : 1;
		float D = glm::roundEven(-4.5f);
		Error += D == -4.0f ? 0 : 1;
		float E = glm::roundEven(-5.5f);
		Error += E == -6.0f ? 0 : 1;
		float F = glm::roundEven(-6.5f);
		Error += F == -6.0f ? 0 : 1;
		float G = glm::roundEven(-7.5f);
		Error += G == -8.0f ? 0 : 1;
	}

	return Error;
}

int test_isnan()
{
	int Error = 0;

	float Zero_f = 0.0;
	double Zero_d = 0.0;

	{
		Error += true == glm::isnan(0.0/Zero_d) ? 0 : 1;
		Error += true == glm::any(glm::isnan(glm::dvec2(0.0 / Zero_d))) ? 0 : 1;
		Error += true == glm::any(glm::isnan(glm::dvec3(0.0 / Zero_d))) ? 0 : 1;
		Error += true == glm::any(glm::isnan(glm::dvec4(0.0 / Zero_d))) ? 0 : 1;
	}

	{
		Error += true == glm::isnan(0.0f/Zero_f) ? 0 : 1;
		Error += true == glm::any(glm::isnan(glm::vec2(0.0f/Zero_f))) ? 0 : 1;
		Error += true == glm::any(glm::isnan(glm::vec3(0.0f/Zero_f))) ? 0 : 1;
		Error += true == glm::any(glm::isnan(glm::vec4(0.0f/Zero_f))) ? 0 : 1;
	}

	return Error;
}
 
int test_isinf()
{
	int Error = 0;
 
	float Zero_f = 0.0;
	double Zero_d = 0.0;

	{
		Error += true == glm::isinf( 1.0/Zero_d) ? 0 : 1;
		Error += true == glm::isinf(-1.0/Zero_d) ? 0 : 1;
		Error += true == glm::any(glm::isinf(glm::dvec2( 1.0/Zero_d))) ? 0 : 1;
		Error += true == glm::any(glm::isinf(glm::dvec2(-1.0/Zero_d))) ? 0 : 1;
		Error += true == glm::any(glm::isinf(glm::dvec3( 1.0/Zero_d))) ? 0 : 1;
		Error += true == glm::any(glm::isinf(glm::dvec3(-1.0/Zero_d))) ? 0 : 1;
		Error += true == glm::any(glm::isinf(glm::dvec4( 1.0/Zero_d))) ? 0 : 1;
		Error += true == glm::any(glm::isinf(glm::dvec4(-1.0/Zero_d))) ? 0 : 1;
	}
 
	{
		Error += true == glm::isinf( 1.0f/Zero_f) ? 0 : 1;
		Error += true == glm::isinf(-1.0f/Zero_f) ? 0 : 1;
		Error += true == glm::any(glm::isinf(glm::vec2( 1.0f/Zero_f))) ? 0 : 1;
		Error += true == glm::any(glm::isinf(glm::vec2(-1.0f/Zero_f))) ? 0 : 1;
		Error += true == glm::any(glm::isinf(glm::vec3( 1.0f/Zero_f))) ? 0 : 1;
		Error += true == glm::any(glm::isinf(glm::vec3(-1.0f/Zero_f))) ? 0 : 1;
		Error += true == glm::any(glm::isinf(glm::vec4( 1.0f/Zero_f))) ? 0 : 1;
		Error += true == glm::any(glm::isinf(glm::vec4(-1.0f/Zero_f))) ? 0 : 1;
	}
 
	return Error;
}

int main()
{
	int Error(0);

	Error += test_modf();
	Error += test_floatBitsToInt();
	Error += test_floatBitsToUint();
	Error += test_step::run();
	Error += test_mix::run();
	Error += test_round();
	Error += test_roundEven();
	Error += test_isnan();
	//Error += test_isinf();

	return Error;
}

