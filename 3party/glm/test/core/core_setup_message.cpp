///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2011-05-31
// Updated : 2013-08-27
// Licence : This source is under MIT License
// File    : test/core/setup_message.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#define GLM_MESSAGES
#include <glm/vec3.hpp>
#include <iostream>

int test_compiler()
{
	int Error(0);
	
	if(GLM_COMPILER & GLM_COMPILER_VC)
	{
		switch(GLM_COMPILER)
		{
		case GLM_COMPILER_VC8:
			std::cout << "GLM_COMPILER_VC8" << std::endl;	
			break;
		case GLM_COMPILER_VC9:
			std::cout << "GLM_COMPILER_VC9" << std::endl;	
			break;
		case GLM_COMPILER_VC10:
			std::cout << "GLM_COMPILER_VC10" << std::endl;	
			break;
		case GLM_COMPILER_VC11:
			std::cout << "GLM_COMPILER_VC11" << std::endl;	
			break;
		case GLM_COMPILER_VC12:
			std::cout << "GLM_COMPILER_VC12" << std::endl;
			break;
		default:
			std::cout << "Visual C++ version not detected" << std::endl;
			Error += 1;
			break;
		}
	}
	else if(GLM_COMPILER & GLM_COMPILER_GCC)
	{
		switch(GLM_COMPILER)
		{
		case GLM_COMPILER_GCC34:
			std::cout << "GLM_COMPILER_GCC34" << std::endl;	
			break;
		case GLM_COMPILER_GCC35:
			std::cout << "GLM_COMPILER_GCC35" << std::endl;	
			break;
		case GLM_COMPILER_GCC40:
			std::cout << "GLM_COMPILER_GCC40" << std::endl;	
			break;
		case GLM_COMPILER_GCC41:
			std::cout << "GLM_COMPILER_GCC41" << std::endl;	
			break;
		case GLM_COMPILER_GCC42:
			std::cout << "GLM_COMPILER_GCC42" << std::endl;	
			break;
		case GLM_COMPILER_GCC43:
			std::cout << "GLM_COMPILER_GCC43" << std::endl;	
			break;
		case GLM_COMPILER_GCC44:
			std::cout << "GLM_COMPILER_GCC44" << std::endl;	
			break;
		case GLM_COMPILER_GCC45:
			std::cout << "GLM_COMPILER_GCC45" << std::endl;	
			break;
		case GLM_COMPILER_GCC46:
			std::cout << "GLM_COMPILER_GCC46" << std::endl;	
			break;
		case GLM_COMPILER_GCC47:
			std::cout << "GLM_COMPILER_GCC47" << std::endl;	
			break;
		case GLM_COMPILER_GCC48:
			std::cout << "GLM_COMPILER_GCC48" << std::endl;	
			break;
		case GLM_COMPILER_GCC49:
			std::cout << "GLM_COMPILER_GCC49" << std::endl;
			break;
		default:
			std::cout << "GCC version not detected" << std::endl;
			Error += 1;
			break;
		}
	}
	else if(GLM_COMPILER & GLM_COMPILER_CUDA)
	{
		std::cout << "GLM_COMPILER_CUDA" << std::endl;		
	}
	else if(GLM_COMPILER & GLM_COMPILER_CLANG)
	{
		switch(GLM_COMPILER)
		{
		case GLM_COMPILER_CLANG26:
			std::cout << "GLM_COMPILER_CLANG26" << std::endl;		
			break;
		case GLM_COMPILER_CLANG27:
			std::cout << "GLM_COMPILER_CLANG27" << std::endl;		
			break;
		case GLM_COMPILER_CLANG28:
			std::cout << "GLM_COMPILER_CLANG28" << std::endl;		
			break;
		case GLM_COMPILER_CLANG29:
			std::cout << "GLM_COMPILER_CLANG29" << std::endl;		
			break;
		case GLM_COMPILER_CLANG30:
			std::cout << "GLM_COMPILER_CLANG30" << std::endl;		
			break;
		case GLM_COMPILER_CLANG31:
			std::cout << "GLM_COMPILER_CLANG31" << std::endl;		
			break;
		case GLM_COMPILER_CLANG32:
			std::cout << "GLM_COMPILER_CLANG32" << std::endl;
			break;
		case GLM_COMPILER_CLANG33:
			std::cout << "GLM_COMPILER_CLANG33" << std::endl;
			break;
		case GLM_COMPILER_CLANG40:
			std::cout << "GLM_COMPILER_CLANG40" << std::endl;
			break;
		case GLM_COMPILER_CLANG41:
			std::cout << "GLM_COMPILER_CLANG41" << std::endl;
			break;
		case GLM_COMPILER_CLANG42:
			std::cout << "GLM_COMPILER_CLANG42" << std::endl;
			break;
		case GLM_COMPILER_CLANG43:
			std::cout << "GLM_COMPILER_CLANG43" << std::endl;
			break;
		case GLM_COMPILER_CLANG50:
			std::cout << "GLM_COMPILER_CLANG50" << std::endl;
			break;
		default:
			std::cout << "Clang version not detected" << std::endl;
			break;
		}
	}
	else if(GLM_COMPILER & GLM_COMPILER_LLVM_GCC)
	{
		std::cout << "GLM_COMPILER_LLVM_GCC" << std::endl;

	}
	else if(GLM_COMPILER & GLM_COMPILER_INTEL)
	{
		switch(GLM_COMPILER)
		{
		case GLM_COMPILER_INTEL9:
			std::cout << "GLM_COMPILER_INTEL9" << std::endl;
			break;
		case GLM_COMPILER_INTEL10_0:
			std::cout << "GLM_COMPILER_INTEL10_0" << std::endl;
			break;
		case GLM_COMPILER_INTEL10_1:
			std::cout << "GLM_COMPILER_INTEL10_1" << std::endl;
			break;
		case GLM_COMPILER_INTEL11_0:
			std::cout << "GLM_COMPILER_INTEL11_0" << std::endl;
			break;
		case GLM_COMPILER_INTEL11_1:
			std::cout << "GLM_COMPILER_INTEL11_1" << std::endl;
			break;
		case GLM_COMPILER_INTEL12_1:
			std::cout << "GLM_COMPILER_INTEL12_1" << std::endl;
			break;
		case GLM_COMPILER_INTEL13_0:
			std::cout << "GLM_COMPILER_INTEL13_0" << std::endl;
			break;
		default:
			std::cout << "Intel compiler version not detected" << std::endl;
			Error += 1;
			break;
		}
	}
	else
	{
		std::cout << "Undetected compiler" << std::endl;
		Error += 1;
	}
	
	return Error;
}

int test_model()
{
	int Error = 0;
	
	Error += ((sizeof(void*) == 4) && (GLM_MODEL == GLM_MODEL_32)) || ((sizeof(void*) == 8) && (GLM_MODEL == GLM_MODEL_64)) ? 0 : 1;
	
	if(GLM_MODEL == GLM_MODEL_32)
		std::cout << "GLM_MODEL_32" << std::endl;
	else if(GLM_MODEL == GLM_MODEL_64)
		std::cout << "GLM_MODEL_64" << std::endl;
	
	return Error;
}

int test_instruction_set()
{
	int Error = 0;

	std::cout << "GLM_ARCH: ";

	if(GLM_ARCH == GLM_ARCH_PURE)
		std::cout << "GLM_ARCH_PURE ";
	if(GLM_ARCH & GLM_ARCH_AVX2)
		std::cout << "GLM_ARCH_AVX2 ";
	if(GLM_ARCH & GLM_ARCH_AVX)
		std::cout << "GLM_ARCH_AVX ";
	if(GLM_ARCH & GLM_ARCH_AVX)
		std::cout << "GLM_ARCH_SSE4 ";
	if(GLM_ARCH & GLM_ARCH_SSE3)
		std::cout << "GLM_ARCH_SSE3 ";
	if(GLM_ARCH & GLM_ARCH_SSE2)
		std::cout << "GLM_ARCH_SSE2 ";

	std::cout << std::endl;

	return Error;
}

int test_cpp_version()
{
	std::cout << "__cplusplus: " << __cplusplus << std::endl;
	
	return 0;
}

int test_operators()
{
	glm::vec3 A(1.0f);
	glm::vec3 B(1.0f);
	bool R = A != B;
	bool S = A == B;

	return (S && !R) ? 0 : 1;
}

template <typename T>
struct vec
{

};

template <template <typename> class C, typename T>
struct Class
{

};

template <typename T>
struct Class<vec, T>
{

};

int main()
{
	//Class<vec, float> C;

	int Error = 0;

	Error += test_cpp_version();
	Error += test_compiler();
	Error += test_model();
	Error += test_instruction_set();
	Error += test_operators();
	
	return Error;
}
