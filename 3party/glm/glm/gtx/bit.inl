///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2007-03-14
// Updated : 2013-12-25
// Licence : This source is under MIT License
// File    : glm/gtx/bit.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "../detail/_vectorize.hpp"
#include <limits>

namespace glm
{
	template <typename genIType>
	GLM_FUNC_QUALIFIER genIType mask
	(
		genIType const & count
	)
	{
		return ((genIType(1) << (count)) - genIType(1));
	}

	VECTORIZE_VEC(mask)

	// highestBitValue
	template <typename genType>
	GLM_FUNC_QUALIFIER genType highestBitValue
	(
		genType const & value
	)
	{
		genType tmp = value;
		genType result = genType(0);
		while(tmp)
		{
			result = (tmp & (~tmp + 1)); // grab lowest bit
			tmp &= ~result; // clear lowest bit
		}
		return result;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec2<int, P> highestBitValue
	(
		detail::tvec2<T, P> const & value
	)
	{
		return detail::tvec2<int, P>(
			highestBitValue(value[0]),
			highestBitValue(value[1]));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<int, P> highestBitValue
	(
		detail::tvec3<T, P> const & value
	)
	{
		return detail::tvec3<int, P>(
			highestBitValue(value[0]),
			highestBitValue(value[1]),
			highestBitValue(value[2]));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec4<int, P> highestBitValue
	(
		detail::tvec4<T, P> const & value
	)
	{
		return detail::tvec4<int, P>(
			highestBitValue(value[0]),
			highestBitValue(value[1]),
			highestBitValue(value[2]),
			highestBitValue(value[3]));
	}

	// isPowerOfTwo
	template <typename genType>
	GLM_FUNC_QUALIFIER bool isPowerOfTwo(genType const & Value)
	{
		//detail::If<std::numeric_limits<genType>::is_signed>::apply(abs, Value);
		//return !(Value & (Value - 1));

		// For old complier?
		genType Result = Value;
		if(std::numeric_limits<genType>::is_signed)
			Result = abs(Result);
		return !(Result & (Result - 1));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec2<bool, P> isPowerOfTwo
	(
		detail::tvec2<T, P> const & value
	)
	{
		return detail::tvec2<bool, P>(
			isPowerOfTwo(value[0]),
			isPowerOfTwo(value[1]));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<bool, P> isPowerOfTwo
	(
		detail::tvec3<T, P> const & value
	)
	{
		return detail::tvec3<bool, P>(
			isPowerOfTwo(value[0]),
			isPowerOfTwo(value[1]),
			isPowerOfTwo(value[2]));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec4<bool, P> isPowerOfTwo
	(
		detail::tvec4<T, P> const & value
	)
	{
		return detail::tvec4<bool, P>(
			isPowerOfTwo(value[0]),
			isPowerOfTwo(value[1]),
			isPowerOfTwo(value[2]),
			isPowerOfTwo(value[3]));
	}

	// powerOfTwoAbove
	template <typename genType>
	GLM_FUNC_QUALIFIER genType powerOfTwoAbove(genType const & value)
	{
		return isPowerOfTwo(value) ? value : highestBitValue(value) << 1;
	}

	VECTORIZE_VEC(powerOfTwoAbove)

	// powerOfTwoBelow
	template <typename genType>
	GLM_FUNC_QUALIFIER genType powerOfTwoBelow
	(
		genType const & value
	)
	{
		return isPowerOfTwo(value) ? value : highestBitValue(value);
	}

	VECTORIZE_VEC(powerOfTwoBelow)

	// powerOfTwoNearest
	template <typename genType>
	GLM_FUNC_QUALIFIER genType powerOfTwoNearest
	(
		genType const & value
	)
	{
		if(isPowerOfTwo(value))
			return value;

		genType prev = highestBitValue(value);
		genType next = prev << 1;
		return (next - value) < (value - prev) ? next : prev;
	}

	VECTORIZE_VEC(powerOfTwoNearest)

	template <typename genType>
	GLM_FUNC_QUALIFIER genType bitRevert(genType const & In)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<genType>::is_integer, "'bitRevert' only accept integer values");

		genType Out = 0;
		std::size_t BitSize = sizeof(genType) * 8;
		for(std::size_t i = 0; i < BitSize; ++i)
			if(In & (genType(1) << i))
				Out |= genType(1) << (BitSize - 1 - i);
		return Out;
	}

	VECTORIZE_VEC(bitRevert)

	template <typename genType>
	GLM_FUNC_QUALIFIER genType bitRotateRight(genType const & In, std::size_t Shift)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<genType>::is_integer, "'bitRotateRight' only accept integer values");

		std::size_t BitSize = sizeof(genType) * 8;
		return (In << Shift) | (In >> (BitSize - Shift));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec2<T, P> bitRotateRight
	(
		detail::tvec2<T, P> const & Value, 
		std::size_t Shift
	)
	{
		return detail::tvec2<T, P>(
			bitRotateRight(Value[0], Shift),
			bitRotateRight(Value[1], Shift));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> bitRotateRight
	(
		detail::tvec3<T, P> const & Value, 
		std::size_t Shift
	)
	{
		return detail::tvec3<T, P>(
			bitRotateRight(Value[0], Shift),
			bitRotateRight(Value[1], Shift),
			bitRotateRight(Value[2], Shift));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec4<T, P> bitRotateRight
	(
		detail::tvec4<T, P> const & Value, 
		std::size_t Shift
	)
	{
		return detail::tvec4<T, P>(
			bitRotateRight(Value[0], Shift),
			bitRotateRight(Value[1], Shift),
			bitRotateRight(Value[2], Shift),
			bitRotateRight(Value[3], Shift));
	}

	template <typename genType>
	GLM_FUNC_QUALIFIER genType bitRotateLeft(genType const & In, std::size_t Shift)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<genType>::is_integer, "'bitRotateLeft' only accept integer values");

		std::size_t BitSize = sizeof(genType) * 8;
		return (In >> Shift) | (In << (BitSize - Shift));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec2<T, P> bitRotateLeft
	(
		detail::tvec2<T, P> const & Value, 
		std::size_t Shift
	)
	{
		return detail::tvec2<T, P>(
			bitRotateLeft(Value[0], Shift),
			bitRotateLeft(Value[1], Shift));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> bitRotateLeft
	(
		detail::tvec3<T, P> const & Value, 
		std::size_t Shift
	)
	{
		return detail::tvec3<T, P>(
			bitRotateLeft(Value[0], Shift),
			bitRotateLeft(Value[1], Shift),
			bitRotateLeft(Value[2], Shift));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec4<T, P> bitRotateLeft
	(
		detail::tvec4<T, P> const & Value, 
		std::size_t Shift
	)
	{
		return detail::tvec4<T, P>(
			bitRotateLeft(Value[0], Shift),
			bitRotateLeft(Value[1], Shift),
			bitRotateLeft(Value[2], Shift),
			bitRotateLeft(Value[3], Shift));
	}

	template <typename genIUType>
	GLM_FUNC_QUALIFIER genIUType fillBitfieldWithOne
	(
		genIUType const & Value,
		int const & FromBit, 
		int const & ToBit
	)
	{
		assert(FromBit <= ToBit);
		assert(ToBit <= sizeof(genIUType) * std::size_t(8));

		genIUType Result = Value;
		for(signed i = 0; i <= ToBit; ++i)
			Result |= (1 << i);
		return Result;
	}

	template <typename genIUType>
	GLM_FUNC_QUALIFIER genIUType fillBitfieldWithZero
	(
		genIUType const & Value,
		int const & FromBit, 
		int const & ToBit
	)
	{
		assert(FromBit <= ToBit);
		assert(ToBit <= sizeof(genIUType) * std::size_t(8));

		genIUType Result = Value;
		for(signed i = 0; i <= ToBit; ++i)
			Result &= ~(1 << i);
		return Result;
	}

	namespace detail
	{
		template <typename PARAM, typename RET>
		GLM_FUNC_DECL RET bitfieldInterleave(PARAM x, PARAM y);

		template <typename PARAM, typename RET>
		GLM_FUNC_DECL RET bitfieldInterleave(PARAM x, PARAM y, PARAM z);

		template <typename PARAM, typename RET>
		GLM_FUNC_DECL RET bitfieldInterleave(PARAM x, PARAM y, PARAM z, PARAM w);

/*
		template <typename PARAM, typename RET>
		inline RET bitfieldInterleave(PARAM x, PARAM y)
		{
			RET Result = 0; 
			for (int i = 0; i < sizeof(PARAM) * 8; i++)
				Result |= (x & 1U << i) << i | (y & 1U << i) << (i + 1);
			return Result;
		}

		template <typename PARAM, typename RET>
		inline RET bitfieldInterleave(PARAM x, PARAM y, PARAM z)
		{
			RET Result = 0; 
			for (RET i = 0; i < sizeof(PARAM) * 8; i++)
			{
				Result |= ((RET(x) & (RET(1) << i)) << ((i << 1) + 0));
				Result |= ((RET(y) & (RET(1) << i)) << ((i << 1) + 1));
				Result |= ((RET(z) & (RET(1) << i)) << ((i << 1) + 2));
			}
			return Result;
		}

		template <typename PARAM, typename RET>
		inline RET bitfieldInterleave(PARAM x, PARAM y, PARAM z, PARAM w)
		{
			RET Result = 0; 
			for (int i = 0; i < sizeof(PARAM) * 8; i++)
			{
				Result |= ((((RET(x) >> i) & RET(1))) << RET((i << 2) + 0));
				Result |= ((((RET(y) >> i) & RET(1))) << RET((i << 2) + 1));
				Result |= ((((RET(z) >> i) & RET(1))) << RET((i << 2) + 2));
				Result |= ((((RET(w) >> i) & RET(1))) << RET((i << 2) + 3));
			}
			return Result;
		}
*/
		template <>
		GLM_FUNC_QUALIFIER glm::uint16 bitfieldInterleave(glm::uint8 x, glm::uint8 y)
		{
			glm::uint16 REG1(x);
			glm::uint16 REG2(y);

			REG1 = ((REG1 <<  4) | REG1) & glm::uint16(0x0F0F);
			REG2 = ((REG2 <<  4) | REG2) & glm::uint16(0x0F0F);

			REG1 = ((REG1 <<  2) | REG1) & glm::uint16(0x3333);
			REG2 = ((REG2 <<  2) | REG2) & glm::uint16(0x3333);

			REG1 = ((REG1 <<  1) | REG1) & glm::uint16(0x5555);
			REG2 = ((REG2 <<  1) | REG2) & glm::uint16(0x5555);

			return REG1 | (REG2 << 1);
		}

		template <>
		GLM_FUNC_QUALIFIER glm::uint32 bitfieldInterleave(glm::uint16 x, glm::uint16 y)
		{
			glm::uint32 REG1(x);
			glm::uint32 REG2(y);

			REG1 = ((REG1 <<  8) | REG1) & glm::uint32(0x00FF00FF);
			REG2 = ((REG2 <<  8) | REG2) & glm::uint32(0x00FF00FF);

			REG1 = ((REG1 <<  4) | REG1) & glm::uint32(0x0F0F0F0F);
			REG2 = ((REG2 <<  4) | REG2) & glm::uint32(0x0F0F0F0F);

			REG1 = ((REG1 <<  2) | REG1) & glm::uint32(0x33333333);
			REG2 = ((REG2 <<  2) | REG2) & glm::uint32(0x33333333);

			REG1 = ((REG1 <<  1) | REG1) & glm::uint32(0x55555555);
			REG2 = ((REG2 <<  1) | REG2) & glm::uint32(0x55555555);

			return REG1 | (REG2 << 1);
		}

		template <>
		GLM_FUNC_QUALIFIER glm::uint64 bitfieldInterleave(glm::uint32 x, glm::uint32 y)
		{
			glm::uint64 REG1(x);
			glm::uint64 REG2(y);

			REG1 = ((REG1 << 16) | REG1) & glm::uint64(0x0000FFFF0000FFFF);
			REG2 = ((REG2 << 16) | REG2) & glm::uint64(0x0000FFFF0000FFFF);

			REG1 = ((REG1 <<  8) | REG1) & glm::uint64(0x00FF00FF00FF00FF);
			REG2 = ((REG2 <<  8) | REG2) & glm::uint64(0x00FF00FF00FF00FF);

			REG1 = ((REG1 <<  4) | REG1) & glm::uint64(0x0F0F0F0F0F0F0F0F);
			REG2 = ((REG2 <<  4) | REG2) & glm::uint64(0x0F0F0F0F0F0F0F0F);

			REG1 = ((REG1 <<  2) | REG1) & glm::uint64(0x3333333333333333);
			REG2 = ((REG2 <<  2) | REG2) & glm::uint64(0x3333333333333333);

			REG1 = ((REG1 <<  1) | REG1) & glm::uint64(0x5555555555555555);
			REG2 = ((REG2 <<  1) | REG2) & glm::uint64(0x5555555555555555);

			return REG1 | (REG2 << 1);
		}

		template <>
		GLM_FUNC_QUALIFIER glm::uint32 bitfieldInterleave(glm::uint8 x, glm::uint8 y, glm::uint8 z)
		{
			glm::uint32 REG1(x);
			glm::uint32 REG2(y);
			glm::uint32 REG3(z);
			
			REG1 = ((REG1 << 16) | REG1) & glm::uint32(0x00FF0000FF0000FF);
			REG2 = ((REG2 << 16) | REG2) & glm::uint32(0x00FF0000FF0000FF);
			REG3 = ((REG3 << 16) | REG3) & glm::uint32(0x00FF0000FF0000FF);
			
			REG1 = ((REG1 <<  8) | REG1) & glm::uint32(0xF00F00F00F00F00F);
			REG2 = ((REG2 <<  8) | REG2) & glm::uint32(0xF00F00F00F00F00F);
			REG3 = ((REG3 <<  8) | REG3) & glm::uint32(0xF00F00F00F00F00F);
			
			REG1 = ((REG1 <<  4) | REG1) & glm::uint32(0x30C30C30C30C30C3);
			REG2 = ((REG2 <<  4) | REG2) & glm::uint32(0x30C30C30C30C30C3);
			REG3 = ((REG3 <<  4) | REG3) & glm::uint32(0x30C30C30C30C30C3);
			
			REG1 = ((REG1 <<  2) | REG1) & glm::uint32(0x9249249249249249);
			REG2 = ((REG2 <<  2) | REG2) & glm::uint32(0x9249249249249249);
			REG3 = ((REG3 <<  2) | REG3) & glm::uint32(0x9249249249249249);
			
			return REG1 | (REG2 << 1) | (REG3 << 2);
		}
		
		template <>
		GLM_FUNC_QUALIFIER glm::uint64 bitfieldInterleave(glm::uint16 x, glm::uint16 y, glm::uint16 z)
		{
			glm::uint64 REG1(x);
			glm::uint64 REG2(y);
			glm::uint64 REG3(z);
			
			REG1 = ((REG1 << 32) | REG1) & glm::uint64(0xFFFF00000000FFFF);
			REG2 = ((REG2 << 32) | REG2) & glm::uint64(0xFFFF00000000FFFF);
			REG3 = ((REG3 << 32) | REG3) & glm::uint64(0xFFFF00000000FFFF);
			
			REG1 = ((REG1 << 16) | REG1) & glm::uint64(0x00FF0000FF0000FF);
			REG2 = ((REG2 << 16) | REG2) & glm::uint64(0x00FF0000FF0000FF);
			REG3 = ((REG3 << 16) | REG3) & glm::uint64(0x00FF0000FF0000FF);
			
			REG1 = ((REG1 <<  8) | REG1) & glm::uint64(0xF00F00F00F00F00F);
			REG2 = ((REG2 <<  8) | REG2) & glm::uint64(0xF00F00F00F00F00F);
			REG3 = ((REG3 <<  8) | REG3) & glm::uint64(0xF00F00F00F00F00F);
			
			REG1 = ((REG1 <<  4) | REG1) & glm::uint64(0x30C30C30C30C30C3);
			REG2 = ((REG2 <<  4) | REG2) & glm::uint64(0x30C30C30C30C30C3);
			REG3 = ((REG3 <<  4) | REG3) & glm::uint64(0x30C30C30C30C30C3);
			
			REG1 = ((REG1 <<  2) | REG1) & glm::uint64(0x9249249249249249);
			REG2 = ((REG2 <<  2) | REG2) & glm::uint64(0x9249249249249249);
			REG3 = ((REG3 <<  2) | REG3) & glm::uint64(0x9249249249249249);
			
			return REG1 | (REG2 << 1) | (REG3 << 2);
		}
		
		template <>
		GLM_FUNC_QUALIFIER glm::uint64 bitfieldInterleave(glm::uint32 x, glm::uint32 y, glm::uint32 z)
		{
			glm::uint64 REG1(x);
			glm::uint64 REG2(y);
			glm::uint64 REG3(z);

			REG1 = ((REG1 << 32) | REG1) & glm::uint64(0xFFFF00000000FFFF);
			REG2 = ((REG2 << 32) | REG2) & glm::uint64(0xFFFF00000000FFFF);
			REG3 = ((REG3 << 32) | REG3) & glm::uint64(0xFFFF00000000FFFF);

			REG1 = ((REG1 << 16) | REG1) & glm::uint64(0x00FF0000FF0000FF);
			REG2 = ((REG2 << 16) | REG2) & glm::uint64(0x00FF0000FF0000FF);
			REG3 = ((REG3 << 16) | REG3) & glm::uint64(0x00FF0000FF0000FF);

			REG1 = ((REG1 <<  8) | REG1) & glm::uint64(0xF00F00F00F00F00F);
			REG2 = ((REG2 <<  8) | REG2) & glm::uint64(0xF00F00F00F00F00F);
			REG3 = ((REG3 <<  8) | REG3) & glm::uint64(0xF00F00F00F00F00F);

			REG1 = ((REG1 <<  4) | REG1) & glm::uint64(0x30C30C30C30C30C3);
			REG2 = ((REG2 <<  4) | REG2) & glm::uint64(0x30C30C30C30C30C3);
			REG3 = ((REG3 <<  4) | REG3) & glm::uint64(0x30C30C30C30C30C3);

			REG1 = ((REG1 <<  2) | REG1) & glm::uint64(0x9249249249249249);
			REG2 = ((REG2 <<  2) | REG2) & glm::uint64(0x9249249249249249);
			REG3 = ((REG3 <<  2) | REG3) & glm::uint64(0x9249249249249249);

			return REG1 | (REG2 << 1) | (REG3 << 2);
		}

		template <>
		GLM_FUNC_QUALIFIER glm::uint32 bitfieldInterleave(glm::uint8 x, glm::uint8 y, glm::uint8 z, glm::uint8 w)
		{
			glm::uint32 REG1(x);
			glm::uint32 REG2(y);
			glm::uint32 REG3(z);
			glm::uint32 REG4(w);
			
			REG1 = ((REG1 << 12) | REG1) & glm::uint32(0x000F000F000F000F);
			REG2 = ((REG2 << 12) | REG2) & glm::uint32(0x000F000F000F000F);
			REG3 = ((REG3 << 12) | REG3) & glm::uint32(0x000F000F000F000F);
			REG4 = ((REG4 << 12) | REG4) & glm::uint32(0x000F000F000F000F);
			
			REG1 = ((REG1 <<  6) | REG1) & glm::uint32(0x0303030303030303);
			REG2 = ((REG2 <<  6) | REG2) & glm::uint32(0x0303030303030303);
			REG3 = ((REG3 <<  6) | REG3) & glm::uint32(0x0303030303030303);
			REG4 = ((REG4 <<  6) | REG4) & glm::uint32(0x0303030303030303);
			
			REG1 = ((REG1 <<  3) | REG1) & glm::uint32(0x1111111111111111);
			REG2 = ((REG2 <<  3) | REG2) & glm::uint32(0x1111111111111111);
			REG3 = ((REG3 <<  3) | REG3) & glm::uint32(0x1111111111111111);
			REG4 = ((REG4 <<  3) | REG4) & glm::uint32(0x1111111111111111);
			
			return REG1 | (REG2 << 1) | (REG3 << 2) | (REG4 << 3);
		}
		
		template <>
		GLM_FUNC_QUALIFIER glm::uint64 bitfieldInterleave(glm::uint16 x, glm::uint16 y, glm::uint16 z, glm::uint16 w)
		{
			glm::uint64 REG1(x);
			glm::uint64 REG2(y);
			glm::uint64 REG3(z);
			glm::uint64 REG4(w);

			REG1 = ((REG1 << 24) | REG1) & glm::uint64(0x000000FF000000FF);
			REG2 = ((REG2 << 24) | REG2) & glm::uint64(0x000000FF000000FF);
			REG3 = ((REG3 << 24) | REG3) & glm::uint64(0x000000FF000000FF);
			REG4 = ((REG4 << 24) | REG4) & glm::uint64(0x000000FF000000FF);

			REG1 = ((REG1 << 12) | REG1) & glm::uint64(0x000F000F000F000F);
			REG2 = ((REG2 << 12) | REG2) & glm::uint64(0x000F000F000F000F);
			REG3 = ((REG3 << 12) | REG3) & glm::uint64(0x000F000F000F000F);
			REG4 = ((REG4 << 12) | REG4) & glm::uint64(0x000F000F000F000F);

			REG1 = ((REG1 <<  6) | REG1) & glm::uint64(0x0303030303030303);
			REG2 = ((REG2 <<  6) | REG2) & glm::uint64(0x0303030303030303);
			REG3 = ((REG3 <<  6) | REG3) & glm::uint64(0x0303030303030303);
			REG4 = ((REG4 <<  6) | REG4) & glm::uint64(0x0303030303030303);

			REG1 = ((REG1 <<  3) | REG1) & glm::uint64(0x1111111111111111);
			REG2 = ((REG2 <<  3) | REG2) & glm::uint64(0x1111111111111111);
			REG3 = ((REG3 <<  3) | REG3) & glm::uint64(0x1111111111111111);
			REG4 = ((REG4 <<  3) | REG4) & glm::uint64(0x1111111111111111);

			return REG1 | (REG2 << 1) | (REG3 << 2) | (REG4 << 3);
		}
	}//namespace detail

	GLM_FUNC_QUALIFIER int16 bitfieldInterleave(int8 x, int8 y)
	{
		union sign8
		{
			int8 i;
			uint8 u;
		} sign_x, sign_y;

		union sign16
		{
			int16 i;
			uint16 u;
		} result;

		sign_x.i = x;
		sign_y.i = y;
		result.u = bitfieldInterleave(sign_x.u, sign_y.u);

		return result.i;
	}

	GLM_FUNC_QUALIFIER uint16 bitfieldInterleave(uint8 x, uint8 y)
	{
		return detail::bitfieldInterleave<uint8, uint16>(x, y);
	}

	GLM_FUNC_QUALIFIER int32 bitfieldInterleave(int16 x, int16 y)
	{
		union sign16
		{
			int16 i;
			uint16 u;
		} sign_x, sign_y;

		union sign32
		{
			int32 i;
			uint32 u;
		} result;

		sign_x.i = x;
		sign_y.i = y;
		result.u = bitfieldInterleave(sign_x.u, sign_y.u);

		return result.i;
	}

	GLM_FUNC_QUALIFIER uint32 bitfieldInterleave(uint16 x, uint16 y)
	{
		return detail::bitfieldInterleave<uint16, uint32>(x, y);
	}

	GLM_FUNC_QUALIFIER int64 bitfieldInterleave(int32 x, int32 y)
	{
		union sign32
		{
			int32 i;
			uint32 u;
		} sign_x, sign_y;

		union sign64
		{
			int64 i;
			uint64 u;
		} result;

		sign_x.i = x;
		sign_y.i = y;
		result.u = bitfieldInterleave(sign_x.u, sign_y.u);

		return result.i;
	}

	GLM_FUNC_QUALIFIER uint64 bitfieldInterleave(uint32 x, uint32 y)
	{
		return detail::bitfieldInterleave<uint32, uint64>(x, y);
	}

	GLM_FUNC_QUALIFIER int32 bitfieldInterleave(int8 x, int8 y, int8 z)
	{
		union sign8
		{
			int8 i;
			uint8 u;
		} sign_x, sign_y, sign_z;

		union sign32
		{
			int32 i;
			uint32 u;
		} result;

		sign_x.i = x;
		sign_y.i = y;
		sign_z.i = z;
		result.u = bitfieldInterleave(sign_x.u, sign_y.u, sign_z.u);

		return result.i;
	}

	GLM_FUNC_QUALIFIER uint32 bitfieldInterleave(uint8 x, uint8 y, uint8 z)
	{
		return detail::bitfieldInterleave<uint8, uint32>(x, y, z);
	}

	GLM_FUNC_QUALIFIER int64 bitfieldInterleave(int16 x, int16 y, int16 z)
	{
		union sign16
		{
			int16 i;
			uint16 u;
		} sign_x, sign_y, sign_z;

		union sign64
		{
			int64 i;
			uint64 u;
		} result;

		sign_x.i = x;
		sign_y.i = y;
		sign_z.i = z;
		result.u = bitfieldInterleave(sign_x.u, sign_y.u, sign_z.u);

		return result.i;
	}

	GLM_FUNC_QUALIFIER uint64 bitfieldInterleave(uint16 x, uint16 y, uint16 z)
	{
		return detail::bitfieldInterleave<uint32, uint64>(x, y, z);
	}

	GLM_FUNC_QUALIFIER int64 bitfieldInterleave(int32 x, int32 y, int32 z)
	{
		union sign16
		{
			int32 i;
			uint32 u;
		} sign_x, sign_y, sign_z;

		union sign64
		{
			int64 i;
			uint64 u;
		} result;

		sign_x.i = x;
		sign_y.i = y;
		sign_z.i = z;
		result.u = bitfieldInterleave(sign_x.u, sign_y.u, sign_z.u);

		return result.i;
	}

	GLM_FUNC_QUALIFIER uint64 bitfieldInterleave(uint32 x, uint32 y, uint32 z)
	{
		return detail::bitfieldInterleave<uint32, uint64>(x, y, z);
	}

	GLM_FUNC_QUALIFIER int32 bitfieldInterleave(int8 x, int8 y, int8 z, int8 w)
	{
		union sign8
		{
			int8 i;
			uint8 u;
		} sign_x, sign_y, sign_z, sign_w;

		union sign32
		{
			int32 i;
			uint32 u;
		} result;

		sign_x.i = x;
		sign_y.i = y;
		sign_z.i = z;
		sign_w.i = w;
		result.u = bitfieldInterleave(sign_x.u, sign_y.u, sign_z.u, sign_w.u);

		return result.i;
	}

	GLM_FUNC_QUALIFIER uint32 bitfieldInterleave(uint8 x, uint8 y, uint8 z, uint8 w)
	{
		return detail::bitfieldInterleave<uint8, uint32>(x, y, z, w);
	}

	GLM_FUNC_QUALIFIER int64 bitfieldInterleave(int16 x, int16 y, int16 z, int16 w)
	{
		union sign16
		{
			int16 i;
			uint16 u;
		} sign_x, sign_y, sign_z, sign_w;

		union sign64
		{
			int64 i;
			uint64 u;
		} result;

		sign_x.i = x;
		sign_y.i = y;
		sign_z.i = z;
		sign_w.i = w;
		result.u = bitfieldInterleave(sign_x.u, sign_y.u, sign_z.u, sign_w.u);

		return result.i;
	}

	GLM_FUNC_QUALIFIER uint64 bitfieldInterleave(uint16 x, uint16 y, uint16 z, uint16 w)
	{
		return detail::bitfieldInterleave<uint16, uint64>(x, y, z, w);
	}

}//namespace glm
