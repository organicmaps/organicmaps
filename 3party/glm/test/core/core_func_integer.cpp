///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2011-05-03
// Updated : 2011-05-03
// Licence : This source is under MIT licence
// File    : test/core/func_integer.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#define GLM_FORCE_RADIANS
#include <glm/integer.hpp>
#include <iostream>

enum result
{
	SUCCESS,
	FAIL,
	ASSERT,
	STATIC_ASSERT
};

namespace bitfieldExtract
{
	template <typename genType, typename sizeType>
	struct type
	{
		genType		Value;
		sizeType	BitFirst;
		sizeType	BitCount;
		genType		Return;
		result		Result;
	};

	typedef type<glm::uint, glm::uint> typeU32;

	typeU32 const Data32[] =
	{
		{0xffffffff, 8, 0, 0x00000000, SUCCESS},
		{0x00000000, 0,32, 0x00000000, SUCCESS},
		{0xffffffff, 0,32, 0xffffffff, SUCCESS},
		{0x0f0f0f0f, 0,32, 0x0f0f0f0f, SUCCESS},
		{0x00000000, 8, 0, 0x00000000, SUCCESS},
		{0x80000000,31, 1, 0x00000001, SUCCESS},
		{0x7fffffff,31, 1, 0x00000000, SUCCESS},
		{0x00000300, 8, 8, 0x00000003, SUCCESS},
		{0x0000ff00, 8, 8, 0x000000ff, SUCCESS},
		{0xfffffff0, 0, 5, 0x00000010, SUCCESS},
		{0x000000ff, 1, 3, 0x00000007, SUCCESS},
		{0x000000ff, 0, 3, 0x00000007, SUCCESS},
		{0x00000000, 0, 2, 0x00000000, SUCCESS},
		{0xffffffff, 0, 8, 0x000000ff, SUCCESS},
		{0xffff0000,16,16, 0x0000ffff, SUCCESS},
		{0xfffffff0, 0, 8, 0x00000000, FAIL},
		{0xffffffff,16,16, 0x00000000, FAIL},
		//{0xffffffff,32, 1, 0x00000000, ASSERT}, // Throw an assert 
		//{0xffffffff, 0,33, 0x00000000, ASSERT}, // Throw an assert 
		//{0xffffffff,16,16, 0x00000000, ASSERT}, // Throw an assert 
	};

	int test()
	{
		glm::uint count = sizeof(Data32) / sizeof(typeU32);
		
		for(glm::uint i = 0; i < count; ++i)
		{
			glm::uint Return = glm::bitfieldExtract(
				Data32[i].Value, 
				Data32[i].BitFirst, 
				Data32[i].BitCount);
			
			bool Compare = Data32[i].Return == Return;
			
			if(Data32[i].Result == SUCCESS && Compare)
				continue;
			else if(Data32[i].Result == FAIL && !Compare)
				continue;
			
			std::cout << "glm::bitfieldExtract test fail on test " << i << std::endl;
			return 1;
		}
		
		return 0;
	}
}//extractField

namespace bitfieldReverse
{
	template <typename genType>
	struct type
	{
		genType		Value;
		genType		Return;
		result		Result;
	};

	typedef type<glm::uint> typeU32;

	typeU32 const Data32[] =
	{
		{0xffffffff, 0xffffffff, SUCCESS},
		{0x00000000, 0x00000000, SUCCESS},
		{0xf0000000, 0x0000000f, SUCCESS},
	};

	int test()
	{
		glm::uint count = sizeof(Data32) / sizeof(typeU32);
		
		for(glm::uint i = 0; i < count; ++i)
		{
			glm::uint Return = glm::bitfieldReverse(
				Data32[i].Value);
			
			bool Compare = Data32[i].Return == Return;
			
			if(Data32[i].Result == SUCCESS && Compare)
				continue;
			else if(Data32[i].Result == FAIL && !Compare)
				continue;
			
			std::cout << "glm::bitfieldReverse test fail on test " << i << std::endl;
			return 1;
		}
		
		return 0;
	}
}//bitRevert

namespace findMSB
{
	template <typename genType>
	struct type
	{
		genType		Value;
		genType		Return;
	};

	type<int> const DataI32[] =
	{
		{0x00000000, -1},
		{0x00000001,  0},
		{0x00000002,  1},
		{0x00000003,  1},
		{0x00000004,  2},
		{0x00000005,  2},
		{0x00000007,  2},
		{0x00000008,  3},
		{0x00000010,  4},
		{0x00000020,  5},
		{0x00000040,  6},
		{0x00000080,  7},
		{0x00000100,  8},
		{0x00000200,  9},
		{0x00000400, 10},
		{0x00000800, 11},
		{0x00001000, 12},
		{0x00002000, 13},
		{0x00004000, 14},
		{0x00008000, 15},
		{0x00010000, 16},
		{0x00020000, 17},
		{0x00040000, 18},
		{0x00080000, 19},
		{0x00100000, 20},
		{0x00200000, 21},
		{0x00400000, 22},
		{0x00800000, 23},
		{0x01000000, 24},
		{0x02000000, 25},
		{0x04000000, 26},
		{0x08000000, 27},
		{0x10000000, 28},
		{0x20000000, 29},
		{0x40000000, 30}
	};

	int test()
	{
		int Error(0);

		for(std::size_t i = 0; i < sizeof(DataI32) / sizeof(type<int>); ++i)
		{
			int Result = glm::findMSB(DataI32[i].Value);
			Error += DataI32[i].Return == Result ? 0 : 1;
			assert(!Error);
		}

		return Error;
	}
}//findMSB

namespace findLSB
{
	template <typename genType>
	struct type
	{
		genType		Value;
		genType		Return;
	};

	type<int> const DataI32[] =
	{
		{0x00000001,  0},
		{0x00000003,  0},
		{0x00000002,  1}
	};

	int test()
	{
		int Error(0);

		for(std::size_t i = 0; i < sizeof(DataI32) / sizeof(type<int>); ++i)
		{
			int Result = glm::findLSB(DataI32[i].Value);
			Error += DataI32[i].Return == Result ? 0 : 1;
			assert(!Error);
		}

		return Error;
	}
}//findLSB

namespace usubBorrow
{
	int test()
	{
		int Error(0);
		
		glm::uint x = 16;
		glm::uint y = 17;
		glm::uint Borrow = 0;
		glm::uint Result = glm::usubBorrow(x, y, Borrow);
		
		return Error;
	}
	
}//namespace usubBorrow

int main()
{
	int Error = 0;

	std::cout << "sizeof(glm::uint64): " << sizeof(glm::detail::uint64) << std::endl;

	Error += ::bitfieldExtract::test();
	Error += ::bitfieldReverse::test();
	Error += ::findMSB::test();
	Error += ::findLSB::test();

	return Error;
}
