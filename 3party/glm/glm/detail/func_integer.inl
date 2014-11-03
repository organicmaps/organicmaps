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
/// @ref core
/// @file glm/core/func_integer.inl
/// @date 2010-03-17 / 2011-06-15
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#include "type_vec2.hpp"
#include "type_vec3.hpp"
#include "type_vec4.hpp"
#include "type_int.hpp"
#include "_vectorize.hpp"
#if(GLM_ARCH != GLM_ARCH_PURE)
#if(GLM_COMPILER & GLM_COMPILER_VC)
#	include <intrin.h>
#	pragma intrinsic(_BitScanReverse)
#endif//(GLM_COMPILER & GLM_COMPILER_VC)
#endif//(GLM_ARCH != GLM_ARCH_PURE)
#include <limits>

namespace glm
{
	// uaddCarry
	template <>
	GLM_FUNC_QUALIFIER uint uaddCarry
	(
		uint const & x,
		uint const & y,
		uint & Carry
	)
	{
		uint64 Value64 = static_cast<uint64>(x) + static_cast<uint64>(y);
		uint32 Result = static_cast<uint32>(Value64 % (static_cast<uint64>(1) << static_cast<uint64>(32)));
		Carry = (Value64 % (static_cast<uint64>(1) << static_cast<uint64>(32))) > 1 ? static_cast<uint32>(1) : static_cast<uint32>(0);
		return Result;
	}

	template <>
	GLM_FUNC_QUALIFIER uvec2 uaddCarry
	(
		uvec2 const & x,
		uvec2 const & y,
		uvec2 & Carry
	)
	{
		return uvec2(
			uaddCarry(x[0], y[0], Carry[0]),
			uaddCarry(x[1], y[1], Carry[1]));
	}

	template <>
	GLM_FUNC_QUALIFIER uvec3 uaddCarry
	(
		uvec3 const & x,
		uvec3 const & y,
		uvec3 & Carry
	)
	{
		return uvec3(
			uaddCarry(x[0], y[0], Carry[0]),
			uaddCarry(x[1], y[1], Carry[1]),
			uaddCarry(x[2], y[2], Carry[2]));
	}

	template <>
	GLM_FUNC_QUALIFIER uvec4 uaddCarry
	(
		uvec4 const & x,
		uvec4 const & y,
		uvec4 & Carry
	)
	{
		return uvec4(
			uaddCarry(x[0], y[0], Carry[0]),
			uaddCarry(x[1], y[1], Carry[1]),
			uaddCarry(x[2], y[2], Carry[2]),
			uaddCarry(x[3], y[3], Carry[3]));
	}

	// usubBorrow
	template <>
	GLM_FUNC_QUALIFIER uint usubBorrow
	(
		uint const & x,
		uint const & y,
		uint & Borrow
	)
	{
		GLM_STATIC_ASSERT(sizeof(uint) == sizeof(uint32), "uint and uint32 size mismatch");

		Borrow = x >= y ? static_cast<uint32>(0) : static_cast<uint32>(1);
		if(y >= x)
			return y - x;
		else
			return static_cast<uint32>((static_cast<int64>(1) << static_cast<int64>(32)) + (static_cast<int64>(y) - static_cast<int64>(x)));
	}

	template <>
	GLM_FUNC_QUALIFIER uvec2 usubBorrow
	(
		uvec2 const & x,
		uvec2 const & y,
		uvec2 & Borrow
	)
	{
		return uvec2(
			usubBorrow(x[0], y[0], Borrow[0]),
			usubBorrow(x[1], y[1], Borrow[1]));
	}

	template <>
	GLM_FUNC_QUALIFIER uvec3 usubBorrow
	(
		uvec3 const & x,
		uvec3 const & y,
		uvec3 & Borrow
	)
	{
		return uvec3(
			usubBorrow(x[0], y[0], Borrow[0]),
			usubBorrow(x[1], y[1], Borrow[1]),
			usubBorrow(x[2], y[2], Borrow[2]));
	}

	template <>
	GLM_FUNC_QUALIFIER uvec4 usubBorrow
	(
		uvec4 const & x,
		uvec4 const & y,
		uvec4 & Borrow
	)
	{
		return uvec4(
			usubBorrow(x[0], y[0], Borrow[0]),
			usubBorrow(x[1], y[1], Borrow[1]),
			usubBorrow(x[2], y[2], Borrow[2]),
			usubBorrow(x[3], y[3], Borrow[3]));
	}

	// umulExtended
	template <>
	GLM_FUNC_QUALIFIER void umulExtended
	(
		uint const & x,
		uint const & y,
		uint & msb,
		uint & lsb
	)
	{
		GLM_STATIC_ASSERT(sizeof(uint) == sizeof(uint32), "uint and uint32 size mismatch");

		uint64 Value64 = static_cast<uint64>(x) * static_cast<uint64>(y);
		uint32* PointerMSB = (reinterpret_cast<uint32*>(&Value64) + 1);
		msb = *PointerMSB;
		uint32* PointerLSB = (reinterpret_cast<uint32*>(&Value64) + 0);
		lsb = *PointerLSB;
	}

	template <>
	GLM_FUNC_QUALIFIER void umulExtended
	(
		uvec2 const & x,
		uvec2 const & y,
		uvec2 & msb,
		uvec2 & lsb
	)
	{
		umulExtended(x[0], y[0], msb[0], lsb[0]);
		umulExtended(x[1], y[1], msb[1], lsb[1]);
	}

	template <>
	GLM_FUNC_QUALIFIER void umulExtended
	(
		uvec3 const & x,
		uvec3 const & y,
		uvec3 & msb,
		uvec3 & lsb
	)
	{
		umulExtended(x[0], y[0], msb[0], lsb[0]);
		umulExtended(x[1], y[1], msb[1], lsb[1]);
		umulExtended(x[2], y[2], msb[2], lsb[2]);
	}

	template <>
	GLM_FUNC_QUALIFIER void umulExtended
	(
		uvec4 const & x,
		uvec4 const & y,
		uvec4 & msb,
		uvec4 & lsb
	)
	{
		umulExtended(x[0], y[0], msb[0], lsb[0]);
		umulExtended(x[1], y[1], msb[1], lsb[1]);
		umulExtended(x[2], y[2], msb[2], lsb[2]);
		umulExtended(x[3], y[3], msb[3], lsb[3]);
	}

	// imulExtended
	template <>
	GLM_FUNC_QUALIFIER void imulExtended
	(
		int const & x,
		int const & y,
		int & msb,
		int & lsb
	)
	{
		GLM_STATIC_ASSERT(sizeof(int) == sizeof(int32), "int and int32 size mismatch");

		int64 Value64 = static_cast<int64>(x) * static_cast<int64>(y);
		int32* PointerMSB = (reinterpret_cast<int32*>(&Value64) + 1);
		msb = *PointerMSB;
		int32* PointerLSB = (reinterpret_cast<int32*>(&Value64));
		lsb = *PointerLSB;
	}

	template <>
	GLM_FUNC_QUALIFIER void imulExtended
	(
		ivec2 const & x,
		ivec2 const & y,
		ivec2 & msb,
		ivec2 & lsb
	)
	{
		imulExtended(x[0], y[0], msb[0], lsb[0]),
		imulExtended(x[1], y[1], msb[1], lsb[1]);
	}

	template <>
	GLM_FUNC_QUALIFIER void imulExtended
	(
		ivec3 const & x,
		ivec3 const & y,
		ivec3 & msb,
		ivec3 & lsb
	)
	{
		imulExtended(x[0], y[0], msb[0], lsb[0]),
		imulExtended(x[1], y[1], msb[1], lsb[1]);
		imulExtended(x[2], y[2], msb[2], lsb[2]);
	}

	template <>
	GLM_FUNC_QUALIFIER void imulExtended
	(
		ivec4 const & x,
		ivec4 const & y,
		ivec4 & msb,
		ivec4 & lsb
	)
	{
		imulExtended(x[0], y[0], msb[0], lsb[0]),
		imulExtended(x[1], y[1], msb[1], lsb[1]);
		imulExtended(x[2], y[2], msb[2], lsb[2]);
		imulExtended(x[3], y[3], msb[3], lsb[3]);
	}

	// bitfieldExtract
	template <typename genIUType>
	GLM_FUNC_QUALIFIER genIUType bitfieldExtract
	(
		genIUType const & Value,
		int const & Offset,
		int const & Bits
	)
	{
		int GenSize = int(sizeof(genIUType)) << int(3);

		assert(Offset + Bits <= GenSize);

		genIUType ShiftLeft = Bits ? Value << (GenSize - (Bits + Offset)) : genIUType(0);
		genIUType ShiftBack = ShiftLeft >> genIUType(GenSize - Bits);

		return ShiftBack;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec2<T, P> bitfieldExtract
	(
		detail::tvec2<T, P> const & Value,
		int const & Offset,
		int const & Bits
	)
	{
		return detail::tvec2<T, P>(
			bitfieldExtract(Value[0], Offset, Bits),
			bitfieldExtract(Value[1], Offset, Bits));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> bitfieldExtract
	(
		detail::tvec3<T, P> const & Value,
		int const & Offset,
		int const & Bits
	)
	{
		return detail::tvec3<T, P>(
			bitfieldExtract(Value[0], Offset, Bits),
			bitfieldExtract(Value[1], Offset, Bits),
			bitfieldExtract(Value[2], Offset, Bits));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec4<T, P> bitfieldExtract
	(
		detail::tvec4<T, P> const & Value,
		int const & Offset,
		int const & Bits
	)
	{
		return detail::tvec4<T, P>(
			bitfieldExtract(Value[0], Offset, Bits),
			bitfieldExtract(Value[1], Offset, Bits),
			bitfieldExtract(Value[2], Offset, Bits),
			bitfieldExtract(Value[3], Offset, Bits));
	}

	// bitfieldInsert
	template <typename genIUType>
	GLM_FUNC_QUALIFIER genIUType bitfieldInsert
	(
		genIUType const & Base,
		genIUType const & Insert,
		int const & Offset,
		int const & Bits
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<genIUType>::is_integer, "'bitfieldInsert' only accept integer values");
		assert(Offset + Bits <= sizeof(genIUType));

		if(Bits == 0)
			return Base;

		genIUType Mask = 0;
		for(int Bit = Offset; Bit < Offset + Bits; ++Bit)
			Mask |= (1 << Bit);

		return (Base & ~Mask) | (Insert & Mask);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec2<T, P> bitfieldInsert
	(
		detail::tvec2<T, P> const & Base,
		detail::tvec2<T, P> const & Insert,
		int const & Offset,
		int const & Bits
	)
	{
		return detail::tvec2<T, P>(
			bitfieldInsert(Base[0], Insert[0], Offset, Bits),
			bitfieldInsert(Base[1], Insert[1], Offset, Bits));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<T, P> bitfieldInsert
	(
		detail::tvec3<T, P> const & Base,
		detail::tvec3<T, P> const & Insert,
		int const & Offset,
		int const & Bits
	)
	{
		return detail::tvec3<T, P>(
			bitfieldInsert(Base[0], Insert[0], Offset, Bits),
			bitfieldInsert(Base[1], Insert[1], Offset, Bits),
			bitfieldInsert(Base[2], Insert[2], Offset, Bits));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec4<T, P> bitfieldInsert
	(
		detail::tvec4<T, P> const & Base,
		detail::tvec4<T, P> const & Insert,
		int const & Offset,
		int const & Bits
	)
	{
		return detail::tvec4<T, P>(
			bitfieldInsert(Base[0], Insert[0], Offset, Bits),
			bitfieldInsert(Base[1], Insert[1], Offset, Bits),
			bitfieldInsert(Base[2], Insert[2], Offset, Bits),
			bitfieldInsert(Base[3], Insert[3], Offset, Bits));
	}

	// bitfieldReverse
	template <typename genIUType>
	GLM_FUNC_QUALIFIER genIUType bitfieldReverse(genIUType const & Value)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<genIUType>::is_integer, "'bitfieldReverse' only accept integer values");

		genIUType Out = 0;
		std::size_t BitSize = sizeof(genIUType) * 8;
		for(std::size_t i = 0; i < BitSize; ++i)
			if(Value & (genIUType(1) << i))
				Out |= genIUType(1) << (BitSize - 1 - i);
		return Out;
	}	

	VECTORIZE_VEC(bitfieldReverse)

	// bitCount
	template <typename genIUType>
	GLM_FUNC_QUALIFIER int bitCount(genIUType const & Value)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<genIUType>::is_integer, "'bitCount' only accept integer values");

		int Count = 0;
		for(std::size_t i = 0; i < sizeof(genIUType) * std::size_t(8); ++i)
		{
			if(Value & (1 << i))
				++Count;
		}
		return Count;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec2<int, P> bitCount
	(
		detail::tvec2<T, P> const & value
	)
	{
		return detail::tvec2<int, P>(
			bitCount(value[0]),
			bitCount(value[1]));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<int, P> bitCount
	(
		detail::tvec3<T, P> const & value
	)
	{
		return detail::tvec3<int, P>(
			bitCount(value[0]),
			bitCount(value[1]),
			bitCount(value[2]));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec4<int, P> bitCount
	(
		detail::tvec4<T, P> const & value
	)
	{
		return detail::tvec4<int, P>(
			bitCount(value[0]),
			bitCount(value[1]),
			bitCount(value[2]),
			bitCount(value[3]));
	}

	// findLSB
	template <typename genIUType>
	GLM_FUNC_QUALIFIER int findLSB
	(
		genIUType const & Value
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<genIUType>::is_integer, "'findLSB' only accept integer values");
		if(Value == 0)
			return -1;

		genIUType Bit;
		for(Bit = genIUType(0); !(Value & (1 << Bit)); ++Bit){}
		return Bit;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec2<int, P> findLSB
	(
		detail::tvec2<T, P> const & value
	)
	{
		return detail::tvec2<int, P>(
			findLSB(value[0]),
			findLSB(value[1]));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<int, P> findLSB
	(
		detail::tvec3<T, P> const & value
	)
	{
		return detail::tvec3<int, P>(
			findLSB(value[0]),
			findLSB(value[1]),
			findLSB(value[2]));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec4<int, P> findLSB
	(
		detail::tvec4<T, P> const & value
	)
	{
		return detail::tvec4<int, P>(
			findLSB(value[0]),
			findLSB(value[1]),
			findLSB(value[2]),
			findLSB(value[3]));
	}

	// findMSB
#if((GLM_ARCH != GLM_ARCH_PURE) && (GLM_COMPILER & GLM_COMPILER_VC))

	template <typename genIUType>
	GLM_FUNC_QUALIFIER int findMSB
	(
		genIUType const & Value
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<genIUType>::is_integer, "'findMSB' only accept integer values");
		if(Value == 0)
			return -1;

		unsigned long Result(0);
		_BitScanReverse(&Result, Value);
		return int(Result);
	}
/*
// __builtin_clz seems to be buggy as it crasks for some values, from 0x00200000 to 80000000
#elif((GLM_ARCH != GLM_ARCH_PURE) && (GLM_COMPILER & GLM_COMPILER_GCC) && (GLM_COMPILER >= GLM_COMPILER_GCC40))

	template <typename genIUType>
	GLM_FUNC_QUALIFIER int findMSB
	(
		genIUType const & Value
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<genIUType>::is_integer, "'findMSB' only accept integer values");
		if(Value == 0)
			return -1;

		// clz returns the number or trailing 0-bits; see
		// http://gcc.gnu.org/onlinedocs/gcc-4.7.1/gcc/Other-Builtins.html
		//
		// NoteBecause __builtin_clz only works for unsigned ints, this
		// implementation will not work for 64-bit integers.
		//
		return 31 - __builtin_clzl(Value);
	}
*/
#else

/* SSE implementation idea

		__m128i const Zero = _mm_set_epi32( 0,  0,  0,  0);
		__m128i const One = _mm_set_epi32( 1,  1,  1,  1);
		__m128i Bit = _mm_set_epi32(-1, -1, -1, -1);
		__m128i Tmp = _mm_set_epi32(Value, Value, Value, Value);
		__m128i Mmi = Zero;
		for(int i = 0; i < 32; ++i)
		{
			__m128i Shilt = _mm_and_si128(_mm_cmpgt_epi32(Tmp, One), One);
			Tmp = _mm_srai_epi32(Tmp, One);
			Bit = _mm_add_epi32(Bit, _mm_and_si128(Shilt, i));
			Mmi = _mm_and_si128(Mmi, One);
		}
		return Bit;

*/

	template <typename genIUType>
	GLM_FUNC_QUALIFIER int findMSB
	(
		genIUType const & Value
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<genIUType>::is_integer, "'findMSB' only accept integer values");
		
		if(Value == genIUType(0) || Value == genIUType(-1))
			return -1;
		else if(Value > 0)
		{
			genIUType Bit = genIUType(-1);
			for(genIUType tmp = Value; tmp > 0; tmp >>= 1, ++Bit){}
			return Bit;
		}
		else //if(Value < 0)
		{
			int const BitCount(sizeof(genIUType) * 8);
			int MostSignificantBit(-1);
			for(int BitIndex(0); BitIndex < BitCount; ++BitIndex)
				MostSignificantBit = (Value & (1 << BitIndex)) ? MostSignificantBit : BitIndex;
			assert(MostSignificantBit >= 0);
			return MostSignificantBit;
		}
	}
#endif//(GLM_COMPILER)

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec2<int, P> findMSB
	(
		detail::tvec2<T, P> const & value
	)
	{
		return detail::tvec2<int, P>(
			findMSB(value[0]),
			findMSB(value[1]));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec3<int, P> findMSB
	(
		detail::tvec3<T, P> const & value
	)
	{
		return detail::tvec3<int, P>(
			findMSB(value[0]),
			findMSB(value[1]),
			findMSB(value[2]));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER detail::tvec4<int, P> findMSB
	(
		detail::tvec4<T, P> const & value
	)
	{
		return detail::tvec4<int, P>(
			findMSB(value[0]),
			findMSB(value[1]),
			findMSB(value[2]),
			findMSB(value[3]));
	}
}//namespace glm
