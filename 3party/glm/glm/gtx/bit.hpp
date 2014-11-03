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
/// @ref gtx_bit
/// @file glm/gtx/bit.hpp
/// @date 2007-03-14 / 2011-06-07
/// @author Christophe Riccio
///
/// @see core (dependence)
/// @see gtc_half_float (dependence)
///
/// @defgroup gtx_bit GLM_GTX_bit
/// @ingroup gtx
/// 
/// @brief Allow to perform bit operations on integer values
/// 
/// <glm/gtx/bit.hpp> need to be included to use these functionalities.
///////////////////////////////////////////////////////////////////////////////////

#ifndef GLM_GTX_bit
#define GLM_GTX_bit

// Dependencies
#include "../detail/type_int.hpp"
#include "../detail/setup.hpp"
#include <cstddef>

#if(defined(GLM_MESSAGES) && !defined(GLM_EXT_INCLUDED))
#	pragma message("GLM: GLM_GTX_bit extension included")
#endif

namespace glm
{
	/// @addtogroup gtx_bit
	/// @{

	/// Build a mask of 'count' bits
	/// @see gtx_bit
	template <typename genIType>
	GLM_FUNC_DECL genIType mask(genIType const & count);

	//! Find the highest bit set to 1 in a integer variable and return its value. 
	/// @see gtx_bit
	template <typename genType> 
	GLM_FUNC_DECL genType highestBitValue(genType const & value);

	//! Return true if the value is a power of two number. 
	/// @see gtx_bit
	template <typename genType> 
	GLM_FUNC_DECL bool isPowerOfTwo(genType const & value);

	//! Return the power of two number which value is just higher the input value.
	/// @see gtx_bit
	template <typename genType> 
	GLM_FUNC_DECL genType powerOfTwoAbove(genType const & value);

	//! Return the power of two number which value is just lower the input value. 
	/// @see gtx_bit
	template <typename genType> 
	GLM_FUNC_DECL genType powerOfTwoBelow(genType const & value);

	//! Return the power of two number which value is the closet to the input value. 
	/// @see gtx_bit
	template <typename genType> 
	GLM_FUNC_DECL genType powerOfTwoNearest(genType const & value);

	//! Revert all bits of any integer based type. 
	/// @see gtx_bit
	template <typename genType> 
	GLM_DEPRECATED GLM_FUNC_DECL genType bitRevert(genType const & value);

	//! Rotate all bits to the right.
	/// @see gtx_bit
	template <typename genType>
	GLM_FUNC_DECL genType bitRotateRight(genType const & In, std::size_t Shift);

	//! Rotate all bits to the left.
	/// @see gtx_bit
	template <typename genType>
	GLM_FUNC_DECL genType bitRotateLeft(genType const & In, std::size_t Shift);

	//! Set to 1 a range of bits.
	/// @see gtx_bit
	template <typename genIUType>
	GLM_FUNC_DECL genIUType fillBitfieldWithOne(
		genIUType const & Value,
		int const & FromBit, 
		int const & ToBit);

	//! Set to 0 a range of bits.
	/// @see gtx_bit
	template <typename genIUType>
	GLM_FUNC_DECL genIUType fillBitfieldWithZero(
		genIUType const & Value,
		int const & FromBit, 
		int const & ToBit);

	/// Interleaves the bits of x and y. 
	/// The first bit is the first bit of x followed by the first bit of y.
	/// The other bits are interleaved following the previous sequence.
	/// 
	/// @see gtx_bit
	GLM_FUNC_DECL int16 bitfieldInterleave(int8 x, int8 y);

	/// Interleaves the bits of x and y. 
	/// The first bit is the first bit of x followed by the first bit of y.
	/// The other bits are interleaved following the previous sequence.
	/// 
	/// @see gtx_bit
	GLM_FUNC_DECL uint16 bitfieldInterleave(uint8 x, uint8 y);

	/// Interleaves the bits of x and y. 
	/// The first bit is the first bit of x followed by the first bit of y.
	/// The other bits are interleaved following the previous sequence.
	/// 
	/// @see gtx_bit
	GLM_FUNC_DECL int32 bitfieldInterleave(int16 x, int16 y);

	/// Interleaves the bits of x and y. 
	/// The first bit is the first bit of x followed by the first bit of y.
	/// The other bits are interleaved following the previous sequence.
	/// 
	/// @see gtx_bit
	GLM_FUNC_DECL uint32 bitfieldInterleave(uint16 x, uint16 y);

	/// Interleaves the bits of x and y. 
	/// The first bit is the first bit of x followed by the first bit of y.
	/// The other bits are interleaved following the previous sequence.
	/// 
	/// @see gtx_bit
	GLM_FUNC_DECL int64 bitfieldInterleave(int32 x, int32 y);

	/// Interleaves the bits of x and y. 
	/// The first bit is the first bit of x followed by the first bit of y.
	/// The other bits are interleaved following the previous sequence.
	/// 
	/// @see gtx_bit
	GLM_FUNC_DECL uint64 bitfieldInterleave(uint32 x, uint32 y);

	/// Interleaves the bits of x, y and z. 
	/// The first bit is the first bit of x followed by the first bit of y and the first bit of z.
	/// The other bits are interleaved following the previous sequence.
	/// 
	/// @see gtx_bit
	GLM_FUNC_DECL int32 bitfieldInterleave(int8 x, int8 y, int8 z);

	/// Interleaves the bits of x, y and z. 
	/// The first bit is the first bit of x followed by the first bit of y and the first bit of z.
	/// The other bits are interleaved following the previous sequence.
	/// 
	/// @see gtx_bit
	GLM_FUNC_DECL uint32 bitfieldInterleave(uint8 x, uint8 y, uint8 z);

	/// Interleaves the bits of x, y and z. 
	/// The first bit is the first bit of x followed by the first bit of y and the first bit of z.
	/// The other bits are interleaved following the previous sequence.
	/// 
	/// @see gtx_bit
	GLM_FUNC_DECL int64 bitfieldInterleave(int16 x, int16 y, int16 z);

	/// Interleaves the bits of x, y and z. 
	/// The first bit is the first bit of x followed by the first bit of y and the first bit of z.
	/// The other bits are interleaved following the previous sequence.
	/// 
	/// @see gtx_bit
	GLM_FUNC_DECL uint64 bitfieldInterleave(uint16 x, uint16 y, uint16 z);

	/// Interleaves the bits of x, y and z. 
	/// The first bit is the first bit of x followed by the first bit of y and the first bit of z.
	/// The other bits are interleaved following the previous sequence.
	/// 
	/// @see gtx_bit
	GLM_FUNC_DECL int64 bitfieldInterleave(int32 x, int32 y, int32 z);

	/// Interleaves the bits of x, y and z. 
	/// The first bit is the first bit of x followed by the first bit of y and the first bit of z.
	/// The other bits are interleaved following the previous sequence.
	/// 
	/// @see gtx_bit
	GLM_FUNC_DECL uint64 bitfieldInterleave(uint32 x, uint32 y, uint32 z);

	/// Interleaves the bits of x, y, z and w. 
	/// The first bit is the first bit of x followed by the first bit of y, the first bit of z and finally the first bit of w.
	/// The other bits are interleaved following the previous sequence.
	/// 
	/// @see gtx_bit
	GLM_FUNC_DECL int32 bitfieldInterleave(int8 x, int8 y, int8 z, int8 w);

	/// Interleaves the bits of x, y, z and w. 
	/// The first bit is the first bit of x followed by the first bit of y, the first bit of z and finally the first bit of w.
	/// The other bits are interleaved following the previous sequence.
	/// 
	/// @see gtx_bit
	GLM_FUNC_DECL uint32 bitfieldInterleave(uint8 x, uint8 y, uint8 z, uint8 w);

	/// Interleaves the bits of x, y, z and w. 
	/// The first bit is the first bit of x followed by the first bit of y, the first bit of z and finally the first bit of w.
	/// The other bits are interleaved following the previous sequence.
	/// 
	/// @see gtx_bit
	GLM_FUNC_DECL int64 bitfieldInterleave(int16 x, int16 y, int16 z, int16 w);

	/// Interleaves the bits of x, y, z and w. 
	/// The first bit is the first bit of x followed by the first bit of y, the first bit of z and finally the first bit of w.
	/// The other bits are interleaved following the previous sequence.
	/// 
	/// @see gtx_bit
	GLM_FUNC_DECL uint64 bitfieldInterleave(uint16 x, uint16 y, uint16 z, uint16 w);

	/// @}
} //namespace glm

#include "bit.inl"

#endif//GLM_GTX_bit
