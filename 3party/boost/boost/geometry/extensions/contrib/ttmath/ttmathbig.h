/*
 * This file is a part of TTMath Bignum Library
 * and is distributed under the (new) BSD licence.
 * Author: Tomasz Sowa <t.sowa@ttmath.org>
 */

/* 
 * Copyright (c) 2006-2011, Tomasz Sowa
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *    
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *    
 *  * Neither the name Tomasz Sowa nor the names of contributors to this
 *    project may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef headerfilettmathbig
#define headerfilettmathbig

/*!
	\file ttmathbig.h
    \brief A Class for representing floating point numbers
*/

#include "ttmathint.h"
#include "ttmaththreads.h"

#include <iostream>

#ifdef TTMATH_MULTITHREADS
#include <signal.h>
#endif

namespace ttmath
{


/*!
	\brief Big implements the floating point numbers
*/
template <uint exp, uint man>
class Big
{

/*
	value = mantissa * 2^exponent	

	exponent - an integer value with a sign
	mantissa - an integer value without a sing

	mantissa must be pushed into the left side that is the highest bit from 
	mantissa must be one (of course if there's another value than zero) -- this job
	(pushing bits into the left side) making Standardizing() method

	for example:
	if we want to store value one (1) into our Big object we must:
		set mantissa to 1
		set exponent to 0
		set info to 0
		and call method Standardizing()
*/


public:

Int<exp>  exponent;
UInt<man> mantissa;
unsigned char info;


/*!
	Sign
	the mask of a bit from 'info' which means that there is a sign
	(when the bit is set)
*/
#define TTMATH_BIG_SIGN 128


/*!
	Not a number
	if this bit is set that there is not a valid number
*/
#define TTMATH_BIG_NAN  64


/*!
	Zero
	if this bit is set that there is value zero
	mantissa should be zero and exponent should be zero too
	(the Standardizing() method does this)
*/
#define TTMATH_BIG_ZERO  32


	/*!
		this method sets NaN if there was a carry (and returns 1 in such a case)

		c can be 0, 1 or other value different from zero
	*/
	uint CheckCarry(uint c)
	{
		if( c != 0 )
		{
			SetNan();
			return 1;
		}

	return 0;
	}

public:


	/*!
		returning the string represents the currect type of the library
		we have following types:
		  asm_vc_32   - with asm code designed for Microsoft Visual C++ (32 bits)
		  asm_gcc_32  - with asm code designed for GCC (32 bits)
		  asm_vc_64   - with asm for VC (64 bit)
		  asm_gcc_64  - with asm for GCC (64 bit)
		  no_asm_32   - pure C++ version (32 bit) - without any asm code
		  no_asm_64   - pure C++ version (64 bit) - without any asm code
	*/
	static const char * LibTypeStr()
	{
		return UInt<man>::LibTypeStr();
	}


	/*!
		returning the currect type of the library
	*/
	static LibTypeCode LibType()
	{
		return UInt<man>::LibType();
	}



	/*!
		this method moves all bits from mantissa into its left side
		(suitably changes the exponent) or if the mantissa is zero
		it sets the exponent to zero as well
		(and clears the sign bit and sets the zero bit)

		it can return a carry
		the carry will be when we don't have enough space in the exponent

		you don't have to use this method if you don't change the mantissa
		and exponent directly
	*/
	uint Standardizing()
	{
		if( mantissa.IsTheHighestBitSet() )
		{
			ClearInfoBit(TTMATH_BIG_ZERO);
			return 0;
		}

		if( CorrectZero() )
			return 0;

		uint comp = mantissa.CompensationToLeft();

	return exponent.Sub( comp );
	}


private:

	/*!
		if the mantissa is equal zero this method sets exponent to zero and
		info without the sign

		it returns true if there was the correction
	*/
	bool CorrectZero()
	{
		if( mantissa.IsZero() )
		{
			SetInfoBit(TTMATH_BIG_ZERO);
			ClearInfoBit(TTMATH_BIG_SIGN);
			exponent.SetZero();

			return true;
		}
		else
		{
			ClearInfoBit(TTMATH_BIG_ZERO);
		}

	return false;
	}


public:

	/*!
		this method clears a specific bit in the 'info' variable

		bit is one of: TTMATH_BIG_SIGN, TTMATH_BIG_NAN etc.
	*/
	void ClearInfoBit(unsigned char bit)
	{
		info = info & (~bit);
	}


	/*!
		this method sets a specific bit in the 'info' variable

		bit is one of: TTMATH_BIG_SIGN, TTMATH_BIG_NAN etc.

	*/
	void SetInfoBit(unsigned char bit)
	{
		info = info | bit;
	}


	/*!
		this method returns true if a specific bit in the 'info' variable is set

		bit is one of: TTMATH_BIG_SIGN, TTMATH_BIG_NAN etc.
	*/
	bool IsInfoBit(unsigned char bit) const
	{
		return (info & bit) != 0;
	}


	/*!
		this method sets zero
	*/
	void SetZero()
	{
		info = TTMATH_BIG_ZERO;
		exponent.SetZero();
		mantissa.SetZero();

		/*
			we don't have to compensate zero
		*/
	}

	
	/*!
		this method sets one
	*/
	void SetOne()
	{
		info = 0;
		mantissa.SetZero();
		mantissa.table[man-1] = TTMATH_UINT_HIGHEST_BIT;
		exponent = -sint(man * TTMATH_BITS_PER_UINT - 1);

		// don't have to Standardize() - the last bit from mantissa is set
	}


	/*!
		this method sets value 0.5
	*/
	void Set05()
	{
		SetOne();
		exponent.SubOne();
	}


	/*!
		this method sets NaN flag (Not a Number)
		when this flag is set that means there is no a valid number
	*/
	void SetNan()
	{
		SetInfoBit(TTMATH_BIG_NAN);
	}


	/*!
		this method sets NaN flag (Not a Number)
		also clears the mantissa and exponent (similarly as it would be a zero value)
	*/
	void SetZeroNan()
	{
		SetZero();
		SetNan();
	}


	/*!
		this method swappes this for an argument
	*/
	void Swap(Big<exp, man> & ss2)
	{
		unsigned char info_temp = info;
		info = ss2.info;
		ss2.info = info_temp;

		exponent.Swap(ss2.exponent);
		mantissa.Swap(ss2.mantissa);
	}


private:

	/*!
		this method sets the mantissa of the value of pi
	*/
	void SetMantissaPi()
	{
	// this is a static table which represents the value of Pi (mantissa of it)
	// (first is the highest word)
	// we must define this table as 'unsigned int' because 
	// both on 32bit and 64bit platforms this table is 32bit
	static const unsigned int temp_table[] = {
		0xc90fdaa2, 0x2168c234, 0xc4c6628b, 0x80dc1cd1, 0x29024e08, 0x8a67cc74, 0x020bbea6, 0x3b139b22, 
		0x514a0879, 0x8e3404dd, 0xef9519b3, 0xcd3a431b, 0x302b0a6d, 0xf25f1437, 0x4fe1356d, 0x6d51c245, 
		0xe485b576, 0x625e7ec6, 0xf44c42e9, 0xa637ed6b, 0x0bff5cb6, 0xf406b7ed, 0xee386bfb, 0x5a899fa5, 
		0xae9f2411, 0x7c4b1fe6, 0x49286651, 0xece45b3d, 0xc2007cb8, 0xa163bf05, 0x98da4836, 0x1c55d39a, 
		0x69163fa8, 0xfd24cf5f, 0x83655d23, 0xdca3ad96, 0x1c62f356, 0x208552bb, 0x9ed52907, 0x7096966d, 
		0x670c354e, 0x4abc9804, 0xf1746c08, 0xca18217c, 0x32905e46, 0x2e36ce3b, 0xe39e772c, 0x180e8603, 
		0x9b2783a2, 0xec07a28f, 0xb5c55df0, 0x6f4c52c9, 0xde2bcbf6, 0x95581718, 0x3995497c, 0xea956ae5, 
		0x15d22618, 0x98fa0510, 0x15728e5a, 0x8aaac42d, 0xad33170d, 0x04507a33, 0xa85521ab, 0xdf1cba64, 
		0xecfb8504, 0x58dbef0a, 0x8aea7157, 0x5d060c7d, 0xb3970f85, 0xa6e1e4c7, 0xabf5ae8c, 0xdb0933d7, 
		0x1e8c94e0, 0x4a25619d, 0xcee3d226, 0x1ad2ee6b, 0xf12ffa06, 0xd98a0864, 0xd8760273, 0x3ec86a64, 
		0x521f2b18, 0x177b200c, 0xbbe11757, 0x7a615d6c, 0x770988c0, 0xbad946e2, 0x08e24fa0, 0x74e5ab31, 
		0x43db5bfc, 0xe0fd108e, 0x4b82d120, 0xa9210801, 0x1a723c12, 0xa787e6d7, 0x88719a10, 0xbdba5b26, 
		0x99c32718, 0x6af4e23c, 0x1a946834, 0xb6150bda, 0x2583e9ca, 0x2ad44ce8, 0xdbbbc2db, 0x04de8ef9, 
		0x2e8efc14, 0x1fbecaa6, 0x287c5947, 0x4e6bc05d, 0x99b2964f, 0xa090c3a2, 0x233ba186, 0x515be7ed, 
		0x1f612970, 0xcee2d7af, 0xb81bdd76, 0x2170481c, 0xd0069127, 0xd5b05aa9, 0x93b4ea98, 0x8d8fddc1, 
		0x86ffb7dc, 0x90a6c08f, 0x4df435c9, 0x34028492, 0x36c3fab4, 0xd27c7026, 0xc1d4dcb2, 0x602646de, 
		0xc9751e76, 0x3dba37bd, 0xf8ff9406, 0xad9e530e, 0xe5db382f, 0x413001ae, 0xb06a53ed, 0x9027d831, 
		0x179727b0, 0x865a8918, 0xda3edbeb, 0xcf9b14ed, 0x44ce6cba, 0xced4bb1b, 0xdb7f1447, 0xe6cc254b, 
		0x33205151, 0x2bd7af42, 0x6fb8f401, 0x378cd2bf, 0x5983ca01, 0xc64b92ec, 0xf032ea15, 0xd1721d03, 
		0xf482d7ce, 0x6e74fef6, 0xd55e702f, 0x46980c82, 0xb5a84031, 0x900b1c9e, 0x59e7c97f, 0xbec7e8f3, 
		0x23a97a7e, 0x36cc88be, 0x0f1d45b7, 0xff585ac5, 0x4bd407b2, 0x2b4154aa, 0xcc8f6d7e, 0xbf48e1d8, 
		0x14cc5ed2, 0x0f8037e0, 0xa79715ee, 0xf29be328, 0x06a1d58b, 0xb7c5da76, 0xf550aa3d, 0x8a1fbff0, 
		0xeb19ccb1, 0xa313d55c, 0xda56c9ec, 0x2ef29632, 0x387fe8d7, 0x6e3c0468, 0x043e8f66, 0x3f4860ee, 
		0x12bf2d5b, 0x0b7474d6, 0xe694f91e, 0x6dbe1159, 0x74a3926f, 0x12fee5e4, 0x38777cb6, 0xa932df8c, 
		0xd8bec4d0, 0x73b931ba, 0x3bc832b6, 0x8d9dd300, 0x741fa7bf, 0x8afc47ed, 0x2576f693, 0x6ba42466, 
		0x3aab639c, 0x5ae4f568, 0x3423b474, 0x2bf1c978, 0x238f16cb, 0xe39d652d, 0xe3fdb8be, 0xfc848ad9, 
		0x22222e04, 0xa4037c07, 0x13eb57a8, 0x1a23f0c7, 0x3473fc64, 0x6cea306b, 0x4bcbc886, 0x2f8385dd, 
		0xfa9d4b7f, 0xa2c087e8, 0x79683303, 0xed5bdd3a, 0x062b3cf5, 0xb3a278a6, 0x6d2a13f8, 0x3f44f82d, 
		0xdf310ee0, 0x74ab6a36, 0x4597e899, 0xa0255dc1, 0x64f31cc5, 0x0846851d, 0xf9ab4819, 0x5ded7ea1, 
		0xb1d510bd, 0x7ee74d73, 0xfaf36bc3, 0x1ecfa268, 0x359046f4, 0xeb879f92, 0x4009438b, 0x481c6cd7, 
		0x889a002e, 0xd5ee382b, 0xc9190da6, 0xfc026e47, 0x9558e447, 0x5677e9aa, 0x9e3050e2, 0x765694df, 
		0xc81f56e8, 0x80b96e71, 0x60c980dd, 0x98a573ea, 0x4472065a, 0x139cd290, 0x6cd1cb72, 0x9ec52a53 // last one was: 0x9ec52a52
		//0x86d44014, ...
		// (the last word 0x9ec52a52 was rounded up because the next one is 0x86d44014 -- first bit is one 0x8..)
		// 256 32bit words for the mantissa -- about 2464 valid decimal digits
		};
		// the value of PI is comming from the website http://zenwerx.com/pi.php
		// 3101 digits were taken from this website
		//  (later the digits were compared with:
		//   http://www.eveandersson.com/pi/digits/1000000 and http://www.geom.uiuc.edu/~huberty/math5337/groupe/digits.html )
		// and they were set into Big<1,400> type (using operator=(const char*) on a 32bit platform)
		// and then the first 256 words were taken into this table
		// (TTMATH_BUILTIN_VARIABLES_SIZE on 32bit platform should have the value 256,
		// and on 64bit platform value 128 (256/2=128))
	
		mantissa.SetFromTable(temp_table, sizeof(temp_table) / sizeof(int));
	}

public:


	/*!
		this method sets the value of pi
	*/
	void SetPi()
	{
		SetMantissaPi();
		info = 0;
		exponent = -sint(man)*sint(TTMATH_BITS_PER_UINT) + 2;
	}


	/*!
		this method sets the value of 0.5 * pi
	*/
	void Set05Pi()
	{
		SetMantissaPi();
		info = 0;
		exponent = -sint(man)*sint(TTMATH_BITS_PER_UINT) + 1;
	}


	/*!
		this method sets the value of 2 * pi
	*/
	void Set2Pi()
	{
		SetMantissaPi();
		info = 0;
		exponent = -sint(man)*sint(TTMATH_BITS_PER_UINT) + 3;
	}


	/*!
		this method sets the value of e
		(the base of the natural logarithm)
	*/
	void SetE()
	{
	static const unsigned int temp_table[] = {
		0xadf85458, 0xa2bb4a9a, 0xafdc5620, 0x273d3cf1, 0xd8b9c583, 0xce2d3695, 0xa9e13641, 0x146433fb, 
		0xcc939dce, 0x249b3ef9, 0x7d2fe363, 0x630c75d8, 0xf681b202, 0xaec4617a, 0xd3df1ed5, 0xd5fd6561, 
		0x2433f51f, 0x5f066ed0, 0x85636555, 0x3ded1af3, 0xb557135e, 0x7f57c935, 0x984f0c70, 0xe0e68b77, 
		0xe2a689da, 0xf3efe872, 0x1df158a1, 0x36ade735, 0x30acca4f, 0x483a797a, 0xbc0ab182, 0xb324fb61, 
		0xd108a94b, 0xb2c8e3fb, 0xb96adab7, 0x60d7f468, 0x1d4f42a3, 0xde394df4, 0xae56ede7, 0x6372bb19, 
		0x0b07a7c8, 0xee0a6d70, 0x9e02fce1, 0xcdf7e2ec, 0xc03404cd, 0x28342f61, 0x9172fe9c, 0xe98583ff, 
		0x8e4f1232, 0xeef28183, 0xc3fe3b1b, 0x4c6fad73, 0x3bb5fcbc, 0x2ec22005, 0xc58ef183, 0x7d1683b2, 
		0xc6f34a26, 0xc1b2effa, 0x886b4238, 0x611fcfdc, 0xde355b3b, 0x6519035b, 0xbc34f4de, 0xf99c0238, 
		0x61b46fc9, 0xd6e6c907, 0x7ad91d26, 0x91f7f7ee, 0x598cb0fa, 0xc186d91c, 0xaefe1309, 0x85139270, 
		0xb4130c93, 0xbc437944, 0xf4fd4452, 0xe2d74dd3, 0x64f2e21e, 0x71f54bff, 0x5cae82ab, 0x9c9df69e, 
		0xe86d2bc5, 0x22363a0d, 0xabc52197, 0x9b0deada, 0x1dbf9a42, 0xd5c4484e, 0x0abcd06b, 0xfa53ddef, 
		0x3c1b20ee, 0x3fd59d7c, 0x25e41d2b, 0x669e1ef1, 0x6e6f52c3, 0x164df4fb, 0x7930e9e4, 0xe58857b6, 
		0xac7d5f42, 0xd69f6d18, 0x7763cf1d, 0x55034004, 0x87f55ba5, 0x7e31cc7a, 0x7135c886, 0xefb4318a, 
		0xed6a1e01, 0x2d9e6832, 0xa907600a, 0x918130c4, 0x6dc778f9, 0x71ad0038, 0x092999a3, 0x33cb8b7a, 
		0x1a1db93d, 0x7140003c, 0x2a4ecea9, 0xf98d0acc, 0x0a8291cd, 0xcec97dcf, 0x8ec9b55a, 0x7f88a46b, 
		0x4db5a851, 0xf44182e1, 0xc68a007e, 0x5e0dd902, 0x0bfd64b6, 0x45036c7a, 0x4e677d2c, 0x38532a3a, 
		0x23ba4442, 0xcaf53ea6, 0x3bb45432, 0x9b7624c8, 0x917bdd64, 0xb1c0fd4c, 0xb38e8c33, 0x4c701c3a, 
		0xcdad0657, 0xfccfec71, 0x9b1f5c3e, 0x4e46041f, 0x388147fb, 0x4cfdb477, 0xa52471f7, 0xa9a96910, 
		0xb855322e, 0xdb6340d8, 0xa00ef092, 0x350511e3, 0x0abec1ff, 0xf9e3a26e, 0x7fb29f8c, 0x183023c3, 
		0x587e38da, 0x0077d9b4, 0x763e4e4b, 0x94b2bbc1, 0x94c6651e, 0x77caf992, 0xeeaac023, 0x2a281bf6, 
		0xb3a739c1, 0x22611682, 0x0ae8db58, 0x47a67cbe, 0xf9c9091b, 0x462d538c, 0xd72b0374, 0x6ae77f5e, 
		0x62292c31, 0x1562a846, 0x505dc82d, 0xb854338a, 0xe49f5235, 0xc95b9117, 0x8ccf2dd5, 0xcacef403, 
		0xec9d1810, 0xc6272b04, 0x5b3b71f9, 0xdc6b80d6, 0x3fdd4a8e, 0x9adb1e69, 0x62a69526, 0xd43161c1, 
		0xa41d570d, 0x7938dad4, 0xa40e329c, 0xcff46aaa, 0x36ad004c, 0xf600c838, 0x1e425a31, 0xd951ae64, 
		0xfdb23fce, 0xc9509d43, 0x687feb69, 0xedd1cc5e, 0x0b8cc3bd, 0xf64b10ef, 0x86b63142, 0xa3ab8829, 
		0x555b2f74, 0x7c932665, 0xcb2c0f1c, 0xc01bd702, 0x29388839, 0xd2af05e4, 0x54504ac7, 0x8b758282, 
		0x2846c0ba, 0x35c35f5c, 0x59160cc0, 0x46fd8251, 0x541fc68c, 0x9c86b022, 0xbb709987, 0x6a460e74, 
		0x51a8a931, 0x09703fee, 0x1c217e6c, 0x3826e52c, 0x51aa691e, 0x0e423cfc, 0x99e9e316, 0x50c1217b, 
		0x624816cd, 0xad9a95f9, 0xd5b80194, 0x88d9c0a0, 0xa1fe3075, 0xa577e231, 0x83f81d4a, 0x3f2fa457, 
		0x1efc8ce0, 0xba8a4fe8, 0xb6855dfe, 0x72b0a66e, 0xded2fbab, 0xfbe58a30, 0xfafabe1c, 0x5d71a87e, 
		0x2f741ef8, 0xc1fe86fe, 0xa6bbfde5, 0x30677f0d, 0x97d11d49, 0xf7a8443d, 0x0822e506, 0xa9f4614e, 
		0x011e2a94, 0x838ff88c, 0xd68c8bb7, 0xc51eef6d, 0x49ea8ab4, 0xf2c3df5b, 0xb4e0735a, 0xb0d68749
		// 0x2fe26dd4, ...
		// 256 32bit words for the mantissa -- about 2464 valid decimal digits
		};

		// above value was calculated using Big<1,400> type on a 32bit platform
		// and then the first 256 words were taken,
		// the calculating was made by using ExpSurrounding0(1) method
		// which took 1420 iterations
		// (the result was compared with e taken from http://antwrp.gsfc.nasa.gov/htmltest/gifcity/e.2mil)
		// (TTMATH_BUILTIN_VARIABLES_SIZE on 32bit platform should have the value 256,
		// and on 64bit platform value 128 (256/2=128))

		mantissa.SetFromTable(temp_table, sizeof(temp_table) / sizeof(int));
		exponent = -sint(man)*sint(TTMATH_BITS_PER_UINT) + 2;
		info = 0;
	}


	/*!
		this method sets the value of ln(2)
		the natural logarithm from 2
	*/
	void SetLn2()
	{
	static const unsigned int temp_table[] = {
		0xb17217f7, 0xd1cf79ab, 0xc9e3b398, 0x03f2f6af, 0x40f34326, 0x7298b62d, 0x8a0d175b, 0x8baafa2b, 
		0xe7b87620, 0x6debac98, 0x559552fb, 0x4afa1b10, 0xed2eae35, 0xc1382144, 0x27573b29, 0x1169b825, 
		0x3e96ca16, 0x224ae8c5, 0x1acbda11, 0x317c387e, 0xb9ea9bc3, 0xb136603b, 0x256fa0ec, 0x7657f74b, 
		0x72ce87b1, 0x9d6548ca, 0xf5dfa6bd, 0x38303248, 0x655fa187, 0x2f20e3a2, 0xda2d97c5, 0x0f3fd5c6, 
		0x07f4ca11, 0xfb5bfb90, 0x610d30f8, 0x8fe551a2, 0xee569d6d, 0xfc1efa15, 0x7d2e23de, 0x1400b396, 
		0x17460775, 0xdb8990e5, 0xc943e732, 0xb479cd33, 0xcccc4e65, 0x9393514c, 0x4c1a1e0b, 0xd1d6095d, 
		0x25669b33, 0x3564a337, 0x6a9c7f8a, 0x5e148e82, 0x074db601, 0x5cfe7aa3, 0x0c480a54, 0x17350d2c, 
		0x955d5179, 0xb1e17b9d, 0xae313cdb, 0x6c606cb1, 0x078f735d, 0x1b2db31b, 0x5f50b518, 0x5064c18b, 
		0x4d162db3, 0xb365853d, 0x7598a195, 0x1ae273ee, 0x5570b6c6, 0x8f969834, 0x96d4e6d3, 0x30af889b, 
		0x44a02554, 0x731cdc8e, 0xa17293d1, 0x228a4ef9, 0x8d6f5177, 0xfbcf0755, 0x268a5c1f, 0x9538b982, 
		0x61affd44, 0x6b1ca3cf, 0x5e9222b8, 0x8c66d3c5, 0x422183ed, 0xc9942109, 0x0bbb16fa, 0xf3d949f2, 
		0x36e02b20, 0xcee886b9, 0x05c128d5, 0x3d0bd2f9, 0x62136319, 0x6af50302, 0x0060e499, 0x08391a0c, 
		0x57339ba2, 0xbeba7d05, 0x2ac5b61c, 0xc4e9207c, 0xef2f0ce2, 0xd7373958, 0xd7622658, 0x901e646a, 
		0x95184460, 0xdc4e7487, 0x156e0c29, 0x2413d5e3, 0x61c1696d, 0xd24aaebd, 0x473826fd, 0xa0c238b9, 
		0x0ab111bb, 0xbd67c724, 0x972cd18b, 0xfbbd9d42, 0x6c472096, 0xe76115c0, 0x5f6f7ceb, 0xac9f45ae, 
		0xcecb72f1, 0x9c38339d, 0x8f682625, 0x0dea891e, 0xf07afff3, 0xa892374e, 0x175eb4af, 0xc8daadd8, 
		0x85db6ab0, 0x3a49bd0d, 0xc0b1b31d, 0x8a0e23fa, 0xc5e5767d, 0xf95884e0, 0x6425a415, 0x26fac51c, 
		0x3ea8449f, 0xe8f70edd, 0x062b1a63, 0xa6c4c60c, 0x52ab3316, 0x1e238438, 0x897a39ce, 0x78b63c9f, 
		0x364f5b8a, 0xef22ec2f, 0xee6e0850, 0xeca42d06, 0xfb0c75df, 0x5497e00c, 0x554b03d7, 0xd2874a00, 
		0x0ca8f58d, 0x94f0341c, 0xbe2ec921, 0x56c9f949, 0xdb4a9316, 0xf281501e, 0x53daec3f, 0x64f1b783, 
		0x154c6032, 0x0e2ff793, 0x33ce3573, 0xfacc5fdc, 0xf1178590, 0x3155bbd9, 0x0f023b22, 0x0224fcd8, 
		0x471bf4f4, 0x45f0a88a, 0x14f0cd97, 0x6ea354bb, 0x20cdb5cc, 0xb3db2392, 0x88d58655, 0x4e2a0e8a, 
		0x6fe51a8c, 0xfaa72ef2, 0xad8a43dc, 0x4212b210, 0xb779dfe4, 0x9d7307cc, 0x846532e4, 0xb9694eda, 
		0xd162af05, 0x3b1751f3, 0xa3d091f6, 0x56658154, 0x12b5e8c2, 0x02461069, 0xac14b958, 0x784934b8, 
		0xd6cce1da, 0xa5053701, 0x1aa4fb42, 0xb9a3def4, 0x1bda1f85, 0xef6fdbf2, 0xf2d89d2a, 0x4b183527, 
		0x8fd94057, 0x89f45681, 0x2b552879, 0xa6168695, 0xc12963b0, 0xff01eaab, 0x73e5b5c1, 0x585318e7, 
		0x624f14a5, 0x1a4a026b, 0x68082920, 0x57fd99b6, 0x6dc085a9, 0x8ac8d8ca, 0xf9eeeea9, 0x8a2400ca, 
		0xc95f260f, 0xd10036f9, 0xf91096ac, 0x3195220a, 0x1a356b2a, 0x73b7eaad, 0xaf6d6058, 0x71ef7afb, 
		0x80bc4234, 0x33562e94, 0xb12dfab4, 0x14451579, 0xdf59eae0, 0x51707062, 0x4012a829, 0x62c59cab, 
		0x347f8304, 0xd889659e, 0x5a9139db, 0x14efcc30, 0x852be3e8, 0xfc99f14d, 0x1d822dd6, 0xe2f76797, 
		0xe30219c8, 0xaa9ce884, 0x8a886eb3, 0xc87b7295, 0x988012e8, 0x314186ed, 0xbaf86856, 0xccd3c3b6, 
		0xee94e62f, 0x110a6783, 0xd2aae89c, 0xcc3b76fc, 0x435a0ce1, 0x34c2838f, 0xd571ec6c, 0x1366a993 // last one was: 0x1366a992
		//0xcbb9ac40, ...
		// (the last word 0x1366a992 was rounded up because the next one is 0xcbb9ac40 -- first bit is one 0xc..)
		// 256 32bit words for the mantissa -- about 2464 valid decimal digits
		};	

		// above value was calculated using Big<1,400> type on a 32bit platform
		// and then the first 256 words were taken,
		// the calculating was made by using LnSurrounding1(2) method
		// which took 4035 iterations
		// (the result was compared with ln(2) taken from http://ja0hxv.calico.jp/pai/estart.html)
		// (TTMATH_BUILTIN_VARIABLES_SIZE on 32bit platform should have the value 256,
		// and on 64bit platform value 128 (256/2=128))

		mantissa.SetFromTable(temp_table, sizeof(temp_table) / sizeof(int));
		exponent = -sint(man)*sint(TTMATH_BITS_PER_UINT);
		info = 0;
	}


	/*!
		this method sets the value of ln(10)
		the natural logarithm from 10

		I introduced this constant especially to make the conversion ToString()
		being faster. In fact the method ToString() is keeping values of logarithms
		it has calculated but it must calculate the logarithm at least once.
		If a program, which uses this library, is running for a long time this
		would be ok, but for programs which are running shorter, for example for
		CGI applications which only once are printing values, this would be much
		inconvenience. Then if we're printing with base (radix) 10 and the mantissa
		of our value is smaller than or equal to TTMATH_BUILTIN_VARIABLES_SIZE
		we don't calculate the logarithm but take it from this constant.
	*/
	void SetLn10()
	{
	static const unsigned int temp_table[] = {
		0x935d8ddd, 0xaaa8ac16, 0xea56d62b, 0x82d30a28, 0xe28fecf9, 0xda5df90e, 0x83c61e82, 0x01f02d72, 
		0x962f02d7, 0xb1a8105c, 0xcc70cbc0, 0x2c5f0d68, 0x2c622418, 0x410be2da, 0xfb8f7884, 0x02e516d6, 
		0x782cf8a2, 0x8a8c911e, 0x765aa6c3, 0xb0d831fb, 0xef66ceb0, 0x4ab3c6fa, 0x5161bb49, 0xd219c7bb, 
		0xca67b35b, 0x23605085, 0x8e93368d, 0x44789c4f, 0x5b08b057, 0xd5ede20f, 0x469ea58e, 0x9305e981, 
		0xe2478fca, 0xad3aee98, 0x9cd5b42e, 0x6a271619, 0xa47ecb26, 0x978c5d4f, 0xdb1d28ea, 0x57d4fdc0, 
		0xe40bf3cc, 0x1e14126a, 0x45765cde, 0x268339db, 0xf47fa96d, 0xeb271060, 0xaf88486e, 0xa9b7401e, 
		0x3dfd3c51, 0x748e6d6e, 0x3848c8d2, 0x5faf1bca, 0xe88047f1, 0x7b0d9b50, 0xa949eaaa, 0xdf69e8a5, 
		0xf77e3760, 0x4e943960, 0xe38a5700, 0xffde2db1, 0xad6bfbff, 0xd821ba0a, 0x4cb0466d, 0x61ba648e, 
		0xef99c8e5, 0xf6974f36, 0x3982a78c, 0xa45ddfc8, 0x09426178, 0x19127a6e, 0x3b70fcda, 0x2d732d47, 
		0xb5e4b1c8, 0xc0e5a10a, 0xaa6604a5, 0x324ec3dc, 0xbc64ea80, 0x6e198566, 0x1f1d366c, 0x20663834, 
		0x4d5e843f, 0x20642b97, 0x0a62d18e, 0x478f7bd5, 0x8fcd0832, 0x4a7b32a6, 0xdef85a05, 0xeb56323a, 
		0x421ef5e0, 0xb00410a0, 0xa0d9c260, 0x794a976f, 0xf6ff363d, 0xb00b6b33, 0xf42c58de, 0xf8a3c52d, 
		0xed69b13d, 0xc1a03730, 0xb6524dc1, 0x8c167e86, 0x99d6d20e, 0xa2defd2b, 0xd006f8b4, 0xbe145a2a, 
		0xdf3ccbb3, 0x189da49d, 0xbc1261c8, 0xb3e4daad, 0x6a36cecc, 0xb2d5ae5b, 0x89bf752f, 0xb5dfb353, 
		0xff3065c4, 0x0cfceec8, 0x1be5a9a9, 0x67fddc57, 0xc4b83301, 0x006bf062, 0x4b40ed7a, 0x56c6cdcd, 
		0xa2d6fe91, 0x388e9e3e, 0x48a93f5f, 0x5e3b6eb4, 0xb81c4a5b, 0x53d49ea6, 0x8e668aea, 0xba83c7f8, 
		0xfb5f06c3, 0x58ac8f70, 0xfa9d8c59, 0x8c574502, 0xbaf54c96, 0xc84911f0, 0x0482d095, 0x1a0af022, 
		0xabbab080, 0xec97efd3, 0x671e4e0e, 0x52f166b6, 0xcd5cd226, 0x0dc67795, 0x2e1e34a3, 0xf799677f, 
		0x2c1d48f1, 0x2944b6c5, 0x2ba1307e, 0x704d67f9, 0x1c1035e4, 0x4e927c63, 0x03cf12bf, 0xe2cd2e31, 
		0xf8ee4843, 0x344d51b0, 0xf37da42b, 0x9f0b0fd9, 0x134fb2d9, 0xf815e490, 0xd966283f, 0x23962766, 
		0xeceab1e4, 0xf3b5fc86, 0x468127e2, 0xb606d10d, 0x3a45f4b6, 0xb776102d, 0x2fdbb420, 0x80c8fa84, 
		0xd0ff9f45, 0xc58aef38, 0xdb2410fd, 0x1f1cebad, 0x733b2281, 0x52ca5f36, 0xddf29daa, 0x544334b8, 
		0xdeeaf659, 0x4e462713, 0x1ed485b4, 0x6a0822e1, 0x28db471c, 0xa53938a8, 0x44c3bef7, 0xf35215c8, 
		0xb382bc4e, 0x3e4c6f15, 0x6285f54c, 0x17ab408e, 0xccbf7f5e, 0xd16ab3f6, 0xced2846d, 0xf457e14f, 
		0xbb45d9c5, 0x646ad497, 0xac697494, 0x145de32e, 0x93907128, 0xd263d521, 0x79efb424, 0xd64651d6, 
		0xebc0c9f0, 0xbb583a44, 0xc6412c84, 0x85bb29a6, 0x4d31a2cd, 0x92954469, 0xa32b1abd, 0xf7f5202c, 
		0xa4aa6c93, 0x2e9b53cf, 0x385ab136, 0x2741f356, 0x5de9c065, 0x6009901c, 0x88abbdd8, 0x74efcf73, 
		0x3f761ad4, 0x35f3c083, 0xfd6b8ee0, 0x0bef11c7, 0xc552a89d, 0x58ce4a21, 0xd71e54f2, 0x4157f6c7, 
		0xd4622316, 0xe98956d7, 0x450027de, 0xcbd398d8, 0x4b98b36a, 0x0724c25c, 0xdb237760, 0xe9324b68, 
		0x7523e506, 0x8edad933, 0x92197f00, 0xb853a326, 0xb330c444, 0x65129296, 0x34bc0670, 0xe177806d, 
		0xe338dac4, 0x5537492a, 0xe19add83, 0xcf45000f, 0x5b423bce, 0x6497d209, 0xe30e18a1, 0x3cbf0687, 
		0x67973103, 0xd9485366, 0x81506bba, 0x2e93a9a4, 0x7dd59d3f, 0xf17cd746, 0x8c2075be, 0x552a4348 // last one was: 0x552a4347
		// 0xb4a638ef, ...
		//(the last word 0x552a4347 was rounded up because the next one is 0xb4a638ef -- first bit is one 0xb..)
		// 256 32bit words for the mantissa -- about 2464 valid digits (decimal)
		};	

		// above value was calculated using Big<1,400> type on a 32bit platform
		// and then the first 256 32bit words were taken,
		// the calculating was made by using LnSurrounding1(10) method
		// which took 22080 iterations
		// (the result was compared with ln(10) taken from http://ja0hxv.calico.jp/pai/estart.html)
		// (the formula used in LnSurrounding1(x) converges badly when
	    // the x is greater than one but in fact we can use it, only the
		// number of iterations will be greater)
		// (TTMATH_BUILTIN_VARIABLES_SIZE on 32bit platform should have the value 256,
		// and on 64bit platform value 128 (256/2=128))

		mantissa.SetFromTable(temp_table, sizeof(temp_table) / sizeof(int));
		exponent = -sint(man)*sint(TTMATH_BITS_PER_UINT) + 2;
		info = 0;
	}


	/*!
		this method sets the maximum value which can be held in this type
	*/
	void SetMax()
	{
		info = 0;
		mantissa.SetMax();
		exponent.SetMax();

		// we don't have to use 'Standardizing()' because the last bit from
		// the mantissa is set
	}


	/*!
		this method sets the minimum value which can be held in this type
	*/
	void SetMin()
	{
		info = 0;

		mantissa.SetMax();
		exponent.SetMax();
		SetSign();

		// we don't have to use 'Standardizing()' because the last bit from
		// the mantissa is set
	}


	/*!
		testing whether there is a value zero or not
	*/
	bool IsZero() const
	{
		return IsInfoBit(TTMATH_BIG_ZERO);
	}


	/*!
		this method returns true when there's the sign set
		also we don't check the NaN flag
	*/
	bool IsSign() const
	{
		return IsInfoBit(TTMATH_BIG_SIGN);
	}


	/*!
		this method returns true when there is not a valid number
	*/
	bool IsNan() const
	{
		return IsInfoBit(TTMATH_BIG_NAN);
	}



	/*!
		this method clears the sign
		(there'll be an absolute value)

			e.g.
			-1 -> 1
			2  -> 2
	*/
	void Abs()
	{
		ClearInfoBit(TTMATH_BIG_SIGN);
	}


	/*!
		this method remains the 'sign' of the value
		e.g.  -2 = -1 
		       0 = 0
		      10 = 1
	*/
	void Sgn()
	{
		// we have to check the NaN flag, because the next SetOne() method would clear it
		if( IsNan() )
			return;

		if( IsSign() )
		{
			SetOne();
			SetSign();
		}
		else
		if( IsZero() )
			SetZero(); // !! is nedeed here?
		else
			SetOne();
	}



	/*!
		this method sets the sign

			e.g.
			-1 -> -1
			2  -> -2

		we do not check whether there is a zero or not, if you're using this method
		you must be sure that the value is (or will be afterwards) different from zero
	*/
	void SetSign()
	{
		SetInfoBit(TTMATH_BIG_SIGN);
	}


	/*!
		this method changes the sign
		when there is a value of zero then the sign is not changed

			e.g.
			-1 -> 1
			2  -> -2
	*/
	void ChangeSign()
	{
		// we don't have to check the NaN flag here

		if( IsZero() )
			return;

		if( IsSign() )
			ClearInfoBit(TTMATH_BIG_SIGN);
		else
			SetInfoBit(TTMATH_BIG_SIGN);
	}



private:

	/*!
		this method does the half-to-even rounding (banker's rounding)

		if is_half is:
		  true  - that means the rest was equal the half (0.5 decimal)
		  false - that means the rest was greater than a half (greater than 0.5 decimal)

	    if the rest was less than a half then don't call this method
		(the rounding should does nothing then)
	*/
	uint RoundHalfToEven(bool is_half, bool rounding_up = true)
	{
	uint c = 0;

		if( !is_half || mantissa.IsTheLowestBitSet() )
		{
			if( rounding_up )
			{
				if( mantissa.AddOne() )
				{
					mantissa.Rcr(1, 1);
					c = exponent.AddOne();
				}
			}
			else
			{
				#ifdef TTMATH_DEBUG
				uint c_from_zero =
				#endif
				mantissa.SubOne();

				// we're using rounding_up=false in Add() when the mantissas have different signs
				// mantissa can be zero only when previous mantissa was equal to ss2.mantissa
				// but in such a case 'last_bit_set' will not be set and consequently 'do_rounding' will be false
				TTMATH_ASSERT( c_from_zero == 0 )
			}
		}

	return c;
	}





	/*!
	*
	*	basic mathematic functions
	*
	*/


	/*!
		this method adds one to the existing value
	*/
	uint AddOne()
	{
	Big<exp, man> one;

		one.SetOne();

	return Add(one);
	}


	/*!
		this method subtracts one from the existing value
	*/
	uint SubOne()
	{
	Big<exp, man> one;

		one.SetOne();

	return Sub(one);
	}


private:


	/*!
		an auxiliary method for adding
	*/
	void AddCheckExponents(	Big<exp, man> & ss2,
							Int<exp> & exp_offset,
							bool & last_bit_set,
							bool & rest_zero,
							bool & do_adding,
							bool & do_rounding)
	{
	Int<exp> mantissa_size_in_bits( man * TTMATH_BITS_PER_UINT );

		if( exp_offset == mantissa_size_in_bits )
		{
			last_bit_set = ss2.mantissa.IsTheHighestBitSet();
			rest_zero    = ss2.mantissa.AreFirstBitsZero(man*TTMATH_BITS_PER_UINT - 1);
			do_rounding  = true;	// we'are only rounding
		}
		else
		if( exp_offset < mantissa_size_in_bits )
		{
			uint moved = exp_offset.ToInt(); // how many times we must move ss2.mantissa
			rest_zero  = true;

			if( moved > 0 )
			{
				last_bit_set = static_cast<bool>( ss2.mantissa.GetBit(moved-1) );

				if( moved > 1 )
					rest_zero = ss2.mantissa.AreFirstBitsZero(moved - 1);
			
				// (2) moving 'exp_offset' times
				ss2.mantissa.Rcr(moved, 0);
			}

			do_adding    = true; 
			do_rounding  = true;
		}

		// if exp_offset is greater than mantissa_size_in_bits then we do nothing
		// ss2 is too small for taking into consideration in the sum
	}


	/*!
		an auxiliary method for adding
	*/
	uint AddMantissas(	Big<exp, man> & ss2,
						bool & last_bit_set,
						bool & rest_zero)
	{
	uint c = 0;

		if( IsSign() == ss2.IsSign() )
		{
			// values have the same signs
			if( mantissa.Add(ss2.mantissa) )
			{
				// we have one bit more from addition (carry)
				// now rest_zero means the old rest_zero with the old last_bit_set
				rest_zero    = (!last_bit_set && rest_zero);
				last_bit_set = mantissa.Rcr(1,1);
				c += exponent.AddOne();
			}
		}
		else
		{
			// values have different signs
			// there shouldn't be a carry here because
			// (1) (2) guarantee that the mantissa of this
			// is greater than or equal to the mantissa of the ss2

			#ifdef TTMATH_DEBUG
			uint c_temp =
			#endif
			mantissa.Sub(ss2.mantissa);

			TTMATH_ASSERT( c_temp == 0 )
		}

	return c;
	}


public:


	/*!
		Addition this = this + ss2

		it returns carry if the sum is too big
	*/
	uint Add(Big<exp, man> ss2, bool round = true, bool adding = true)
	{
	bool last_bit_set, rest_zero, do_adding, do_rounding, rounding_up;
	Int<exp> exp_offset( exponent );
	uint c = 0;

		if( IsNan() || ss2.IsNan() )
			return CheckCarry(1);

		if( !adding )
			ss2.ChangeSign(); // subtracting

		exp_offset.Sub( ss2.exponent );
		exp_offset.Abs();

		// (1) abs(this) will be >= abs(ss2)
		if( SmallerWithoutSignThan(ss2) )
			Swap(ss2);
	
		if( ss2.IsZero() )
			return 0;

		last_bit_set = rest_zero = do_adding = do_rounding = false;
		rounding_up = (IsSign() == ss2.IsSign());

		AddCheckExponents(ss2, exp_offset, last_bit_set, rest_zero, do_adding, do_rounding);

		if( do_adding )
			c += AddMantissas(ss2, last_bit_set, rest_zero);

		if( !round || !last_bit_set )
			do_rounding = false;

		if( do_rounding )
			c += RoundHalfToEven(rest_zero, rounding_up);

		if( do_adding || do_rounding )
			c += Standardizing();

	return CheckCarry(c);
	}


	/*!
		Subtraction this = this - ss2

		it returns carry if the result is too big
	*/
	uint Sub(const Big<exp, man> & ss2, bool round = true)
	{
		return Add(ss2, round, false);
	}
		

	/*!
		bitwise AND

		this and ss2 must be >= 0
		return values:
			0 - ok
			1 - carry
			2 - this or ss2 was negative
	*/
	uint BitAnd(Big<exp, man> ss2)
	{
		if( IsNan() || ss2.IsNan() )
			return CheckCarry(1);

		if( IsSign() || ss2.IsSign() )
		{
			SetNan();
			return 2;
		}

		if( IsZero() )
			return 0;

		if( ss2.IsZero() )
		{
			SetZero();
			return 0;
		}

		Int<exp> exp_offset( exponent );
		Int<exp> mantissa_size_in_bits( man * TTMATH_BITS_PER_UINT );

		uint c = 0;

		exp_offset.Sub( ss2.exponent );
		exp_offset.Abs();

		// abs(this) will be >= abs(ss2)
		if( SmallerWithoutSignThan(ss2) )
			Swap(ss2);

		if( exp_offset >= mantissa_size_in_bits )
		{
			// the second value is too small
			SetZero();
			return 0;
		}

		// exp_offset < mantissa_size_in_bits, moving 'exp_offset' times
		ss2.mantissa.Rcr( exp_offset.ToInt(), 0 );
		mantissa.BitAnd(ss2.mantissa);

		c += Standardizing();

	return CheckCarry(c);
	}


	/*!
		bitwise OR

		this and ss2 must be >= 0
		return values:
			0 - ok
			1 - carry
			2 - this or ss2 was negative
	*/
	uint BitOr(Big<exp, man> ss2)
	{
		if( IsNan() || ss2.IsNan() )
			return CheckCarry(1);

		if( IsSign() || ss2.IsSign() )
		{
			SetNan();
			return 2;
		}
		
		if( IsZero() )
		{
			*this = ss2;
			return 0;
		}

		if( ss2.IsZero() )
			return 0;

		Int<exp> exp_offset( exponent );
		Int<exp> mantissa_size_in_bits( man * TTMATH_BITS_PER_UINT );

		uint c = 0;

		exp_offset.Sub( ss2.exponent );
		exp_offset.Abs();

		// abs(this) will be >= abs(ss2)
		if( SmallerWithoutSignThan(ss2) )
			Swap(ss2);

		if( exp_offset >= mantissa_size_in_bits )
			// the second value is too small
			return 0;

		// exp_offset < mantissa_size_in_bits, moving 'exp_offset' times
		ss2.mantissa.Rcr( exp_offset.ToInt(), 0 );
		mantissa.BitOr(ss2.mantissa);

		c += Standardizing();

	return CheckCarry(c);
	}


	/*!
		bitwise XOR

		this and ss2 must be >= 0
		return values:
			0 - ok
			1 - carry
			2 - this or ss2 was negative
	*/
	uint BitXor(Big<exp, man> ss2)
	{
		if( IsNan() || ss2.IsNan() )
			return CheckCarry(1);

		if( IsSign() || ss2.IsSign() )
		{
			SetNan();
			return 2;
		}
		
		if( ss2.IsZero() )
			return 0;

		if( IsZero() )
		{
			*this = ss2;
			return 0;
		}

		Int<exp> exp_offset( exponent );
		Int<exp> mantissa_size_in_bits( man * TTMATH_BITS_PER_UINT );

		uint c = 0;

		exp_offset.Sub( ss2.exponent );
		exp_offset.Abs();

		// abs(this) will be >= abs(ss2)
		if( SmallerWithoutSignThan(ss2) )
			Swap(ss2);

		if( exp_offset >= mantissa_size_in_bits )
			// the second value is too small
			return 0;

		// exp_offset < mantissa_size_in_bits, moving 'exp_offset' times
		ss2.mantissa.Rcr( exp_offset.ToInt(), 0 );
		mantissa.BitXor(ss2.mantissa);

		c += Standardizing();

	return CheckCarry(c);
	}



	/*!
		Multiplication this = this * ss2 (ss2 is uint)

		ss2 without a sign
	*/
	uint MulUInt(uint ss2)
	{
	UInt<man+1> man_result;
	uint i,c = 0;

		if( IsNan() )
			return 1;

		if( IsZero() )
			return 0;

		if( ss2 == 0 )
		{
			SetZero();
			return 0;
		}

		// man_result = mantissa * ss2.mantissa
		mantissa.MulInt(ss2, man_result);

		sint bit = UInt<man>::FindLeadingBitInWord(man_result.table[man]); // man - last word
		
		if( bit!=-1 && uint(bit) > (TTMATH_BITS_PER_UINT/2) )
		{
			// 'i' will be from 0 to TTMATH_BITS_PER_UINT
			i = man_result.CompensationToLeft();
			c = exponent.Add( TTMATH_BITS_PER_UINT - i );

			for(i=0 ; i<man ; ++i)
				mantissa.table[i] = man_result.table[i+1];
		}
		else
		{
			if( bit != -1 )
			{
				man_result.Rcr(bit+1, 0);
				c += exponent.Add(bit+1);
			}

			for(i=0 ; i<man ; ++i)
				mantissa.table[i] = man_result.table[i];
		}

		c += Standardizing();

	return CheckCarry(c);
	}


	/*!
		Multiplication this = this * ss2 (ss2 is sint)

		ss2 with a sign
	*/
	uint MulInt(sint ss2)
	{
		if( IsNan() )
			return 1;

		if( ss2 == 0 )
		{
			SetZero();
			return 0;
		}

		if( IsZero() )
			return 0;

		if( IsSign() == (ss2<0) )
		{
			// the signs are the same (both are either - or +), the result is positive
			Abs();
		}
		else
		{
			// the signs are different, the result is negative
			SetSign();
		}

		if( ss2<0 )
			ss2 = -ss2;


	return MulUInt( uint(ss2) );
	}


private:


	/*!
		this method checks whether a table pointed by 'tab' and 'len'
		has the value 0.5 decimal
		(it is treated as the comma operator would be before the highest bit)
		call this method only if the highest bit is set - you have to test it beforehand

		return:
		  true  - tab was equal the half (0.5 decimal)
		  false - tab was greater than a half (greater than 0.5 decimal)

	*/
	bool CheckGreaterOrEqualHalf(uint * tab, uint len)
	{
	uint i;

		TTMATH_ASSERT( len>0 && (tab[len-1] & TTMATH_UINT_HIGHEST_BIT)!=0 )

		for(i=0 ; i<len-1 ; ++i)
			if( tab[i] != 0 )
				return false;

		if( tab[i] != TTMATH_UINT_HIGHEST_BIT )
			return false;

	return true;
	}


private:

	/*!
		multiplication this = this * ss2
		this method returns a carry
	*/
	uint MulRef(const Big<exp, man> & ss2, bool round = true)
	{
	TTMATH_REFERENCE_ASSERT( ss2 )

	UInt<man*2> man_result;
	uint c = 0;
	uint i;

		if( IsNan() || ss2.IsNan() )
			return CheckCarry(1);

		if( IsZero() )
			return 0;

		if( ss2.IsZero() )
		{
			SetZero();
			return 0;
		}

		// man_result = mantissa * ss2.mantissa
		mantissa.MulBig(ss2.mantissa, man_result);

		// 'i' will be from 0 to man*TTMATH_BITS_PER_UINT
		// because mantissa and ss2.mantissa are standardized 
		// (the highest bit in man_result is set to 1 or
		// if there is a zero value in man_result the method CompensationToLeft()
		// returns 0 but we'll correct this at the end in Standardizing() method)
		i = man_result.CompensationToLeft();
		uint exp_add = man * TTMATH_BITS_PER_UINT - i;

		if( exp_add )
			c += exponent.Add( exp_add );

		c += exponent.Add( ss2.exponent );

		for(i=0 ; i<man ; ++i)
			mantissa.table[i] = man_result.table[i+man];

		if( round && (man_result.table[man-1] & TTMATH_UINT_HIGHEST_BIT) != 0 )
		{
			bool is_half = CheckGreaterOrEqualHalf(man_result.table, man);
			c += RoundHalfToEven(is_half);		
		}

		if( IsSign() == ss2.IsSign() )
		{
			// the signs are the same, the result is positive
			Abs();
		}
		else
		{
			// the signs are different, the result is negative
			// if the value is zero it will be corrected later in Standardizing method
			SetSign();
		}

		c += Standardizing();

	return CheckCarry(c);
	}
	

public:


	/*!
		multiplication this = this * ss2
		this method returns a carry
	*/
	uint Mul(const Big<exp, man> & ss2, bool round = true)
	{
		if( this == &ss2 )
		{
			Big<exp, man> copy_ss2(ss2);
			return MulRef(copy_ss2, round);
		}
		else
		{
			return MulRef(ss2, round);
		}
	}


private:

	/*!
		division this = this / ss2

		return value:
		0 - ok
		1 - carry (in a division carry can be as well)
		2 - improper argument (ss2 is zero)
	*/
	uint DivRef(const Big<exp, man> & ss2, bool round = true)
	{
	TTMATH_REFERENCE_ASSERT( ss2 )

	UInt<man*2> man1;
	UInt<man*2> man2;
	uint i,c = 0;
		
		if( IsNan() || ss2.IsNan() )
			return CheckCarry(1);

		if( ss2.IsZero() )
		{
			SetNan();
			return 2;
		}

		if( IsZero() )
			return 0;

		// !! this two loops can be joined together

		for(i=0 ; i<man ; ++i)
		{
			man1.table[i+man] = mantissa.table[i];
			man2.table[i]     = ss2.mantissa.table[i];
		}

		for(i=0 ; i<man ; ++i)
		{
			man1.table[i] = 0;
			man2.table[i+man] = 0;
		}

		man1.Div(man2);

		i = man1.CompensationToLeft();

		if( i )
			c += exponent.Sub(i);

		c += exponent.Sub(ss2.exponent);
		
		for(i=0 ; i<man ; ++i)
			mantissa.table[i] = man1.table[i+man];

		if( round && (man1.table[man-1] & TTMATH_UINT_HIGHEST_BIT) != 0 )
		{
			bool is_half = CheckGreaterOrEqualHalf(man1.table, man);
			c += RoundHalfToEven(is_half);
		}

		if( IsSign() == ss2.IsSign() )
			Abs();
		else
			SetSign(); // if there is a zero it will be corrected in Standardizing()

		c += Standardizing();

	return CheckCarry(c);
	}


public:

	/*!
		division this = this / ss2

		return value:
		0 - ok
		1 - carry (in a division carry can be as well)
		2 - improper argument (ss2 is zero)
	*/
	uint Div(const Big<exp, man> & ss2, bool round = true)
	{
		if( this == &ss2 )
		{
			Big<exp, man> copy_ss2(ss2);
			return DivRef(copy_ss2, round);
		}
		else
		{
			return DivRef(ss2, round);
		}
	}


private:

	/*!
		the remainder from a division
	*/
	uint ModRef(const Big<exp, man> & ss2)
	{
	TTMATH_REFERENCE_ASSERT( ss2 )

	uint c = 0;

		if( IsNan() || ss2.IsNan() )
			return CheckCarry(1);

		if( ss2.IsZero() )
		{
			SetNan();
			return 2;
		}

		if( !SmallerWithoutSignThan(ss2) )
		{
			Big<exp, man> temp(*this);

			c = temp.Div(ss2);
			temp.SkipFraction();
			c += temp.Mul(ss2);
			c += Sub(temp);

			if( !SmallerWithoutSignThan( ss2 ) )
				c += 1;
		}

	return CheckCarry(c);
	}


public:

	/*!
		the remainder from a division

		e.g.
		 12.6 mod  3 =  0.6   because  12.6 = 3*4 + 0.6
		-12.6 mod  3 = -0.6   bacause -12.6 = 3*(-4) + (-0.6)
		 12.6 mod -3 =  0.6
		-12.6 mod -3 = -0.6

		it means:
		in other words: this(old) = ss2 * q + this(new)

		return value:
		0 - ok
		1 - carry
		2 - improper argument (ss2 is zero)
	*/
	uint Mod(const Big<exp, man> & ss2)
	{
		if( this == &ss2 )
		{
			Big<exp, man> copy_ss2(ss2);
			return ModRef(copy_ss2);
		}
		else
		{
			return ModRef(ss2);
		}
	}


	/*!
		this method returns: 'this' mod 2
		(either zero or one)

		this method is much faster than using Mod( object_with_value_two )
	*/
	uint Mod2() const
	{
		if( exponent>sint(0) || exponent<=-sint(man*TTMATH_BITS_PER_UINT) )
			return 0;

		sint exp_int = exponent.ToInt();
		// 'exp_int' is negative (or zero), we set it as positive
		exp_int = -exp_int;

	return mantissa.GetBit(exp_int);
	}


	/*!
		power this = this ^ pow
		(pow without a sign)

		binary algorithm (r-to-l)

		return values:
		0 - ok
		1 - carry
		2 - incorrect arguments (0^0)
	*/
	template<uint pow_size>
	uint Pow(UInt<pow_size> pow)
	{
		if( IsNan() )
			return 1;

		if( IsZero() )
		{
			if( pow.IsZero() )
			{
				// we don't define zero^zero
				SetNan();
				return 2;
			}

			// 0^(+something) is zero
			return 0;
		}

		Big<exp, man> start(*this);
		Big<exp, man> result;
		result.SetOne();
		uint c = 0;

		while( !c )
		{
			if( pow.table[0] & 1 )
				c += result.Mul(start);

			pow.Rcr(1);

			if( pow.IsZero() )
				break;

			c += start.Mul(start);
		}

		*this = result;

	return CheckCarry(c);
	}


	/*!
		power this = this ^ pow
		p can be negative

		return values:
		0 - ok
		1 - carry
		2 - incorrect arguments 0^0 or 0^(-something)
	*/
	template<uint pow_size>
	uint Pow(Int<pow_size> pow)
	{
		if( IsNan() )
			return 1;

		if( !pow.IsSign() )
			return Pow( UInt<pow_size>(pow) );

		if( IsZero() )
		{
			// if 'p' is negative then
			// 'this' must be different from zero
			SetNan();
			return 2;
		}

		uint c = pow.ChangeSign();

		Big<exp, man> t(*this);
		c += t.Pow( UInt<pow_size>(pow) ); // here can only be a carry (return:1)

		SetOne();
		c += Div(t);

	return CheckCarry(c);
	}


	/*!
		power this = this ^ abs([pow])
		pow is treated as a value without a sign and without a fraction
		 if pow has a sign then the method pow.Abs() is used
		 if pow has a fraction the fraction is skipped (not used in calculation)

		return values:
		0 - ok
		1 - carry
		2 - incorrect arguments (0^0)
	*/
	uint PowUInt(Big<exp, man> pow)
	{
		if( IsNan() || pow.IsNan() )
			return CheckCarry(1);

		if( IsZero() )
		{
			if( pow.IsZero() )
			{
				SetNan();
				return 2;
			}

			// 0^(+something) is zero
			return 0;
		}

		if( pow.IsSign() )
			pow.Abs();

		Big<exp, man> start(*this);
		Big<exp, man> result;
		Big<exp, man> one;
		uint c = 0;
		one.SetOne();
		result = one;

		while( !c )
		{
			if( pow.Mod2() )
				c += result.Mul(start);

			c += pow.exponent.SubOne();

			if( pow < one )
				break;

			c += start.Mul(start);
		}

		*this = result;

	return CheckCarry(c);
	}


	/*!
		power this = this ^ [pow]
		pow is treated as a value without a fraction
		pow can be negative

		return values:
		0 - ok
		1 - carry
		2 - incorrect arguments 0^0 or 0^(-something)
	*/
	uint PowInt(const Big<exp, man> & pow)
	{
		if( IsNan() || pow.IsNan() )
			return CheckCarry(1);

		if( !pow.IsSign() )
			return PowUInt(pow);

		if( IsZero() )
		{
			// if 'pow' is negative then
			// 'this' must be different from zero
			SetNan();
			return 2;
		}

		Big<exp, man> temp(*this);
		uint c = temp.PowUInt(pow); // here can only be a carry (result:1)

		SetOne();
		c += Div(temp);

	return CheckCarry(c);
	}


	/*!
		power this = this ^ pow
		this must be greater than zero (this > 0)
		pow can be negative and with fraction

		return values:
		0 - ok
		1 - carry
		2 - incorrect argument ('this' <= 0)
	*/
	uint PowFrac(const Big<exp, man> & pow)
	{
		if( IsNan() || pow.IsNan() )
			return CheckCarry(1);

		Big<exp, man> temp;
		uint c = temp.Ln(*this);

		if( c != 0 ) // can be 2 from Ln()
		{
			SetNan();
			return c;
		}

		c += temp.Mul(pow);
		c += Exp(temp);

	return CheckCarry(c);
	}


	/*!
		power this = this ^ pow
		pow can be negative and with fraction

		return values:
		0 - ok
		1 - carry
		2 - incorrect argument ('this' or 'pow')
	*/
	uint Pow(const Big<exp, man> & pow)
	{
		if( IsNan() || pow.IsNan() )
			return CheckCarry(1);

		if( IsZero() )
		{
			// 0^pow will be 0 only for pow>0
			if( pow.IsSign() || pow.IsZero() )
			{
				SetNan();
				return 2;
			}

			SetZero();

		return 0;
		}

		if( pow.exponent>-sint(man*TTMATH_BITS_PER_UINT) && pow.exponent<=0 )
		{
			if( pow.IsInteger() )
				return PowInt( pow );
		}

	return PowFrac(pow);
	}


	/*!
		this function calculates the square root
		e.g. let this=9 then this.Sqrt() gives 3

		return: 0 - ok
				1 - carry
		        2 - improper argument (this<0 or NaN)
	*/
	uint Sqrt()
	{
		if( IsNan() || IsSign() )
		{
			SetNan();
			return 2;
		}

		if( IsZero() )
			return 0;

		Big<exp, man> old(*this);
		Big<exp, man> ln;
		uint c = 0;

		// we're using the formula: sqrt(x) = e ^ (ln(x) / 2)
		c += ln.Ln(*this);
		c += ln.exponent.SubOne(); // ln = ln / 2
		c += Exp(ln);

		// above formula doesn't give accurate results for some integers
		// e.g. Sqrt(81) would not be 9 but a value very closed to 9
		// we're rounding the result, calculating result*result and comparing
		// with the old value, if they are equal then the result is an integer too

		if( !c && old.IsInteger() && !IsInteger() )
		{
			Big<exp, man> temp(*this);
			c += temp.Round();

			Big<exp, man> temp2(temp);
			c += temp.Mul(temp2);

			if( temp == old )
				*this = temp2;
		}

	return CheckCarry(c);
	}


private:

#ifdef TTMATH_CONSTANTSGENERATOR
public:
#endif

	/*!
		Exponent this = exp(x) = e^x where x is in (-1,1)

		we're using the formula exp(x) = 1 + (x)/(1!) + (x^2)/(2!) + (x^3)/(3!) + ...
	*/
	void ExpSurrounding0(const Big<exp,man> & x, uint * steps = 0)
	{
		TTMATH_REFERENCE_ASSERT( x )

		Big<exp,man> denominator, denominator_i;
		Big<exp,man> one, old_value, next_part;
		Big<exp,man> numerator = x;
		
		SetOne();
		one.SetOne();
		denominator.SetOne();
		denominator_i.SetOne();

		uint i;
		old_value = *this;

		// we begin from 1 in order to not test at the beginning
	#ifdef TTMATH_CONSTANTSGENERATOR
		for(i=1 ; true ; ++i)
	#else
		for(i=1 ; i<=TTMATH_ARITHMETIC_MAX_LOOP ; ++i)
	#endif
		{
			bool testing = ((i & 3) == 0); // it means '(i % 4) == 0'

			next_part = numerator;

			if( next_part.Div( denominator ) )
				// if there is a carry here we only break the loop 
				// however the result we return as good
				// it means there are too many parts of the formula
				break;

			// there shouldn't be a carry here
			Add( next_part );

			if( testing )
			{
				if( old_value == *this )
					// we've added next few parts of the formula but the result
					// is still the same then we break the loop
					break;
				else
					old_value = *this;
			}

			// we set the denominator and the numerator for a next part of the formula
			if( denominator_i.Add(one) )
				// if there is a carry here the result we return as good
				break;

			if( denominator.Mul(denominator_i) )
				break;

			if( numerator.Mul(x) )
				break;
		}

		if( steps )
			*steps = i;
	}

public:


	/*!
		Exponent this = exp(x) = e^x

		we're using the fact that our value is stored in form of:
			x = mantissa * 2^exponent
		then
			e^x = e^(mantissa* 2^exponent) or
			e^x = (e^mantissa)^(2^exponent)

		'Exp' returns a carry if we can't count the result ('x' is too big)
	*/
	uint Exp(const Big<exp,man> & x)
	{
	uint c = 0;
		
		if( x.IsNan() )
			return CheckCarry(1);

		if( x.IsZero() )
		{
			SetOne();
		return 0;
		}

		// m will be the value of the mantissa in range (-1,1)
		Big<exp,man> m(x);
		m.exponent = -sint(man*TTMATH_BITS_PER_UINT);

		// 'e_' will be the value of '2^exponent'
		//   e_.mantissa.table[man-1] = TTMATH_UINT_HIGHEST_BIT;  and
		//   e_.exponent.Add(1) mean:
		//     e_.mantissa.table[0] = 1;
		//     e_.Standardizing();
		//     e_.exponent.Add(man*TTMATH_BITS_PER_UINT)
		//     (we must add 'man*TTMATH_BITS_PER_UINT' because we've taken it from the mantissa)
		Big<exp,man> e_(x);
		e_.mantissa.SetZero();
		e_.mantissa.table[man-1] = TTMATH_UINT_HIGHEST_BIT;
		c += e_.exponent.Add(1);
		e_.Abs();

		/*
			now we've got:
			m - the value of the mantissa in range (-1,1)
			e_ - 2^exponent

			e_ can be as:
			...2^-2, 2^-1, 2^0, 2^1, 2^2 ...
			...1/4 , 1/2 , 1  , 2  , 4   ...

			above one e_ is integer

			if e_ is greater than 1 we calculate the exponent as:
				e^(m * e_) = ExpSurrounding0(m) ^ e_
			and if e_ is smaller or equal one we calculate the exponent in this way:
				e^(m * e_) = ExpSurrounding0(m* e_)
			because if e_ is smaller or equal 1 then the product of m*e_ is smaller or equal m
		*/

		if( e_ <= 1 )
		{
			m.Mul(e_);
			ExpSurrounding0(m);
		}
		else
		{
			ExpSurrounding0(m);
			c += PowUInt(e_);
		}
	
	return CheckCarry(c);
	}




private:

#ifdef TTMATH_CONSTANTSGENERATOR
public:
#endif

	/*!
		Natural logarithm this = ln(x) where x in range <1,2)

		we're using the formula:
		ln x = 2 * [ (x-1)/(x+1) + (1/3)((x-1)/(x+1))^3 + (1/5)((x-1)/(x+1))^5 + ... ]
	*/
	void LnSurrounding1(const Big<exp,man> & x, uint * steps = 0)
	{
		Big<exp,man> old_value, next_part, denominator, one, two, x1(x), x2(x);

		one.SetOne();

		if( x == one )
		{
			// LnSurrounding1(1) is 0
			SetZero();
			return;
		}

		two = 2;

		x1.Sub(one);
		x2.Add(one);

		x1.Div(x2);
		x2 = x1;
		x2.Mul(x1);

		denominator.SetOne();
		SetZero();

		old_value = *this;
		uint i;


	#ifdef TTMATH_CONSTANTSGENERATOR
		for(i=1 ; true ; ++i)
	#else
		// we begin from 1 in order to not test at the beginning
		for(i=1 ; i<=TTMATH_ARITHMETIC_MAX_LOOP ; ++i)
	#endif
		{
			bool testing = ((i & 3) == 0); // it means '(i % 4) == 0'

			next_part = x1;

			if( next_part.Div(denominator) )
				// if there is a carry here we only break the loop 
				// however the result we return as good
				// it means there are too many parts of the formula
				break;

			// there shouldn't be a carry here
			Add(next_part);

			if( testing )
			{
				if( old_value == *this )
					// we've added next (step_test) parts of the formula but the result
					// is still the same then we break the loop
					break;
				else
					old_value = *this;
			}

			if( x1.Mul(x2) )
				// if there is a carry here the result we return as good
				break;

			if( denominator.Add(two) )
				break;
		}

		// this = this * 2
		// ( there can't be a carry here because we calculate the logarithm between <1,2) )
		exponent.AddOne();

		if( steps )
			*steps = i;
	}




public:


	/*!
		Natural logarithm this = ln(x)
		(a logarithm with the base equal 'e')

		we're using the fact that our value is stored in form of:
			x = mantissa * 2^exponent
		then
			ln(x) = ln (mantissa * 2^exponent) = ln (mantissa) + (exponent * ln (2))

		the mantissa we'll show as a value from range <1,2) because the logarithm
		is decreasing too fast when 'x' is going to 0

		return values:
			0 - ok
			1 - overflow (carry)
			2 - incorrect argument (x<=0)
	*/
	uint Ln(const Big<exp,man> & x)
	{
		if( x.IsNan() )
			return CheckCarry(1);

		if( x.IsSign() || x.IsZero() )
		{
			SetNan();
			return 2;
		}

		Big<exp,man> exponent_temp;
		exponent_temp.FromInt( x.exponent );

		// m will be the value of the mantissa in range <1,2)
		Big<exp,man> m(x);
		m.exponent = -sint(man*TTMATH_BITS_PER_UINT - 1);

		// we must add 'man*TTMATH_BITS_PER_UINT-1' because we've taken it from the mantissa
		uint c = exponent_temp.Add(man*TTMATH_BITS_PER_UINT-1);

	    LnSurrounding1(m);

		Big<exp,man> ln2;
		ln2.SetLn2();
		c += exponent_temp.Mul(ln2);
		c += Add(exponent_temp);

	return CheckCarry(c);
	}


	/*!
		Logarithm from 'x' with a 'base'

		we're using the formula:
			Log(x) with 'base' = ln(x) / ln(base)

		return values:
			0 - ok
			1 - overflow
			2 - incorrect argument (x<=0)
			3 - incorrect base (a<=0 lub a=1)
	*/
	uint Log(const Big<exp,man> & x, const Big<exp,man> & base)
	{
		if( x.IsNan() || base.IsNan() )
			return CheckCarry(1);

		if( x.IsSign() || x.IsZero() )
		{
			SetNan();
			return 2;
		}

		Big<exp,man> denominator;;
		denominator.SetOne();

		if( base.IsSign() || base.IsZero() || base==denominator )
		{
			SetNan();
			return 3;
		}
		
		if( x == denominator ) // (this is: if x == 1)
		{
			// log(1) is 0
			SetZero();
			return 0;
		}

		// another error values we've tested at the beginning
		// there can only be a carry
		uint c = Ln(x);

		c += denominator.Ln(base);
		c += Div(denominator);

	return CheckCarry(c);
	}




	/*!
	*
	*	converting methods
	*
	*/


	/*!
		converting from another type of a Big object
	*/
	template<uint another_exp, uint another_man>
	uint FromBig(const Big<another_exp, another_man> & another)
	{
		info = another.info;

		if( IsNan() )
			return 1;

		if( exponent.FromInt(another.exponent) )
		{
			SetNan();
			return 1;
		}

		uint man_len_min = (man < another_man)? man : another_man;
		uint i;
		uint c = 0;

		for( i = 0 ; i<man_len_min ; ++i )
			mantissa.table[man-1-i] = another.mantissa.table[another_man-1-i];
	
		for( ; i<man ; ++i )
			mantissa.table[man-1-i] = 0;


		// MS Visual Express 2005 reports a warning (in the lines with 'uint man_diff = ...'):
		// warning C4307: '*' : integral constant overflow
		// but we're using 'if( man > another_man )' and 'if( man < another_man )' and there'll be no such situation here
		#ifdef _MSC_VER
		#pragma warning( disable: 4307 )
		#endif

		if( man > another_man )
		{
			uint man_diff = (man - another_man) * TTMATH_BITS_PER_UINT;
			c += exponent.SubInt(man_diff, 0);
		}
		else
		if( man < another_man )
		{
			uint man_diff = (another_man - man) * TTMATH_BITS_PER_UINT;
			c += exponent.AddInt(man_diff, 0);
		}
		
		#ifdef _MSC_VER
		#pragma warning( default: 4307 )
		#endif

		// mantissa doesn't have to be standardized (either the highest bit is set or all bits are equal zero)
		CorrectZero();

	return CheckCarry(c);
	}


private:

	/*!
		an auxiliary method for converting 'this' into 'result'
		if the value is too big this method returns a carry (1)
	*/
	uint ToUIntOrInt(uint & result) const
	{
		result = 0;

		if( IsZero() )
			return 0;

		sint maxbit = -sint(man*TTMATH_BITS_PER_UINT);

		if( exponent > maxbit + sint(TTMATH_BITS_PER_UINT) )
			// if exponent > (maxbit + sint(TTMATH_BITS_PER_UINT)) the value can't be passed
			// into the 'sint' type (it's too big)
			return 1;

		if( exponent <= maxbit )
			// our value is from the range of (-1,1) and we return zero
			return 0;

		// exponent is from a range of (maxbit, maxbit + sint(TTMATH_BITS_PER_UINT) >
		// and [maxbit + sint(TTMATH_BITS_PER_UINT] <= 0
		sint how_many_bits = exponent.ToInt();

		// how_many_bits is negative, we'll make it positive
		how_many_bits = -how_many_bits;
	
		result = (mantissa.table[man-1] >> (how_many_bits % TTMATH_BITS_PER_UINT));

	return 0;
	}


public:

	/*!
		this method converts 'this' into uint
	*/
	uint ToUInt() const
	{
	uint result;

		ToUInt(result);

	return result;
	}


	/*!
		this method converts 'this' into 'result'

		if the value is too big this method returns a carry (1)
	*/
	uint ToUInt(uint & result) const
	{
		if( ToUIntOrInt(result) )
			return 1;

		if( IsSign() )
			return 1;

	return 0;
	}


	/*!
		this method converts 'this' into sint
	*/
	sint ToInt() const
	{
	sint result;

		ToInt(result);

	return result;
	}


	/*!
		this method converts 'this' into 'result'

		if the value is too big this method returns a carry (1)
	*/
	uint ToInt(uint & result) const
	{
		return ToUInt(result);
	}


	/*!
		this method converts 'this' into 'result'

		if the value is too big this method returns a carry (1)
	*/
	uint ToInt(sint & result) const
	{
	uint result_uint;

		uint c = ToUIntOrInt(result_uint);
		result = sint(result_uint);

		if( c )
			return 1;

		uint mask = 0;

		if( IsSign() )
		{
			mask = TTMATH_UINT_MAX_VALUE;
			result = -result;
		}

	return ((result & TTMATH_UINT_HIGHEST_BIT) == (mask & TTMATH_UINT_HIGHEST_BIT)) ? 0 : 1;
	}


private:

	/*!
		an auxiliary method for converting 'this' into 'result'

		if the value is too big this method returns a carry (1)
	*/
	template<uint int_size>
	uint ToUIntOrInt(UInt<int_size> & result) const
	{
		result.SetZero();

		if( IsZero() )
			return 0;
		
		sint maxbit = -sint(man*TTMATH_BITS_PER_UINT);

		if( exponent > maxbit + sint(int_size*TTMATH_BITS_PER_UINT) )
			// if exponent > (maxbit + sint(int_size*TTMATH_BITS_PER_UINT)) the value can't be passed
			// into the 'UInt<int_size>' type (it's too big)
			return 1;

		if( exponent <= maxbit )
			// our value is from range (-1,1) and we return zero
			return 0;

		sint how_many_bits = exponent.ToInt();

		if( how_many_bits < 0 )
		{
			how_many_bits = -how_many_bits;
			uint index    = how_many_bits / TTMATH_BITS_PER_UINT;

			UInt<man> mantissa_temp(mantissa);
			mantissa_temp.Rcr( how_many_bits % TTMATH_BITS_PER_UINT, 0 );

			for(uint i=index, a=0 ; i<man ; ++i,++a)
				result.table[a] = mantissa_temp.table[i];
		}
		else
		{
			uint index = how_many_bits / TTMATH_BITS_PER_UINT;

			if( index + (man-1) < int_size )
			{
				// above 'if' is always true
				// this is only to get rid of a warning "warning: array subscript is above array bounds"
				// (from gcc)
				// we checked the condition there: "if( exponent > maxbit + sint(int_size*TTMATH_BITS_PER_UINT) )"
				// but gcc doesn't understand our types - exponent is Int<>

				for(uint i=0 ; i<man ; ++i)
					result.table[index+i] = mantissa.table[i];
			}

			result.Rcl( how_many_bits % TTMATH_BITS_PER_UINT, 0 );
		}

	return 0;
	}


public:

	/*!
		this method converts 'this' into 'result'

		if the value is too big this method returns a carry (1)
	*/
	template<uint int_size>
	uint ToUInt(UInt<int_size> & result) const
	{
		uint c = ToUIntOrInt(result);

		if( c )
			return 1;

		if( IsSign() )
			return 1;

	return 0;
	}


	/*!
		this method converts 'this' into 'result'

		if the value is too big this method returns a carry (1)
	*/
	template<uint int_size>
	uint ToInt(UInt<int_size> & result) const
	{
		return ToUInt(result);
	}


	/*!
		this method converts 'this' into 'result'

		if the value is too big this method returns a carry (1)
	*/
	template<uint int_size>
	uint ToInt(Int<int_size> & result) const
	{
		uint c = ToUIntOrInt(result);

		if( c )
			return 1;

		uint mask = 0;

		if( IsSign() )
		{
			result.ChangeSign();
			mask = TTMATH_UINT_MAX_VALUE;
		}

	return ((result.table[int_size-1] & TTMATH_UINT_HIGHEST_BIT) == (mask & TTMATH_UINT_HIGHEST_BIT))? 0 : 1;
	}


	/*!
		a method for converting 'uint' to this class
	*/
	uint FromUInt(uint value)
	{
		if( value == 0 )
		{
			SetZero();
			return 0;
		}

		info = 0;

		for(uint i=0 ; i<man-1 ; ++i)
			mantissa.table[i] = 0;

		mantissa.table[man-1] = value;
		exponent = -sint(man-1) * sint(TTMATH_BITS_PER_UINT);

		// there shouldn't be a carry because 'value' has the 'uint' type 
		Standardizing();

	return 0;
	}


	/*!
		a method for converting 'uint' to this class
	*/
	uint FromInt(uint value)
	{
		return FromUInt(value);
	}


	/*!
		a method for converting 'sint' to this class
	*/
	uint FromInt(sint value)
	{
	bool is_sign = false;

		if( value < 0 )
		{
			value   = -value;
			is_sign = true;
		}

		FromUInt(uint(value));

		if( is_sign )
			SetSign();

	return 0;
	}



	/*!
		this method converts from standard double into this class

		standard double means IEEE-754 floating point value with 64 bits
		it is as follows (from http://www.psc.edu/general/software/packages/ieee/ieee.html):

		The IEEE double precision floating point standard representation requires
		a 64 bit word, which may be represented as numbered from 0 to 63, left to
		right. The first bit is the sign bit, S, the next eleven bits are the
		exponent bits, 'E', and the final 52 bits are the fraction 'F':

		S EEEEEEEEEEE FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
		0 1        11 12                                                63

		The value V represented by the word may be determined as follows:

		* If E=2047 and F is nonzero, then V=NaN ("Not a number")
		* If E=2047 and F is zero and S is 1, then V=-Infinity
		* If E=2047 and F is zero and S is 0, then V=Infinity
		* If 0<E<2047 then V=(-1)**S * 2 ** (E-1023) * (1.F) where "1.F" is intended
		  to represent the binary number created by prefixing F with an implicit
		  leading 1 and a binary point.
		* If E=0 and F is nonzero, then V=(-1)**S * 2 ** (-1022) * (0.F) These are
		  "unnormalized" values.
		* If E=0 and F is zero and S is 1, then V=-0
		* If E=0 and F is zero and S is 0, then V=0 
	*/

#ifdef TTMATH_PLATFORM32

	uint FromDouble(double value)
	{
		// I am not sure what will be on a platform which has 
		// a different endianness... but we use this library only
		// on x86 and amd (intel) 64 bits (as there's a lot of assembler code)
		union 
		{
			double d;
			uint u[2]; // two 32bit words
		} temp;

		temp.d = value;

		sint e  = ( temp.u[1] & 0x7FF00000u) >> 20;
		uint m1 = ((temp.u[1] &    0xFFFFFu) << 11) | (temp.u[0] >> 21);
		uint m2 = temp.u[0] << 11;
		
		if( e == 2047 )
		{
			// If E=2047 and F is nonzero, then V=NaN ("Not a number")
			// If E=2047 and F is zero and S is 1, then V=-Infinity
			// If E=2047 and F is zero and S is 0, then V=Infinity

			// we do not support -Infinity and +Infinity
			// we assume that there is always NaN 

			SetNan();
		}
		else
		if( e > 0 )
		{
			// If 0<E<2047 then
			// V=(-1)**S * 2 ** (E-1023) * (1.F)
			// where "1.F" is intended to represent the binary number
			// created by prefixing F with an implicit leading 1 and a binary point.
			
			FromDouble_SetExpAndMan((temp.u[1] & 0x80000000u) != 0,
									e - 1023 - man*TTMATH_BITS_PER_UINT + 1, 0x80000000u,
									m1, m2);

			// we do not have to call Standardizing() here
			// because the mantissa will have the highest bit set
		}
		else
		{
			// e == 0

			if( m1 != 0 || m2 != 0 )
			{
				// If E=0 and F is nonzero,
				// then V=(-1)**S * 2 ** (-1022) * (0.F)
				// These are "unnormalized" values.

				UInt<2> m;
				m.table[1] = m1;
				m.table[0] = m2;
				uint moved = m.CompensationToLeft();

				FromDouble_SetExpAndMan((temp.u[1] & 0x80000000u) != 0,
										e - 1022 - man*TTMATH_BITS_PER_UINT + 1 - moved, 0,
										m.table[1], m.table[2]);
			}
			else
			{
				// If E=0 and F is zero and S is 1, then V=-0
				// If E=0 and F is zero and S is 0, then V=0 

				// we do not support -0 or 0, only is one 0
				SetZero();
			}
		}

	return 0; // never be a carry
	}


private:

	void FromDouble_SetExpAndMan(bool is_sign, int e, uint mhighest, uint m1, uint m2)
	{
		exponent = e;

		if( man > 1 )
		{
			mantissa.table[man-1] = m1 | mhighest;
			mantissa.table[sint(man-2)] = m2;
			// although man>1 we're using casting into sint
			// to get rid from a warning which generates Microsoft Visual:
			// warning C4307: '*' : integral constant overflow

			for(uint i=0 ; i<man-2 ; ++i)
				mantissa.table[i] = 0;
		}
		else
		{
			mantissa.table[0] = m1 | mhighest;
		}

		info = 0;
	
		// the value should be different from zero
		TTMATH_ASSERT( mantissa.IsZero() == false )

		if( is_sign )
			SetSign();
	}


#else

public:

	// 64bit platforms
	uint FromDouble(double value)
	{
		// I am not sure what will be on a plaltform which has 
		// a different endianness... but we use this library only
		// on x86 and amd (intel) 64 bits (as there's a lot of assembler code)
		union 
		{
			double d;
			uint u; // one 64bit word
		} temp;

		temp.d = value;
                          
		sint e = (temp.u & 0x7FF0000000000000ul) >> 52;
		uint m = (temp.u &    0xFFFFFFFFFFFFFul) << 11;
		
		if( e == 2047 )
		{
			// If E=2047 and F is nonzero, then V=NaN ("Not a number")
			// If E=2047 and F is zero and S is 1, then V=-Infinity
			// If E=2047 and F is zero and S is 0, then V=Infinity

			// we do not support -Infinity and +Infinity
			// we assume that there is always NaN 

			SetNan();
		}
		else
		if( e > 0 )
		{
			// If 0<E<2047 then
			// V=(-1)**S * 2 ** (E-1023) * (1.F)
			// where "1.F" is intended to represent the binary number
			// created by prefixing F with an implicit leading 1 and a binary point.
			
			FromDouble_SetExpAndMan((temp.u & 0x8000000000000000ul) != 0,
									e - 1023 - man*TTMATH_BITS_PER_UINT + 1,
									0x8000000000000000ul, m);

			// we do not have to call Standardizing() here
			// because the mantissa will have the highest bit set
		}
		else
		{
			// e == 0

			if( m != 0 )
			{
				// If E=0 and F is nonzero,
				// then V=(-1)**S * 2 ** (-1022) * (0.F)
				// These are "unnormalized" values.

				FromDouble_SetExpAndMan(bool(temp.u & 0x8000000000000000ul),
										e - 1022 - man*TTMATH_BITS_PER_UINT + 1, 0, m);
				Standardizing();
			}
			else
			{
				// If E=0 and F is zero and S is 1, then V=-0
				// If E=0 and F is zero and S is 0, then V=0 

				// we do not support -0 or 0, only is one 0
				SetZero();
			}
		}

	return 0; // never be a carry
	}

private:

	void FromDouble_SetExpAndMan(bool is_sign, sint e, uint mhighest, uint m)
	{
		exponent = e;
		mantissa.table[man-1] = m | mhighest;

		for(uint i=0 ; i<man-1 ; ++i)
			mantissa.table[i] = 0;

		info = 0;

		// the value should be different from zero
		TTMATH_ASSERT( mantissa.IsZero() == false )

		if( is_sign )
			SetSign();
	}

#endif


public:


	/*!
		this method converts from float to this class
	*/
	uint FromFloat(float value)
	{
		return FromDouble(double(value));
	}


	/*!
		this method converts from this class into the 'double'

		if the value is too big:
			'result' will be +/-infinity (depending on the sign)
		if the value is too small:
			'result' will be 0
	*/
	double ToDouble() const
	{
	double result;

		ToDouble(result);

	return result;
	}


private:


	/*!
		an auxiliary method to check if the float value is +/-infinity
		we provide this method because isinf(float) in only in C99 language

		description taken from: http://www.psc.edu/general/software/packages/ieee/ieee.php

		The IEEE single precision floating point standard representation requires a 32 bit word,
		which may be represented as numbered from 0 to 31, left to right.
		The first bit is the sign bit, S, the next eight bits are the exponent bits, 'E',
		and the final 23 bits are the fraction 'F':

		S EEEEEEEE FFFFFFFFFFFFFFFFFFFFFFF
		0 1      8 9                    31

		The value V represented by the word may be determined as follows:

			* If E=255 and F is nonzero, then V=NaN ("Not a number")
			* If E=255 and F is zero and S is 1, then V=-Infinity
			* If E=255 and F is zero and S is 0, then V=Infinity
			* If 0<E<255 then V=(-1)**S * 2 ** (E-127) * (1.F) where "1.F" is intended to represent
			  the binary number created by prefixing F with an implicit leading 1 and a binary point.
			* If E=0 and F is nonzero, then V=(-1)**S * 2 ** (-126) * (0.F) These are "unnormalized" values.
			* If E=0 and F is zero and S is 1, then V=-0
			* If E=0 and F is zero and S is 0, then V=0 		
	*/
	bool IsInf(float value) const
	{
		// need testing on a 64 bit machine

		union 
		{
			float d;
			uint u;
		} temp;

		temp.d = value;

		if( ((temp.u >> 23) & 0xff) == 0xff )
		{
			if( (temp.u & 0x7FFFFF) == 0 )
				return true; // +/- infinity
		}

	return false;
	}


public:

	/*!
		this method converts from this class into the 'float'

		if the value is too big:
			'result' will be +/-infinity (depending on the sign)
		if the value is too small:
			'result' will be 0
	*/
	float ToFloat() const
	{
	float result;

		ToFloat(result);

	return result;
	}


	/*!
		this method converts from this class into the 'float'

		if the value is too big:
			'result' will be +/-infinity (depending on the sign)
			and the method returns 1
		if the value is too small:
			'result' will be 0
			and the method returns 1
	*/
	uint ToFloat(float & result) const
	{
	double result_double;

		uint c = ToDouble(result_double);
		result = float(result_double);
		
		if( result == -0.0f )
			result = 0.0f;

		if( c )
			return 1;

		// although the result_double can have a correct value
		// but after converting to float there can be infinity

		if( IsInf(result) )
			return 1;

		if( result == 0.0f && result_double != 0.0 )
			// result_double was too small for float
			return 1;

	return 0;
	}


	/*!
		this method converts from this class into the 'double'

		if the value is too big:
			'result' will be +/-infinity (depending on the sign)
			and the method returns 1
		if the value is too small:
			'result' will be 0
			and the method returns 1
	*/
	uint ToDouble(double & result) const
	{
		if( IsZero() )
		{
			result = 0.0;
			return 0;
		}

		if( IsNan() )
		{
			result = ToDouble_SetDouble( false, 2047, 0, false, true);

		return 0;
		}

		sint e_correction = sint(man*TTMATH_BITS_PER_UINT) - 1;

		if( exponent >= 1024 - e_correction )
		{
			// +/- infinity
			result = ToDouble_SetDouble( IsSign(), 2047, 0, true);

		return 1;
		}
		else
		if( exponent <= -1023 - 52 - e_correction )
		{
			// too small value - we assume that there'll be a zero
			result = 0;

			// and return a carry
		return 1;
		}
		
		sint e = exponent.ToInt() + e_correction;

		if( e <= -1023 )
		{
			// -1023-52 < e <= -1023  (unnormalized value)
			result = ToDouble_SetDouble( IsSign(), 0, -(e + 1023));
		}
		else
		{
			// -1023 < e < 1024
			result = ToDouble_SetDouble( IsSign(), e + 1023, -1);
		}

	return 0;
	}

private:

#ifdef TTMATH_PLATFORM32

	// 32bit platforms
	double ToDouble_SetDouble(bool is_sign, uint e, sint move, bool infinity = false, bool nan = false) const
	{
		union 
		{
			double d;
			uint u[2]; // two 32bit words
		} temp;

		temp.u[0] = temp.u[1] = 0;

		if( is_sign )
			temp.u[1] |= 0x80000000u;

		temp.u[1] |= (e << 20) & 0x7FF00000u;

		if( nan )
		{
			temp.u[0] |= 1;
			return temp.d;
		}

		if( infinity )
			return temp.d;

		UInt<2> m;
		m.table[1] = mantissa.table[man-1];
		m.table[0] = ( man > 1 ) ? mantissa.table[sint(man-2)] : 0;
		// although man>1 we're using casting into sint
		// to get rid from a warning which generates Microsoft Visual:
		// warning C4307: '*' : integral constant overflow

		m.Rcr( 12 + move );
		m.table[1] &= 0xFFFFFu; // cutting the 20 bit (when 'move' was -1)

		temp.u[1] |= m.table[1];
		temp.u[0] |= m.table[0];

	return temp.d;
	}

#else

	// 64bit platforms
	double ToDouble_SetDouble(bool is_sign, uint e, sint move, bool infinity = false, bool nan = false) const
	{
		union 
		{
			double d;
			uint u; // 64bit word
		} temp;

		temp.u = 0;
		
		if( is_sign )
			temp.u |= 0x8000000000000000ul;
		                
		temp.u |= (e << 52) & 0x7FF0000000000000ul;

		if( nan )
		{
			temp.u |= 1;
			return temp.d;
		}

		if( infinity )
			return temp.d;

		uint m = mantissa.table[man-1];

		m >>= ( 12 + move );
		m &= 0xFFFFFFFFFFFFFul; // cutting the 20 bit (when 'move' was -1)
		temp.u |= m;

	return temp.d;
	}

#endif


public:


	/*!
		an operator= for converting 'sint' to this class
	*/
	Big<exp, man> & operator=(sint value)
	{
		FromInt(value);

	return *this;
	}


	/*!
		an operator= for converting 'uint' to this class
	*/
	Big<exp, man> & operator=(uint value)
	{
		FromUInt(value);

	return *this;
	}


	/*!
		an operator= for converting 'float' to this class
	*/
	Big<exp, man> & operator=(float value)
	{
		FromFloat(value);

	return *this;
	}


	/*!
		an operator= for converting 'double' to this class
	*/
	Big<exp, man> & operator=(double value)
	{
		FromDouble(value);

	return *this;
	}


	/*!
		a constructor for converting 'sint' to this class
	*/
	Big(sint value)
	{
		FromInt(value);
	}

	/*!
		a constructor for converting 'uint' to this class
	*/
	Big(uint value)
	{	
		FromUInt(value);
	}
	

	/*!
		a constructor for converting 'double' to this class
	*/
	Big(double value)
	{
		FromDouble(value);
	}


	/*!
		a constructor for converting 'float' to this class
	*/
	Big(float value)
	{
		FromFloat(value);
	}


#ifdef TTMATH_PLATFORM32

	/*!
		this method converts 'this' into 'result' (64 bit unsigned integer)
		if the value is too big this method returns a carry (1)
	*/
	uint ToUInt(ulint & result) const
	{
	UInt<2> temp; // 64 bits container

		uint c = ToUInt(temp);
		temp.ToUInt(result);

	return c;
	}


	/*!
		this method converts 'this' into 'result' (64 bit unsigned integer)
		if the value is too big this method returns a carry (1)
	*/
	uint ToInt(ulint & result) const
	{
		return ToUInt(result);
	}


	/*!
		this method converts 'this' into 'result' (64 bit unsigned integer)
		if the value is too big this method returns a carry (1)
	*/
	uint ToInt(slint & result) const
	{
	Int<2> temp; // 64 bits container

		uint c = ToInt(temp);
		temp.ToInt(result);

	return c;
	}


	/*!
		a method for converting 'ulint' (64bit unsigned integer) to this class
	*/
	uint FromUInt(ulint value)
	{
		if( value == 0 )
		{
			SetZero();
			return 0;
		}

		info = 0;

		if( man == 1 )
		{
			sint bit = mantissa.FindLeadingBitInWord(uint(value >> TTMATH_BITS_PER_UINT));

			if( bit != -1 )
			{
				// the highest word from value is different from zero
				bit += 1;
				value >>= bit;
				exponent = bit;
			}
			else
			{
				exponent.SetZero();
			}

			mantissa.table[0] = uint(value);
		}
		else
		{
		#ifdef _MSC_VER
		//warning C4307: '*' : integral constant overflow
		#pragma warning( disable: 4307 )
		#endif

			// man >= 2
			mantissa.table[man-1] = uint(value >> TTMATH_BITS_PER_UINT);
			mantissa.table[man-2] = uint(value);

		#ifdef _MSC_VER
		//warning C4307: '*' : integral constant overflow
		#pragma warning( default: 4307 )
		#endif

			exponent = -sint(man-2) * sint(TTMATH_BITS_PER_UINT);

			for(uint i=0 ; i<man-2 ; ++i)
				mantissa.table[i] = 0;
		}

		// there shouldn't be a carry because 'value' has the 'ulint' type 
		// (we have	sufficient exponent)
		Standardizing();

	return 0;
	}


	/*!
		a method for converting 'ulint' (64bit unsigned integer) to this class
	*/
	uint FromInt(ulint value)
	{
		return FromUInt(value);
	}


	/*!
		a method for converting 'slint' (64bit signed integer) to this class
	*/
	uint FromInt(slint value)
	{
	bool is_sign = false;

		if( value < 0 )
		{
			value   = -value;
			is_sign = true;
		}

		FromUInt(ulint(value));

		if( is_sign )
			SetSign();

	return 0;
	}


	/*!
		a constructor for converting 'ulint' (64bit unsigned integer) to this class
	*/
	Big(ulint value)
	{	
		FromUInt(value);
	}


	/*!
		an operator for converting 'ulint' (64bit unsigned integer) to this class
	*/
	Big<exp, man> & operator=(ulint value)
	{	
		FromUInt(value);

	return *this;
	}


	/*!
		a constructor for converting 'slint' (64bit signed integer) to this class
	*/
	Big(slint value)
	{	
		FromInt(value);
	}


	/*!
		an operator for converting 'slint' (64bit signed integer) to this class
	*/
	Big<exp, man> & operator=(slint value)
	{	
		FromInt(value);

	return *this;
	}

#endif



#ifdef TTMATH_PLATFORM64


	/*!
		this method converts 'this' into 'result' (32 bit unsigned integer)
		***this method is created only on a 64bit platform***
		if the value is too big this method returns a carry (1)
	*/
	uint ToUInt(unsigned int & result) const
	{
	uint result_uint;

		uint c = ToUInt(result_uint);
		result = (unsigned int)result_uint;

		if( c || result_uint != uint(result) )
			return 1;

	return 0;
	}


	/*!
		this method converts 'this' into 'result' (32 bit unsigned integer)
		***this method is created only on a 64bit platform***
		if the value is too big this method returns a carry (1)
	*/
	uint ToInt(unsigned int & result) const
	{
		return ToUInt(result);
	}


	/*!
		this method converts 'this' into 'result' (32 bit signed integer)
		***this method is created only on a 64bit platform***
		if the value is too big this method returns a carry (1)
	*/
	uint ToInt(signed int & result) const
	{
	sint result_sint;

		uint c = ToInt(result_sint);
		result = (signed int)result_sint;

		if( c || result_sint != sint(result) )
			return 1;

	return 0;
	}


	/*
		this method converts 32 bit unsigned int to this class
		***this method is created only on a 64bit platform***
	*/
	uint FromUInt(unsigned int value)
	{
		return FromUInt(uint(value));
	}


	/*
		this method converts 32 bit unsigned int to this class
		***this method is created only on a 64bit platform***
	*/
	uint FromInt(unsigned int value)
	{
		return FromUInt(uint(value));
	}


	/*
		this method converts 32 bit signed int to this class
		***this method is created only on a 64bit platform***
	*/
	uint FromInt(signed int value)
	{
		return FromInt(sint(value));
	}


	/*!
		an operator= for converting 32 bit unsigned int to this class
		***this operator is created only on a 64bit platform***
	*/
	Big<exp, man> & operator=(unsigned int value)
	{
		FromUInt(value);

	return *this;
	}


	/*!
		a constructor for converting 32 bit unsigned int to this class
		***this constructor is created only on a 64bit platform***
	*/
	Big(unsigned int value)
	{
		FromUInt(value);
	}


	/*!
		an operator for converting 32 bit signed int to this class
		***this operator is created only on a 64bit platform***
	*/
	Big<exp, man> & operator=(signed int value)
	{
		FromInt(value);

	return *this;
	}


	/*!
		a constructor for converting 32 bit signed int to this class
		***this constructor is created only on a 64bit platform***
	*/
	Big(signed int value)
	{
		FromInt(value);
	}

#endif


private:

	/*!
		an auxiliary method for converting from UInt and Int

		we assume that there'll never be a carry here
		(we have an exponent and the value in Big can be bigger than
		that one from the UInt)
	*/
	template<uint int_size>
	uint FromUIntOrInt(const UInt<int_size> & value, sint compensation)
	{
		uint minimum_size = (int_size < man)? int_size : man;
		exponent          = (sint(int_size)-sint(man)) * sint(TTMATH_BITS_PER_UINT) - compensation;

		// copying the highest words
		uint i;
		for(i=1 ; i<=minimum_size ; ++i)
			mantissa.table[man-i] = value.table[int_size-i];

		// setting the rest of mantissa.table into zero (if some has left)
		for( ; i<=man ; ++i)
			mantissa.table[man-i] = 0;

		// the highest bit is either one or zero (when the whole mantissa is zero)
		// we can only call CorrectZero()
		CorrectZero();

	return 0;
	}


public:

	/*!
		a method for converting from 'UInt<int_size>' to this class
	*/
	template<uint int_size>
	uint FromUInt(UInt<int_size> value)
	{
		info = 0;
		sint compensation = (sint)value.CompensationToLeft();
	
	return FromUIntOrInt(value, compensation);
	}


	/*!
		a method for converting from 'UInt<int_size>' to this class
	*/
	template<uint int_size>
	uint FromInt(const UInt<int_size> & value)
	{
		return FromUInt(value);
	}

		
	/*!
		a method for converting from 'Int<int_size>' to this class
	*/
	template<uint int_size>
	uint FromInt(Int<int_size> value)
	{
		info = 0;
		bool is_sign = false;

		if( value.IsSign() )
		{
			value.ChangeSign();
			is_sign = true;
		}
		
		sint compensation = (sint)value.CompensationToLeft();
		FromUIntOrInt(value, compensation);

		if( is_sign )
			SetSign();

	return 0;
	}


	/*!
		an operator= for converting from 'Int<int_size>' to this class
	*/
	template<uint int_size>
	Big<exp,man> & operator=(const Int<int_size> & value)
	{
		FromInt(value);

	return *this;
	}


	/*!
		a constructor for converting from 'Int<int_size>' to this class
	*/
	template<uint int_size>
	Big(const Int<int_size> & value)
	{
		FromInt(value);
	}


	/*!
		an operator= for converting from 'UInt<int_size>' to this class
	*/
	template<uint int_size>
	Big<exp,man> & operator=(const UInt<int_size> & value)
	{
		FromUInt(value);

	return *this;
	}


	/*!
		a constructor for converting from 'UInt<int_size>' to this class
	*/
	template<uint int_size>
	Big(const UInt<int_size> & value)
	{
		FromUInt(value);
	}


	/*!
		an operator= for converting from 'Big<another_exp, another_man>' to this class
	*/
	template<uint another_exp, uint another_man>
	Big<exp,man> & operator=(const Big<another_exp, another_man> & value)
	{
		FromBig(value);

	return *this;
	}


	/*!
		a constructor for converting from 'Big<another_exp, another_man>' to this class
	*/
	template<uint another_exp, uint another_man>
	Big(const Big<another_exp, another_man> & value)
	{
		FromBig(value);
	}


	/*!
		a default constructor

		by default we don't set any of the members to zero
		only NaN flag is set

		if you want the mantissa and exponent to be set to zero 
		define TTMATH_BIG_DEFAULT_CLEAR macro
		(useful for debug purposes)
	*/
	Big()
	{
		#ifdef TTMATH_BIG_DEFAULT_CLEAR

			SetZeroNan();

		#else

			info = TTMATH_BIG_NAN;
			// we're directly setting 'info' (instead of calling SetNan())
			// in order to get rid of a warning saying that 'info' is uninitialized

		#endif
	}


	/*!
		a destructor
	*/
	~Big()
	{
	}


	/*!
		the default assignment operator
	*/
	Big<exp,man> & operator=(const Big<exp,man> & value)
	{
		info     = value.info;
		exponent = value.exponent;
		mantissa = value.mantissa;

	return *this;
	}


	/*!
		a constructor for copying from another object of this class
	*/
	
	Big(const Big<exp,man> & value)
	{
		operator=(value);
	}
	


	/*!
		a method for converting into a string
		struct Conv is defined in ttmathtypes.h, look there for more information about parameters

		output:
			return value:
			0 - ok and 'result' will be an object of type std::string (or std::wstring) which holds the value
			1 - if there is a carry (it shoudn't be in a normal situation - if it is that means there
			    is somewhere an error in the library)
	*/
	uint ToString(	std::string & result,
					uint base         = 10,
					bool scient       = false,
					sint scient_from  = 15,
					sint round        = -1,
					bool trim_zeroes  = true,
					char comma     = '.' ) const
	{
		Conv conv;

		conv.base         = base;
		conv.scient       = scient;
		conv.scient_from  = scient_from;
		conv.round        = round;
		conv.trim_zeroes  = trim_zeroes;
		conv.comma        = static_cast<uint>(comma);

	return ToStringBase<std::string, char>(result, conv);
	}


	/*!
		a method for converting into a string
		struct Conv is defined in ttmathtypes.h, look there for more information about parameters
	*/
	uint ToString(std::string & result, const Conv & conv) const
	{
		return ToStringBase<std::string, char>(result, conv);
	}


	/*!
		a method for converting into a string
		struct Conv is defined in ttmathtypes.h, look there for more information about parameters
	*/
	std::string ToString(const Conv & conv) const
	{
		std::string result;
		ToStringBase<std::string, char>(result, conv);
		
	return result;
	}


	/*!
		a method for converting into a string
		struct Conv is defined in ttmathtypes.h, look there for more information about parameters
	*/
	std::string ToString(uint base = 10) const
	{
		Conv conv;
		conv.base = base;

	return ToString(conv);
	}



#ifndef TTMATH_DONT_USE_WCHAR


	/*!
		a method for converting into a string
		struct Conv is defined in ttmathtypes.h, look there for more information about parameters
	*/
	uint ToString(	std::wstring & result,
					uint base         = 10,
					bool scient       = false,
					sint scient_from  = 15,
					sint round        = -1,
					bool trim_zeroes  = true,
					wchar_t comma     = '.' ) const
	{
		Conv conv;

		conv.base         = base;
		conv.scient       = scient;
		conv.scient_from  = scient_from;
		conv.round        = round;
		conv.trim_zeroes  = trim_zeroes;
		conv.comma        = static_cast<uint>(comma);

	return ToStringBase<std::wstring, wchar_t>(result, conv);
	}


	/*!
		a method for converting into a string
		struct Conv is defined in ttmathtypes.h, look there for more information about parameters
	*/
	uint ToString(std::wstring & result, const Conv & conv) const
	{
		return ToStringBase<std::wstring, wchar_t>(result, conv);
	}


	/*!
		a method for converting into a string
		struct Conv is defined in ttmathtypes.h, look there for more information about parameters
	*/
	std::wstring ToWString(const Conv & conv) const
	{
		std::wstring result;
		ToStringBase<std::wstring, wchar_t>(result, conv);
		
	return result;
	}


	/*!
		a method for converting into a string
		struct Conv is defined in ttmathtypes.h, look there for more information about parameters
	*/
	std::wstring ToWString(uint base = 10) const
	{
		Conv conv;
		conv.base = base;

	return ToWString(conv);
	}

#endif



private:


	/*!
		an auxiliary method for converting into the string
	*/
	template<class string_type, class char_type>
	uint ToStringBase(string_type & result, const Conv & conv) const
	{
		static char error_overflow_msg[] = "overflow";
		static char error_nan_msg[]      = "NaN";
		result.erase();

		if( IsNan() )
		{
			Misc::AssignString(result, error_nan_msg);
			return 0;
		}

		if( conv.base<2 || conv.base>16 )
		{
			Misc::AssignString(result, error_overflow_msg);
			return 1;
		}
	
		if( IsZero() )
		{
			result = '0';

		return 0;
		}

		/*
			since 'base' is greater or equal 2 that 'new_exp' of type 'Int<exp>' should
			hold the new value of exponent but we're using 'Int<exp+1>' because
			if the value for example would be 'max()' then we couldn't show it

				max() ->  11111111 * 2 ^ 11111111111  (bin)(the mantissa and exponent have all bits set)
				if we were using 'Int<exp>' we couldn't show it in this format:
				1,1111111 * 2 ^ 11111111111  (bin)
				because we have to add something to the mantissa and because 
				mantissa is full we can't do it and it'll be a carry
				(look at ToString_SetCommaAndExponent(...))

				when the base would be greater than two (for example 10) 
				we could use 'Int<exp>' here
		*/
		Int<exp+1> new_exp;

		if( ToString_CreateNewMantissaAndExponent<string_type, char_type>(result, conv, new_exp) )
		{
			Misc::AssignString(result, error_overflow_msg);
			return 1;
		}

			
		if( ToString_SetCommaAndExponent<string_type, char_type>(result, conv, new_exp) )
		{
			Misc::AssignString(result, error_overflow_msg);
			return 1;
		}

		if( IsSign() )
			result.insert(result.begin(), '-');


	// converted successfully
	return 0;
	}



	/*!
		in the method 'ToString_CreateNewMantissaAndExponent()' we're using 
		type 'Big<exp+1,man>' and we should have the ability to use some
		necessary methods from that class (methods which are private here)
	*/
	friend class Big<exp-1,man>;


	/*!
		an auxiliary method for converting into the string

		input:
			base - the base in range <2,16>

		output:
			return values:
				0 - ok
				1 - if there was a carry
			new_man - the new mantissa for 'base'
			new_exp - the new exponent for 'base'

		mathematic part:

		the value is stored as:
			value = mantissa * 2^exponent
		we want to show 'value' as:
			value = new_man * base^new_exp

		then 'new_man' we'll print using the standard method from UInt<> type for printing
		and 'new_exp' is the offset of the comma operator in a system of a base 'base'

		value = mantissa * 2^exponent
		value = mantissa * 2^exponent * (base^new_exp / base^new_exp)
		value = mantissa * (2^exponent / base^new_exp) * base^new_exp

		look at the part (2^exponent / base^new_exp), there'll be good if we take
		a 'new_exp' equal that value when the (2^exponent / base^new_exp) will be equal one

		on account of the 'base' is not as power of 2 (can be from 2 to 16),
		this formula will not be true for integer 'new_exp' then in our case we take 
		'base^new_exp' _greater_ than '2^exponent' 

		if 'base^new_exp' were smaller than '2^exponent' the new mantissa could be
		greater than the max value of the container UInt<man>

		value = mantissa * (2^exponent / base^new_exp) * base^new_exp
		  let M = mantissa * (2^exponent / base^new_exp) then
		value = M * base^new_exp

		in our calculation we treat M as floating value showing it as:
			M = mm * 2^ee where ee will be <= 0 

		next we'll move all bits of mm into the right when ee is equal zero
		abs(ee) must not be too big that only few bits from mm we can leave

		then we'll have:
			M = mmm * 2^0
		'mmm' is the new_man which we're looking for


		new_exp we calculate in this way:
			2^exponent <= base^new_exp
			new_exp >= log base (2^exponent)   <- logarithm with the base 'base' from (2^exponent)
			
			but we need new_exp as integer then we test:
			if new_exp is greater than zero and with fraction we add one to new_exp
			  new_exp = new_exp + 1    (if new_exp>0 and with fraction)
			and at the end we take the integer part:
			  new_exp = int(new_exp)
	*/
	template<class string_type, class char_type>
	uint ToString_CreateNewMantissaAndExponent(	string_type & new_man, const Conv & conv,
												Int<exp+1> & new_exp) const
	{
	uint c = 0;

		if( conv.base<2 || conv.base>16 )
			return 1;
	
		// special method for base equal 2
		if( conv.base == 2 )
			return ToString_CreateNewMantissaAndExponent_Base2(new_man, new_exp);

		// special method for base equal 4
		if( conv.base == 4 )
			return ToString_CreateNewMantissaAndExponent_BasePow2(new_man, new_exp, 2);

		// special method for base equal 8
		if( conv.base == 8 )
			return ToString_CreateNewMantissaAndExponent_BasePow2(new_man, new_exp, 3);

		// special method for base equal 16
		if( conv.base == 16 )
			return ToString_CreateNewMantissaAndExponent_BasePow2(new_man, new_exp, 4);


		// this = mantissa * 2^exponent

		// temp = +1 * 2^exponent  
		// we're using a bigger type than 'big<exp,man>' (look below)
		Big<exp+1,man> temp;
		temp.info = 0;
		temp.exponent = exponent;
		temp.mantissa.SetOne();
		c += temp.Standardizing();

		// new_exp_ = log base (2^exponent)   
		// if new_exp_ is positive and with fraction then we add one 
		Big<exp+1,man> new_exp_;
		c += new_exp_.ToString_Log(temp, conv.base); // this logarithm isn't very complicated

		// rounding up to the nearest integer
		if( !new_exp_.IsInteger() )
		{
			if( !new_exp_.IsSign() )
				c += new_exp_.AddOne(); // new_exp_ > 0 and with fraction

			new_exp_.SkipFraction();
		}

		if( ToString_CreateNewMantissaTryExponent<string_type, char_type>(new_man, conv, new_exp_, new_exp) )
		{
			// in very rare cases there can be an overflow from ToString_CreateNewMantissaTryExponent
			// it means that new_exp_ was too small (the problem comes from floating point numbers precision)
			// so we increse new_exp_ and try again
			new_exp_.AddOne();
			c += ToString_CreateNewMantissaTryExponent<string_type, char_type>(new_man, conv, new_exp_, new_exp);
		}

	return (c==0)? 0 : 1;
	}



	/*!
		an auxiliary method for converting into the string

		trying to calculate new_man for given exponent (new_exp_)
		if there is a carry it can mean that new_exp_ is too small
	*/
	template<class string_type, class char_type>
	uint ToString_CreateNewMantissaTryExponent(	string_type & new_man, const Conv & conv,
												const Big<exp+1,man> & new_exp_, Int<exp+1> & new_exp) const
	{
	uint c = 0;

		// because 'base^new_exp' is >= '2^exponent' then 
		// because base is >= 2 then we've got:
		// 'new_exp_' must be smaller or equal 'new_exp'
		// and we can pass it into the Int<exp> type
		// (in fact we're using a greater type then it'll be ok)
		c += new_exp_.ToInt(new_exp);

		// base_ = base
		Big<exp+1,man> base_(conv.base);

		// base_ = base_ ^ new_exp_
		c += base_.Pow( new_exp_ ); // use new_exp_ so Pow(Big<> &) version will be used
		// if we hadn't used a bigger type than 'Big<exp,man>' then the result
		// of this formula 'Pow(...)' would have been with an overflow

		// temp = mantissa * 2^exponent / base_^new_exp_
		Big<exp+1,man> temp;
		temp.info = 0;
		temp.mantissa = mantissa;
		temp.exponent = exponent;
		c += temp.Div(base_);

		// moving all bits of the mantissa into the right 
		// (how many times to move depend on the exponent)
		c += temp.ToString_MoveMantissaIntoRight();

		// because we took 'new_exp' as small as it was
		// possible ([log base (2^exponent)] + 1) that after the division 
		// (temp.Div( base_ )) the value of exponent should be equal zero or 
		// minimum smaller than zero then we've got the mantissa which has 
		// maximum valid bits
		temp.mantissa.ToString(new_man, conv.base);

		if( IsInteger() )
		{
			// making sure the new mantissa will be without fraction (integer)
			ToString_CheckMantissaInteger<string_type, char_type>(new_man, new_exp);
		}
		else
		if( conv.base_round )
		{
			c += ToString_BaseRound<string_type, char_type>(new_man, conv, new_exp);
		}

	return (c==0)? 0 : 1;
	}


	/*!
		this method calculates the logarithm
		it is used by ToString_CreateNewMantissaAndExponent() method

		it's not too complicated
		because x=+1*2^exponent (mantissa is one) then during the calculation
		the Ln(x) will not be making the long formula from LnSurrounding1()
		and only we have to calculate 'Ln(base)' but it'll be calculated
		only once, the next time we will get it from the 'history'

        x is greater than 0
		base is in <2,16> range
	*/
	uint ToString_Log(const Big<exp,man> & x, uint base)
	{
		TTMATH_REFERENCE_ASSERT( x )
		TTMATH_ASSERT( base>=2 && base<=16 )

		Big<exp,man> temp;
		temp.SetOne();

		if( x == temp )
		{
			// log(1) is 0
			SetZero();

		return 0;
		}

		// there can be only a carry
		// because the 'x' is in '1+2*exponent' form then 
		// the long formula from LnSurrounding1() will not be calculated
		// (LnSurrounding1() will return one immediately)
		uint c = Ln(x);

		if( base==10 && man<=TTMATH_BUILTIN_VARIABLES_SIZE )
		{
			// for the base equal 10 we're using SetLn10() instead of calculating it
			// (only if we have the constant sufficient big)
			temp.SetLn10();
		}
		else
		{
			c += ToString_LogBase(base, temp);
		}

		c += Div( temp );

	return (c==0)? 0 : 1;
	}


#ifndef TTMATH_MULTITHREADS

	/*!
		this method calculates the logarithm of 'base'
		it's used in single thread environment
	*/
	uint ToString_LogBase(uint base, Big<exp,man> & result)
	{
		TTMATH_ASSERT( base>=2 && base<=16 )

		// this guardians are initialized before the program runs (static POD types)
		static int guardians[15] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
		static Big<exp,man> log_history[15];
		uint index = base - 2;
		uint c = 0;
	
		if( guardians[index] == 0 )
		{
			Big<exp,man> base_(base);
			c += log_history[index].Ln(base_);
			guardians[index] = 1;
		}

		result = log_history[index];

	return (c==0)? 0 : 1;
	}

#else

	/*!
		this method calculates the logarithm of 'base'
		it's used in multi-thread environment
	*/
	uint ToString_LogBase(uint base, Big<exp,man> & result)
	{
		TTMATH_ASSERT( base>=2 && base<=16 )

		// this guardians are initialized before the program runs (static POD types)
		volatile static sig_atomic_t guardians[15] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
		static Big<exp,man> * plog_history;
		uint index = base - 2;
		uint c = 0;
	
		// double-checked locking
		if( guardians[index] == 0 )
		{
			ThreadLock thread_lock;

			// locking
			if( thread_lock.Lock() )
			{
				static Big<exp,man> log_history[15];

				if( guardians[index] == 0 )
				{
					plog_history = log_history;
				
					Big<exp,man> base_(base);
					c += log_history[index].Ln(base_);
					guardians[index] = 1;
				}
			}
			else
			{
				// there was a problem with locking, we store the result directly in 'result' object
				Big<exp,man> base_(base);
				c += result.Ln(base_);
				
			return (c==0)? 0 : 1;
			}

			// automatically unlocking
		}

		result = plog_history[index];

	return (c==0)? 0 : 1;
	}

#endif

	/*!
		an auxiliary method for converting into the string (private)

		this method moving all bits from mantissa into the right side
		the exponent tell us how many times moving (the exponent is <=0)
	*/
	uint ToString_MoveMantissaIntoRight()
	{
		if( exponent.IsZero() )
			return 0;
		
		// exponent can't be greater than zero
		// because we would cat the highest bits of the mantissa
		if( !exponent.IsSign() )
			return 1;


		if( exponent <= -sint(man*TTMATH_BITS_PER_UINT) )
			// if 'exponent' is <= than '-sint(man*TTMATH_BITS_PER_UINT)'
			// it means that we must cut the whole mantissa
			// (there'll not be any of the valid bits)
			return 1;

		// e will be from (-man*TTMATH_BITS_PER_UINT, 0>
		sint e = -( exponent.ToInt() );
		mantissa.Rcr(e,0);

	return 0;
	}


	/*!
		a special method similar to the 'ToString_CreateNewMantissaAndExponent'
		when the 'base' is equal 2

		we use it because if base is equal 2 we don't have to make those
		complicated calculations and the output is directly from the source
		(there will not be any small distortions)
	*/
	template<class string_type>
	uint ToString_CreateNewMantissaAndExponent_Base2(	string_type & new_man,
														Int<exp+1> & new_exp     ) const
	{
		for( sint i=man-1 ; i>=0 ; --i )
		{
			uint value = mantissa.table[i]; 

			for( uint bit=0 ; bit<TTMATH_BITS_PER_UINT ; ++bit )
			{
				if( (value & TTMATH_UINT_HIGHEST_BIT) != 0 )
					new_man += '1';
				else
					new_man += '0';

				value <<= 1;
			}
		}

		new_exp = exponent;

	return 0;
	}


	/*!
		a special method used to calculate the new mantissa and exponent
		when the 'base' is equal 4, 8 or 16

		when base is 4 then bits is 2
		when base is 8 then bits is 3
		when base is 16 then bits is 4
		(and the algorithm can be used with a base greater than 16)
	*/
	template<class string_type>
	uint ToString_CreateNewMantissaAndExponent_BasePow2(	string_type & new_man,
															Int<exp+1> & new_exp,
															uint bits) const
	{
		sint move;							// how many times move the mantissa
		UInt<man+1> man_temp(mantissa);		// man+1 for moving
		new_exp = exponent;
		new_exp.DivInt((sint)bits, move);

		if( move != 0 )
		{
			// we're moving the man_temp to left-hand side
			if( move < 0 )
			{
				move = sint(bits) + move;
				new_exp.SubOne();			// when move is < than 0 then new_exp is < 0 too
			}

			man_temp.Rcl(move);
		}


		if( bits == 3 )
		{
			// base 8
			// now 'move' is greater than or equal 0
			uint len = man*TTMATH_BITS_PER_UINT + move;
			return ToString_CreateNewMantissaAndExponent_Base8(new_man, man_temp, len, bits);
		}
		else
		{
			// base 4 or 16
			return ToString_CreateNewMantissaAndExponent_Base4or16(new_man, man_temp, bits);
		}
	}


	/*!
		a special method used to calculate the new mantissa
		when the 'base' is equal 8

		bits is always 3

		we can use this algorithm when the base is 4 or 16 too
		but we have a faster method ToString_CreateNewMantissaAndExponent_Base4or16()
	*/
	template<class string_type>
	uint ToString_CreateNewMantissaAndExponent_Base8(	string_type & new_man,
														UInt<man+1> & man_temp,
														uint len,
														uint bits) const
	{
		uint shift = TTMATH_BITS_PER_UINT - bits;
		uint mask  = TTMATH_UINT_MAX_VALUE >> shift;
		uint i;

		for( i=0 ; i<len ; i+=bits )
		{
			uint digit = man_temp.table[0] & mask;
			new_man.insert(new_man.begin(), static_cast<char>(Misc::DigitToChar(digit)));

			man_temp.Rcr(bits);
		}

		TTMATH_ASSERT( man_temp.IsZero() )

	return 0;
	}


	/*!
		a special method used to calculate the new mantissa
		when the 'base' is equal 4 or 16

		when the base is equal 4 or 16 the bits is 2 or 4
		and because TTMATH_BITS_PER_UINT (32 or 64) is divisible by 2 (or 4)
		then we can get digits from the end of our mantissa
	*/
	template<class string_type>
	uint ToString_CreateNewMantissaAndExponent_Base4or16(	string_type & new_man,
															UInt<man+1> & man_temp,
															uint bits) const
	{
		TTMATH_ASSERT( TTMATH_BITS_PER_UINT % 2 == 0 )
		TTMATH_ASSERT( TTMATH_BITS_PER_UINT % 4 == 0 )

		uint shift = TTMATH_BITS_PER_UINT - bits;
		uint mask  = TTMATH_UINT_MAX_VALUE << shift;
		uint digit;

		 // table[man] - last word - is different from zero if we moved man_temp
		digit = man_temp.table[man];

		if( digit != 0 )
			new_man += static_cast<char>(Misc::DigitToChar(digit));


		for( int i=man-1 ; i>=0 ; --i )
		{
			uint shift_local = shift;
			uint mask_local  = mask;

			while( mask_local != 0 )
			{
				digit = man_temp.table[i] & mask_local;

				if( shift_local != 0 )
					digit = digit >> shift_local;

				new_man    += static_cast<char>(Misc::DigitToChar(digit));
				mask_local  = mask_local >> bits;
				shift_local = shift_local - bits;
			}
		}

	return 0;
	}


	/*!
		an auxiliary method for converting into the string
	*/
	template<class string_type, class char_type>
	bool ToString_RoundMantissaWouldBeInteger(string_type & new_man, const Conv & conv, Int<exp+1> & new_exp) const
	{
		// if new_exp is greater or equal to zero then we have an integer value,
		// if new_exp is equal -1 then we have only one digit after the comma
		// and after rounding it would be an integer value
		if( !new_exp.IsSign() || new_exp == -1 )
			return true;

		if( new_man.size() >= TTMATH_UINT_HIGHEST_BIT || new_man.size() < 2 )
			return true; // oops, the mantissa is too large for calculating (or too small) - we are not doing the base rounding
		
		uint i = 0;
		char_type digit;

		if( new_exp >= -sint(new_man.size()) )
		{
			uint new_exp_abs = -new_exp.ToInt();
			i = new_man.size() - new_exp_abs; // start from the first digit after the comma operator
		}
		
		if( Misc::CharToDigit(new_man[new_man.size()-1]) >= conv.base/2 )
		{
			if( new_exp < -sint(new_man.size()) )
			{
				// there are some zeroes after the comma operator
				// (between the comma and the first digit from the mantissa)
				// and the result value will never be an integer
				return false;
			}

			digit = static_cast<char_type>( Misc::DigitToChar(conv.base-1) );
		}
		else
		{
			digit = '0';
		}

		for( ; i < new_man.size()-1 ; ++i)
			if( new_man[i] != digit )
				return false; // it will not be an integer

	return true; // it will be integer after rounding
	}


	/*!
		an auxiliary method for converting into the string
		(when this is integer)

		after floating point calculating the new mantissa can consist of some fraction
		so if our value is integer we should check the new mantissa
		(after the decimal point there should be only zeroes)
		
		often this is a last digit different from zero
		ToString_BaseRound would not get rid of it because the method make a test against 
		an integer value (ToString_RoundMantissaWouldBeInteger) and returns immediately
	*/
	template<class string_type, class char_type>
	void ToString_CheckMantissaInteger(string_type & new_man, const Int<exp+1> & new_exp) const
	{
		if( !new_exp.IsSign() )
			return; // return if new_exp >= 0
		
		uint i = 0;
		uint man_size = new_man.size();

		if( man_size >= TTMATH_UINT_HIGHEST_BIT )
			return; // ops, the mantissa is too long

		sint sman_size = -sint(man_size);

		if( new_exp >= sman_size )
		{
			sint e = new_exp.ToInt();
			e = -e;
			// now e means how many last digits from the mantissa should be equal zero

			i = man_size - uint(e);
		}

		for( ; i<man_size ; ++i)
			new_man[i] = '0';
	}


	/*!
		an auxiliary method for converting into the string

		this method is used for base!=2, base!=4, base!=8 and base!=16
		we do the rounding when the value has fraction (is not an integer)
	*/
	template<class string_type, class char_type>
	uint ToString_BaseRound(string_type & new_man, const Conv & conv, Int<exp+1> & new_exp) const
	{
		// we must have minimum two characters
		if( new_man.size() < 2 )
			return 0;

		// assert that there will not be an integer after rounding
		if( ToString_RoundMantissaWouldBeInteger<string_type, char_type>(new_man, conv, new_exp) )
			return 0;

		typename string_type::size_type i = new_man.length() - 1;

		// we're erasing the last character
		uint digit = Misc::CharToDigit( new_man[i] );
		new_man.erase(i, 1);
		uint c = new_exp.AddOne();

		// if the last character is greater or equal 'base/2'
		// we are adding one into the new mantissa
		if( digit >= conv.base / 2 )
			ToString_RoundMantissa_AddOneIntoMantissa<string_type, char_type>(new_man, conv);

	return c;
	}
	

	/*!
		an auxiliary method for converting into the string

		this method addes one into the new mantissa
	*/
	template<class string_type, class char_type>
	void ToString_RoundMantissa_AddOneIntoMantissa(string_type & new_man, const Conv & conv) const
	{
		if( new_man.empty() )
			return;

		sint i = sint( new_man.length() ) - 1;
		bool was_carry = true;

		for( ; i>=0 && was_carry ; --i )
		{
			// we can have the comma as well because
			// we're using this method later in ToString_CorrectDigitsAfterComma_Round()
			// (we're only ignoring it)
			if( new_man[i] == static_cast<char_type>(conv.comma) )
				continue;

			// we're adding one
			uint digit = Misc::CharToDigit( new_man[i] ) + 1;

			if( digit == conv.base )
				digit = 0;
			else
				was_carry = false;

			new_man[i] = static_cast<char_type>( Misc::DigitToChar(digit) );
		}

		if( i<0 && was_carry )
			new_man.insert( new_man.begin() , '1' );
	}



	/*!
		an auxiliary method for converting into the string

		this method sets the comma operator and/or puts the exponent
		into the string
	*/
	template<class string_type, class char_type>
	uint ToString_SetCommaAndExponent(string_type & new_man, const Conv & conv, Int<exp+1> & new_exp) const
	{
	uint carry = 0;

		if( new_man.empty() )
			return carry;

		Int<exp+1> scientific_exp( new_exp );

		// 'new_exp' depends on the 'new_man' which is stored like this e.g:
		//  32342343234 (the comma is at the end)
		// we'd like to show it in this way:
		//  3.2342343234 (the 'scientific_exp' is connected with this example)

		sint offset = sint( new_man.length() ) - 1;
		carry += scientific_exp.Add( offset );
		// there shouldn't have been a carry because we're using
		// a greater type -- 'Int<exp+1>' instead of 'Int<exp>'

		bool print_scientific = conv.scient;

		if( !print_scientific )
		{
			if( scientific_exp > conv.scient_from || scientific_exp < -sint(conv.scient_from) )
				print_scientific = true;
		}

		if( !print_scientific )
			ToString_SetCommaAndExponent_Normal<string_type, char_type>(new_man, conv, new_exp);
		else
			// we're passing the 'scientific_exp' instead of 'new_exp' here
			ToString_SetCommaAndExponent_Scientific<string_type, char_type>(new_man, conv, scientific_exp);

	return (carry==0)? 0 : 1;
	}


	/*!
		an auxiliary method for converting into the string
	*/
	template<class string_type, class char_type>
	void ToString_SetCommaAndExponent_Normal(string_type & new_man,	const Conv & conv, Int<exp+1> & new_exp ) const
	{
		if( !new_exp.IsSign() ) // it means: if( new_exp >= 0 )
			ToString_SetCommaAndExponent_Normal_AddingZero<string_type, char_type>(new_man, new_exp);
		else
			ToString_SetCommaAndExponent_Normal_SetCommaInside<string_type, char_type>(new_man, conv, new_exp);


		ToString_Group_man<string_type, char_type>(new_man, conv);
	}


	/*!
		an auxiliary method for converting into the string
	*/
	template<class string_type, class char_type>
	void ToString_SetCommaAndExponent_Normal_AddingZero(string_type & new_man,
														Int<exp+1> & new_exp) const
	{
		// we're adding zero characters at the end
		// 'i' will be smaller than 'when_scientific' (or equal)
		uint i = new_exp.ToInt();
		
		if( new_man.length() + i > new_man.capacity() )
			// about 6 characters more (we'll need it for the comma or something)
			new_man.reserve( new_man.length() + i + 6 );
		
		for( ; i>0 ; --i)
			new_man += '0';
	}


	/*!
		an auxiliary method for converting into the string
	*/
	template<class string_type, class char_type>
	void ToString_SetCommaAndExponent_Normal_SetCommaInside(
															string_type & new_man,
															const Conv & conv,
															Int<exp+1> & new_exp ) const
	{
		// new_exp is < 0 

		sint new_man_len = sint(new_man.length()); // 'new_man_len' with a sign
		sint e = -( new_exp.ToInt() ); // 'e' will be positive

		if( new_exp > -new_man_len )
		{
			// we're setting the comma within the mantissa
			
			sint index = new_man_len - e;
			new_man.insert( new_man.begin() + index, static_cast<char_type>(conv.comma));
		}
		else
		{
			// we're adding zero characters before the mantissa

			uint how_many = e - new_man_len;
			string_type man_temp(how_many+1, '0');

			man_temp.insert( man_temp.begin()+1, static_cast<char_type>(conv.comma));
			new_man.insert(0, man_temp);
		}

		ToString_CorrectDigitsAfterComma<string_type, char_type>(new_man, conv);
	}


	/*!
		an auxiliary method for converting into the string
	*/
	template<class string_type, class char_type>
	void ToString_SetCommaAndExponent_Scientific(	string_type & new_man,
													const Conv & conv,
													Int<exp+1> & scientific_exp ) const
	{
		if( new_man.empty() )
			return;
		
		if( new_man.size() > 1 )
		{
			new_man.insert( new_man.begin()+1, static_cast<char_type>(conv.comma) );
			ToString_CorrectDigitsAfterComma<string_type, char_type>(new_man, conv);
		}

		ToString_Group_man<string_type, char_type>(new_man, conv);

		if( conv.base == 10 )
		{
			new_man += 'e';

			if( !scientific_exp.IsSign() )
				new_man += '+';
		}
		else
		{
			// the 10 here is meant as the base 'base'
			// (no matter which 'base' we're using there'll always be 10 here)
			Misc::AddString(new_man, "*10^");
		}

		string_type temp_exp;
		scientific_exp.ToString( temp_exp, conv.base );

		new_man += temp_exp;
	}


	/*!
		an auxiliary method for converting into the string
	*/
	template<class string_type, class char_type>
	void ToString_Group_man(string_type & new_man, const Conv & conv) const
	{
		typedef typename string_type::size_type StrSize;

		if( conv.group == 0 )
			return;

		// first we're looking for the comma operator
		StrSize index = new_man.find(static_cast<char_type>(conv.comma), 0);

		if( index == string_type::npos )
			index = new_man.size();	

		ToString_Group_man_before_comma<string_type, char_type>(new_man, conv, index);
		ToString_Group_man_after_comma<string_type, char_type>(new_man, conv, index+1);
	}



	/*!
		an auxiliary method for converting into the string
	*/
	template<class string_type, class char_type>
	void ToString_Group_man_before_comma(	string_type & new_man, const Conv & conv,
											typename string_type::size_type & index) const
	{
	typedef typename string_type::size_type StrSize;

		uint group = 0;
		StrSize i = index;
		uint group_digits = conv.group_digits;

		if( group_digits < 1 )
			group_digits = 1;

		// adding group characters before the comma operator
		// i>0 because on the first position we don't put any additional grouping characters
		for( ; i>0 ; --i, ++group)
		{
			if( group >= group_digits )
			{
				group = 0;
				new_man.insert(i, 1, static_cast<char_type>(conv.group));
				++index;
			}
		}
	}


	/*!
		an auxiliary method for converting into the string
	*/
	template<class string_type, class char_type>
	void ToString_Group_man_after_comma(string_type & new_man, const Conv & conv,
										typename string_type::size_type index) const
	{
		uint group = 0;
		uint group_digits = conv.group_digits;

		if( group_digits < 1 )
			group_digits = 1;

		for( ; index<new_man.size() ; ++index, ++group)
		{
			if( group >= group_digits )
			{
				group = 0;
				new_man.insert(index, 1, static_cast<char_type>(conv.group));
				++index;
			}
		}
	}


	/*!
		an auxiliary method for converting into the string
	*/
	template<class string_type, class char_type>
	void ToString_CorrectDigitsAfterComma(	string_type & new_man,
											const Conv & conv ) const
	{
		if( conv.round >= 0 )
			ToString_CorrectDigitsAfterComma_Round<string_type, char_type>(new_man, conv);

		if( conv.trim_zeroes )
			ToString_CorrectDigitsAfterComma_CutOffZeroCharacters<string_type, char_type>(new_man, conv);
	}


	/*!
		an auxiliary method for converting into the string
	*/
	template<class string_type, class char_type>
	void ToString_CorrectDigitsAfterComma_CutOffZeroCharacters(
												string_type & new_man,
												const Conv & conv) const
	{
		// minimum two characters
		if( new_man.length() < 2 )
			return;

		// we're looking for the index of the last character which is not zero
		uint i = uint( new_man.length() ) - 1;
		for( ; i>0 && new_man[i]=='0' ; --i );

		// if there is another character than zero at the end
		// we're finishing
		if( i == new_man.length() - 1 )
			return;

		// we must have a comma 
		// (the comma can be removed by ToString_CorrectDigitsAfterComma_Round
		// which is called before)
		if( new_man.find_last_of(static_cast<char_type>(conv.comma), i) == string_type::npos )
			return;

		// if directly before the first zero is the comma operator
		// we're cutting it as well
		if( i>0 && new_man[i]==static_cast<char_type>(conv.comma) )
			--i;

		new_man.erase(i+1, new_man.length()-i-1);
	}


	/*!
		an auxiliary method for converting into the string
	*/
	template<class string_type, class char_type>
	void ToString_CorrectDigitsAfterComma_Round(
											string_type & new_man,
											const Conv & conv ) const
	{
		typedef typename string_type::size_type StrSize;

		// first we're looking for the comma operator
		StrSize index = new_man.find(static_cast<char_type>(conv.comma), 0);

		if( index == string_type::npos )
			// nothing was found (actually there can't be this situation)
			return;

		// we're calculating how many digits there are at the end (after the comma)
		// 'after_comma' will be greater than zero because at the end
		// we have at least one digit
		StrSize after_comma = new_man.length() - index - 1;

		// if 'max_digit_after_comma' is greater than 'after_comma' (or equal)
		// we don't have anything for cutting
		if( static_cast<StrSize>(conv.round) >= after_comma )
			return;

		uint last_digit = Misc::CharToDigit( new_man[ index + conv.round + 1 ], conv.base );

		// we're cutting the rest of the string
		new_man.erase(index + conv.round + 1, after_comma - conv.round);

		if( conv.round == 0 )
		{
			// we're cutting the comma operator as well
			// (it's not needed now because we've cut the whole rest after the comma)
			new_man.erase(index, 1);
		}

		if( last_digit >= conv.base / 2 )
			// we must round here
			ToString_RoundMantissa_AddOneIntoMantissa<string_type, char_type>(new_man, conv);
	}



public:

	/*!
		a method for converting a string into its value

		it returns 1 if the value is too big -- we cannot pass it into the range
		of our class Big<exp,man> (or if the base is incorrect)

		that means only digits before the comma operator can make this value too big, 
		all digits after the comma we can ignore

		'source' - pointer to the string for parsing

		if 'after_source' is set that when this method finishes
		it sets the pointer to the new first character after parsed value

		'value_read' - if the pointer is provided that means the value_read will be true
		only when a value has been actually read, there can be situation where only such
		a string '-' or '+' will be parsed -- 'after_source' will be different from 'source' but
		no value has been read (there are no digits)
		on other words if 'value_read' is true -- there is at least one digit in the string
	*/
	uint FromString(const char * source, uint base = 10, const char ** after_source = 0, bool * value_read = 0)
	{
		Conv conv;
		conv.base = base;

		return FromStringBase(source, conv, after_source, value_read);
	}


	/*!
		a method for converting a string into its value
	*/
	uint FromString(const char * source, const Conv & conv, const char ** after_source = 0, bool * value_read = 0)
	{
		return FromStringBase(source, conv, after_source, value_read);
	}


	/*!
		a method for converting a string into its value		
	*/
	uint FromString(const std::string & string, uint base = 10, const char ** after_source = 0, bool * value_read = 0)
	{
		return FromString(string.c_str(), base, after_source, value_read);
	}


	/*!
		a method for converting a string into its value		
	*/
	uint FromString(const std::string & string, const Conv & conv, const char ** after_source = 0, bool * value_read = 0)
	{
		return FromString(string.c_str(), conv, after_source, value_read);
	}


#ifndef TTMATH_DONT_USE_WCHAR

	/*!
		a method for converting a string into its value
	*/
	uint FromString(const wchar_t * source, uint base = 10, const wchar_t ** after_source = 0, bool * value_read = 0)
	{
		Conv conv;
		conv.base = base;

		return FromStringBase(source, conv, after_source, value_read);
	}


	/*!
		a method for converting a string into its value
	*/
	uint FromString(const wchar_t * source, const Conv & conv, const wchar_t ** after_source = 0, bool * value_read = 0)
	{
		return FromStringBase(source, conv, after_source, value_read);
	}


	/*!
		a method for converting a string into its value		
	*/
	uint FromString(const std::wstring & string, uint base = 10, const wchar_t ** after_source = 0, bool * value_read = 0)
	{
		return FromString(string.c_str(), base, after_source, value_read);
	}


	/*!
		a method for converting a string into its value		
	*/
	uint FromString(const std::wstring & string, const Conv & conv, const wchar_t ** after_source = 0, bool * value_read = 0)
	{
		return FromString(string.c_str(), conv, after_source, value_read);
	}

#endif


private:


	/*!
		an auxiliary method for converting from a string
	*/
	template<class char_type>
	uint FromStringBase(const char_type * source, const Conv & conv, const char_type ** after_source = 0, bool * value_read = 0)
	{
	bool is_sign;
	bool value_read_temp = false;

		if( conv.base<2 || conv.base>16 )
		{
			SetNan();

			if( after_source )
				*after_source = source;

			if( value_read )
				*value_read = value_read_temp;

			return 1;
		}

		SetZero();
		FromString_TestSign( source, is_sign );

		uint c = FromString_ReadPartBeforeComma( source, conv, value_read_temp );

		if( FromString_TestCommaOperator(source, conv) )
			c += FromString_ReadPartAfterComma( source, conv, value_read_temp );

		if( value_read_temp && conv.base == 10 )
			c += FromString_ReadScientificIfExists( source );

		if( is_sign && !IsZero() )
			ChangeSign();

		if( after_source )
			*after_source = source;

		if( value_read )
			*value_read = value_read_temp;

	return CheckCarry(c);
	}


	/*!
		we're testing whether the value is with the sign

		(this method is used from 'FromString_ReadPartScientific' too)
	*/
	template<class char_type>
	void FromString_TestSign( const char_type * & source, bool & is_sign )
	{
		Misc::SkipWhiteCharacters(source);

		is_sign = false;

		if( *source == '-' )
		{
			is_sign = true;
			++source;
		}
		else
		if( *source == '+' )
		{
			++source;
		}
	}


	/*!
		we're testing whether there's a comma operator
	*/
	template<class char_type>
	bool FromString_TestCommaOperator(const char_type * & source, const Conv & conv)
	{
		if( (*source == static_cast<char_type>(conv.comma)) || 
			(*source == static_cast<char_type>(conv.comma2) && conv.comma2 != 0 ) )
		{
			++source;

		return true;
		}

	return false;
	}


	/*!
		this method reads the first part of a string
		(before the comma operator)
	*/
	template<class char_type>
	uint FromString_ReadPartBeforeComma( const char_type * & source, const Conv & conv, bool & value_read )
	{
		sint character;
		Big<exp, man> temp;
		Big<exp, man> base_( conv.base );
		
		Misc::SkipWhiteCharacters( source );

		for( ; true ; ++source )
		{
			if( conv.group!=0 && *source==static_cast<char>(conv.group) )
				continue;

			character = Misc::CharToDigit(*source, conv.base);

			if( character == -1 )
				break;

			value_read = true;
			temp = character;

			if( Mul(base_) )
				return 1;

			if( Add(temp) )
				return 1;
		}

	return 0;
	}


	/*!
		this method reads the second part of a string
		(after the comma operator)
	*/
	template<class char_type>
	uint FromString_ReadPartAfterComma( const char_type * & source, const Conv & conv, bool & value_read )
	{
	sint character;
	uint c = 0, index = 1;
	Big<exp, man> sum, part, power, old_value, base_( conv.base );

		// we don't remove any white characters here

		// this is only to avoid getting a warning about an uninitialized object 'old_value' which GCC reports
		// (in fact we will initialize it later when the condition 'testing' is fulfilled)
		old_value.SetZero();

		power.SetOne();
		sum.SetZero();

		for( ; true ; ++source, ++index )
		{
			if( conv.group!=0 && *source==static_cast<char>(conv.group) )
				continue;
			
			character = Misc::CharToDigit(*source, conv.base);

			if( character == -1 )
				break;

			value_read = true;

			part = character;

			if( power.Mul( base_ ) )
				// there's no sens to add the next parts, but we can't report this
				// as an error (this is only inaccuracy)
				break;

			if( part.Div( power ) )
				break;

			// every 5 iteration we make a test whether the value will be changed or not
			// (character must be different from zero to this test)
			bool testing = (character != 0 && (index % 5) == 0);

			if( testing )
				old_value = sum;

			// there actually shouldn't be a carry here
			c += sum.Add( part );

			if( testing && old_value == sum )
				// after adding 'part' the value has not been changed
				// there's no sense to add any next parts
				break;
		}

		// we could break the parsing somewhere in the middle of the string,
		// but the result (value) still can be good
		// we should set a correct value of 'source' now
		for( ; Misc::CharToDigit(*source, conv.base) != -1 ; ++source );

		c += Add(sum);

	return (c==0)? 0 : 1;
	}


	/*!
		this method checks whether there is a scientific part: [e|E][-|+]value

		it is called when the base is 10 and some digits were read before
	*/
	template<class char_type>
	uint FromString_ReadScientificIfExists(const char_type * & source)
	{
	uint c = 0;

		bool scientific_read = false;
		const char_type * before_scientific = source;

		if( FromString_TestScientific(source) )
			c += FromString_ReadPartScientific( source, scientific_read );

		if( !scientific_read )
			source = before_scientific;

	return (c==0)? 0 : 1;
	}



	/*!
		we're testing whether is there the character 'e'

		this character is only allowed when we're using the base equals 10
	*/
	template<class char_type>
	bool FromString_TestScientific(const char_type * & source)
	{
		Misc::SkipWhiteCharacters(source);

		if( *source=='e' || *source=='E' )
		{
			++source;

		return true;
		}

	return false;
	}


	/*!
		this method reads the exponent (after 'e' character) when there's a scientific
		format of value and only when we're using the base equals 10
	*/
	template<class char_type>
	uint FromString_ReadPartScientific( const char_type * & source, bool & scientific_read )
	{
	uint c = 0;
	Big<exp, man> new_exponent, temp;
	bool was_sign = false;

		FromString_TestSign( source, was_sign );
		c += FromString_ReadPartScientific_ReadExponent( source, new_exponent, scientific_read );

		if( scientific_read )
		{
			if( was_sign )
				new_exponent.ChangeSign();

			temp = 10;
			c += temp.Pow( new_exponent );
			c += Mul(temp);
		}

	return (c==0)? 0 : 1;
	}


	/*!
		this method reads the value of the extra exponent when scientific format is used
		(only when base == 10)
	*/
	template<class char_type>
	uint FromString_ReadPartScientific_ReadExponent( const char_type * & source, Big<exp, man> & new_exponent, bool & scientific_read )
	{
	sint character;
	Big<exp, man> base, temp;

		Misc::SkipWhiteCharacters(source);

		new_exponent.SetZero();
		base = 10;

		for( ; (character=Misc::CharToDigit(*source, 10)) != -1 ; ++source )
		{
			scientific_read = true;

			temp = character;

			if( new_exponent.Mul(base) )
				return 1;

			if( new_exponent.Add(temp) )
				return 1;
		}

	return 0;
	}


public:


	/*!
		a constructor for converting a string into this class
	*/
	Big(const char * string)
	{
		FromString( string );
	}


	/*!
		a constructor for converting a string into this class
	*/
	Big(const std::string & string)
	{
		FromString( string.c_str() );
	}


	/*!
		an operator= for converting a string into its value
	*/
	Big<exp, man> & operator=(const char * string)
	{
		FromString( string );

	return *this;
	}


	/*!
		an operator= for converting a string into its value
	*/
	Big<exp, man> & operator=(const std::string & string)
	{
		FromString( string.c_str() );

	return *this;
	}



#ifndef TTMATH_DONT_USE_WCHAR

	/*!
		a constructor for converting a string into this class
	*/
	Big(const wchar_t * string)
	{
		FromString( string );
	}


	/*!
		a constructor for converting a string into this class
	*/
	Big(const std::wstring & string)
	{
		FromString( string.c_str() );
	}


	/*!
		an operator= for converting a string into its value
	*/
	Big<exp, man> & operator=(const wchar_t * string)
	{
		FromString( string );

	return *this;
	}


	/*!
		an operator= for converting a string into its value
	*/
	Big<exp, man> & operator=(const std::wstring & string)
	{
		FromString( string.c_str() );

	return *this;
	}


#endif



	/*!
	*
	*	methods for comparing
	*
	*/


	/*!
		this method performs the formula 'abs(this) < abs(ss2)'
		and returns the result

		(in other words it treats 'this' and 'ss2' as values without a sign)
		we don't check the NaN flag
	*/
	bool SmallerWithoutSignThan(const Big<exp,man> & ss2) const
	{
		if( IsZero() )
		{
			if( ss2.IsZero() )
				// we've got two zeroes
				return false;
			else
				// this==0 and ss2!=0
				return true;
		}

		if( ss2.IsZero() )
			// this!=0 and ss2==0
			return false;

		// we're using the fact that all bits in mantissa are pushed
		// into the left side -- Standardizing()
		if( exponent == ss2.exponent )
			return mantissa < ss2.mantissa;

	return exponent < ss2.exponent;
	}


	/*!
		this method performs the formula 'abs(this) > abs(ss2)'
		and returns the result

		(in other words it treats 'this' and 'ss2' as values without a sign)
		we don't check the NaN flag
	*/
	bool GreaterWithoutSignThan(const Big<exp,man> & ss2) const
	{
		if( IsZero() )
		{
			if( ss2.IsZero() )
				// we've got two zeroes
				return false;
			else
				// this==0 and ss2!=0
				return false;
		}

		if( ss2.IsZero() )
			// this!=0 and ss2==0
			return true;

		// we're using the fact that all bits in mantissa are pushed
		// into the left side -- Standardizing()
		if( exponent == ss2.exponent )
			return mantissa > ss2.mantissa;

	return exponent > ss2.exponent;
	}


	/*!
		this method performs the formula 'abs(this) == abs(ss2)'
		and returns the result

		(in other words it treats 'this' and 'ss2' as values without a sign)
		we don't check the NaN flag
	*/
	bool EqualWithoutSign(const Big<exp,man> & ss2) const
	{
		if( IsZero() )
		{
			if( ss2.IsZero() )
				// we've got two zeroes
				return true;
			else
				// this==0 and ss2!=0
				return false;
		}

		if( ss2.IsZero() )
			// this!=0 and ss2==0
			return false;

		if( exponent==ss2.exponent && mantissa==ss2.mantissa )
			return true;

	return false;
	}


	bool operator<(const Big<exp,man> & ss2) const
	{
		if( IsSign() && !ss2.IsSign() )
			// this<0 and ss2>=0
			return true;

		if( !IsSign() && ss2.IsSign() )
			// this>=0 and ss2<0
			return false;

		// both signs are the same

		if( IsSign() )
			return ss2.SmallerWithoutSignThan( *this );

	return SmallerWithoutSignThan( ss2 );
	}


	bool operator==(const Big<exp,man> & ss2) const
	{
		if( IsSign() != ss2.IsSign() )
			return false;

	return EqualWithoutSign( ss2 );
	}


	bool operator>(const Big<exp,man> & ss2) const
	{
		if( IsSign() && !ss2.IsSign() )
			// this<0 and ss2>=0
			return false;

		if( !IsSign() && ss2.IsSign() )
			// this>=0 and ss2<0
			return true;

		// both signs are the same

		if( IsSign() )
			return ss2.GreaterWithoutSignThan( *this );

	return GreaterWithoutSignThan( ss2 );
	}


	bool operator>=(const Big<exp,man> & ss2) const
	{
		return !operator<( ss2 );
	}


	bool operator<=(const Big<exp,man> & ss2) const
	{
		return !operator>( ss2 );
	}


	bool operator!=(const Big<exp,man> & ss2) const
	{
		return !operator==(ss2);
	}





	/*!
	*
	*	standard mathematical operators 
	*
	*/


	/*!
		an operator for changing the sign

		this method is not changing 'this' but the changed value is returned
	*/
	Big<exp,man> operator-() const
	{
		Big<exp,man> temp(*this);

		temp.ChangeSign();

	return temp;
	}


	Big<exp,man> operator-(const Big<exp,man> & ss2) const
	{
	Big<exp,man> temp(*this);

		temp.Sub(ss2);

	return temp;
	}

	Big<exp,man> & operator-=(const Big<exp,man> & ss2)
	{
		Sub(ss2);

	return *this;
	}


	Big<exp,man> operator+(const Big<exp,man> & ss2) const
	{
	Big<exp,man> temp(*this);

		temp.Add(ss2);

	return temp;
	}


	Big<exp,man> & operator+=(const Big<exp,man> & ss2)
	{
		Add(ss2);

	return *this;
	}


	Big<exp,man> operator*(const Big<exp,man> & ss2) const
	{
	Big<exp,man> temp(*this);

		temp.Mul(ss2);

	return temp;
	}


	Big<exp,man> & operator*=(const Big<exp,man> & ss2)
	{
		Mul(ss2);

	return *this;
	}


	Big<exp,man> operator/(const Big<exp,man> & ss2) const
	{
	Big<exp,man> temp(*this);

		temp.Div(ss2);

	return temp;
	}


	Big<exp,man> & operator/=(const Big<exp,man> & ss2)
	{
		Div(ss2);

	return *this;
	}


	/*!
		Prefix operator e.g ++variable
	*/
	Big<exp,man> & operator++()
	{
		AddOne();

	return *this;
	}


	/*!
		Postfix operator e.g variable++
	*/
	Big<exp,man> operator++(int)
	{
	Big<exp,man> temp( *this );

		AddOne();

	return temp;
	}


	Big<exp,man> & operator--()
	{
		SubOne();

	return *this;
	}


	Big<exp,man> operator--(int)
	{
	Big<exp,man> temp( *this );

		SubOne();

	return temp;
	}



	/*!
	*
	*	bitwise operators
	*   (we do not define bitwise not)
	*/


	Big<exp,man> operator&(const Big<exp,man> & p2) const
	{
		Big<exp,man> temp( *this );

		temp.BitAnd(p2);

	return temp;
	}


	Big<exp,man> & operator&=(const Big<exp,man> & p2)
	{
		BitAnd(p2);

	return *this;
	}


	Big<exp,man> operator|(const Big<exp,man> & p2) const
	{
		Big<exp,man> temp( *this );

		temp.BitOr(p2);

	return temp;
	}


	Big<exp,man> & operator|=(const Big<exp,man> & p2)
	{
		BitOr(p2);

	return *this;
	}


	Big<exp,man> operator^(const Big<exp,man> & p2) const
	{
		Big<exp,man> temp( *this );

		temp.BitXor(p2);

	return temp;
	}


	Big<exp,man> & operator^=(const Big<exp,man> & p2)
	{
		BitXor(p2);

	return *this;
	}






	/*!
		this method makes an integer value by skipping any fractions

		for example:
			10.7 will be 10
			12.1  -- 12
			-20.2 -- 20
			-20.9 -- 20
			-0.7  -- 0
			0.8   -- 0
	*/
	void SkipFraction()
	{
		if( IsNan() || IsZero() )
			return;

		if( !exponent.IsSign() )
			// exponent >=0 -- the value don't have any fractions
			return;

		if( exponent <= -sint(man*TTMATH_BITS_PER_UINT) )
		{
			// the value is from (-1,1), we return zero
			SetZero();
			return;
		}

		// exponent is in range (-man*TTMATH_BITS_PER_UINT, 0)
		sint e = exponent.ToInt();
	
		mantissa.ClearFirstBits( -e );
		
		// we don't have to standardize 'Standardizing()' the value because
		// there's at least one bit in the mantissa
		// (the highest bit which we didn't touch)
	}


	/*!
		this method remains only a fraction from the value

		for example:
			30.56 will be 0.56
			-12.67 -- -0.67
	*/
	void RemainFraction()
	{
		if( IsNan() || IsZero() )
			return;

		if( !exponent.IsSign() )
		{
			// exponent >= 0 -- the value doesn't have any fractions
			// we return zero
			SetZero();
			return;
		}

		if( exponent <= -sint(man*TTMATH_BITS_PER_UINT) )
		{
			// the value is from (-1,1)
			// we don't make anything with the value
			return;
		}

		// e will be from (-man*TTMATH_BITS_PER_UINT, 0)
		sint e = exponent.ToInt();

		sint how_many_bits_leave = sint(man*TTMATH_BITS_PER_UINT) + e; // there'll be a subtraction -- e is negative
		mantissa.Rcl( how_many_bits_leave, 0);

		// there'll not be a carry because the exponent is too small
		exponent.Sub( how_many_bits_leave );

		// we must call Standardizing() here
		Standardizing();
	}



	/*!
		this method returns true if the value is integer
		(there is no a fraction)

		(we don't check nan)
	*/
	bool IsInteger() const
	{
		if( IsZero() )
			return true;

		if( !exponent.IsSign() )
			// exponent >=0 -- the value don't have any fractions
			return true;

		if( exponent <= -sint(man*TTMATH_BITS_PER_UINT) )
			// the value is from (-1,1)
			return false;

		// exponent is in range (-man*TTMATH_BITS_PER_UINT, 0)
		sint e = exponent.ToInt();
		e = -e; // e means how many bits we must check

		uint len  = e / TTMATH_BITS_PER_UINT;
		uint rest = e % TTMATH_BITS_PER_UINT;
		uint i    = 0;

		for( ; i<len ; ++i )
			if( mantissa.table[i] != 0 )
				return false;

		if( rest > 0 )
		{
			uint rest_mask = TTMATH_UINT_MAX_VALUE >> (TTMATH_BITS_PER_UINT - rest);
			if( (mantissa.table[i] & rest_mask) != 0 )
				return false;
		}

	return true;
	}


	/*!
		this method rounds to the nearest integer value
		(it returns a carry if it was)

		for example:
			2.3 = 2
			2.8 = 3

			-2.3 = -2
			-2.8 = 3
	*/
	uint Round()
	{
	Big<exp,man> half;
	uint c;

		if( IsNan() )
			return 1;

		if( IsZero() )
			return 0;

		half.Set05();

		if( IsSign() )
		{
			// 'this' is < 0
			c = Sub( half );
		}
		else
		{
			// 'this' is > 0
			c = Add( half );
		}

		SkipFraction();

	return CheckCarry(c);
	}

	

	/*!
	*
	*	input/output operators for standard streams
	*
	*/

private:

	/*!
		an auxiliary method for outputing to standard streams
	*/
	template<class ostream_type, class string_type>
	static ostream_type & OutputToStream(ostream_type & s, const Big<exp,man> & l)
	{
	string_type ss;

		l.ToString(ss);
		s << ss;

	return s;
	}


public:


	/*!
		output to standard streams
	*/
	friend std::ostream & operator<<(std::ostream & s,  const Big<exp,man> & l)
	{
		return OutputToStream<std::ostream, std::string>(s, l);
	}


#ifndef TTMATH_DONT_USE_WCHAR

	/*!
		output to standard streams
	*/
	friend std::wostream & operator<<(std::wostream & s,  const Big<exp,man> & l)
	{
		return OutputToStream<std::wostream, std::wstring>(s, l);
	}

#endif



private:

	/*!
		an auxiliary method for converting from a string
	*/
	template<class istream_type, class string_type, class char_type>
	static istream_type & InputFromStream(istream_type & s, Big<exp,man> & l)
	{
	string_type ss;
	
	// char or wchar_t for operator>>
	char_type z, old_z;
	bool was_comma = false;
	bool was_e     = false;
	

		// operator>> omits white characters if they're set for ommiting
		s >> z;

		if( z=='-' || z=='+' )
		{
			ss += z;
			s >> z; // we're reading a next character (white characters can be ommited)
		}
		
		old_z = 0;

		// we're reading only digits (base=10) and only one comma operator
		for( ; s.good() ; z=static_cast<char_type>(s.get()) )
		{
			if( z=='.' ||  z==',' )
			{
				if( was_comma || was_e )
					// second comma operator or comma operator after 'e' character
					break;

				was_comma = true;
			}
			else
			if( z == 'e' || z == 'E' )
			{
				if( was_e )
					// second 'e' character
					break;

				was_e = true;
			}
			else
			if( z == '+' || z == '-' )
			{
				if( old_z != 'e' && old_z != 'E' )
					// '+' or '-' is allowed only after 'e' character
					break;
			}
			else
			if( Misc::CharToDigit(z, 10) < 0 )
				break;


			ss   += z;
			old_z = z;
		}

		// we're leaving the last read character
		// (it's not belonging to the value)
		s.unget();

		l.FromString( ss );

	return s;
	}



public:

	/*!
		input from standard streams
	*/
	friend std::istream & operator>>(std::istream & s, Big<exp,man> & l)
	{
		return InputFromStream<std::istream, std::string, char>(s, l);
	}


#ifndef TTMATH_DONT_USE_WCHAR

	/*!
		input from standard streams
	*/
	friend std::wistream & operator>>(std::wistream & s, Big<exp,man> & l)
	{
		return InputFromStream<std::wistream, std::wstring, wchar_t>(s, l);
	}

#endif

};


} // namespace

#endif
