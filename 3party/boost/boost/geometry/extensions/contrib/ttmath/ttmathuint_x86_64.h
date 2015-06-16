/*
 * This file is a part of TTMath Bignum Library
 * and is distributed under the (new) BSD licence.
 * Author: Tomasz Sowa <t.sowa@ttmath.org>
 */

/* 
 * Copyright (c) 2006-2010, Tomasz Sowa
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


#ifndef headerfilettmathuint_x86_64
#define headerfilettmathuint_x86_64


#ifndef TTMATH_NOASM
#ifdef TTMATH_PLATFORM64


/*!
	\file ttmathuint_x86_64.h
    \brief template class UInt<uint> with assembler code for 64bit x86_64 processors

	this file is included at the end of ttmathuint.h
*/

#ifndef __GNUC__
#include <intrin.h>
#endif


namespace ttmath
{

	#ifndef __GNUC__

		extern "C"
			{
			uint __fastcall ttmath_adc_x64(uint* p1, const uint* p2, uint nSize, uint c);
			uint __fastcall ttmath_addindexed_x64(uint* p1, uint nSize, uint nPos, uint nValue);
			uint __fastcall ttmath_addindexed2_x64(uint* p1, uint nSize, uint nPos, uint nValue1, uint nValue2);
			uint __fastcall ttmath_addvector_x64(const uint * ss1, const uint * ss2, uint ss1_size, uint ss2_size, uint * result);
			uint __fastcall ttmath_sbb_x64(uint* p1, const uint* p2, uint nSize, uint c);
			uint __fastcall ttmath_subindexed_x64(uint* p1, uint nSize, uint nPos, uint nValue);
			uint __fastcall ttmath_subvector_x64(const uint * ss1, const uint * ss2, uint ss1_size, uint ss2_size, uint * result);
			uint __fastcall ttmath_rcl_x64(uint* p1, uint nSize, uint nLowestBit);
			uint __fastcall ttmath_rcr_x64(uint* p1, uint nSize, uint nLowestBit);
			uint __fastcall ttmath_div_x64(uint* pnValHi, uint* pnValLo, uint nDiv);
			uint __fastcall ttmath_rcl2_x64(uint* p1, uint nSize, uint nBits, uint c);
			uint __fastcall ttmath_rcr2_x64(uint* p1, uint nSize, uint nBits, uint c);
			};
	#endif


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
	template<uint value_size>
	const char * UInt<value_size>::LibTypeStr()
	{
		#ifndef __GNUC__
			static const char info[] = "asm_vc_64";
		#endif		

		#ifdef __GNUC__
			static const char info[] = "asm_gcc_64";
		#endif

	return info;
	}


	/*!
		returning the currect type of the library
	*/
	template<uint value_size>
	LibTypeCode UInt<value_size>::LibType()
	{
		#ifndef __GNUC__
			LibTypeCode info = asm_vc_64;
		#endif		

		#ifdef __GNUC__
			LibTypeCode info = asm_gcc_64;
		#endif

	return info;
	}


	/*!
	*
	*	basic mathematic functions
	*
	*/



	/*!
		this method adding ss2 to the this and adding carry if it's defined
		(this = this + ss2 + c)

		***this method is created only on a 64bit platform***

		c must be zero or one (might be a bigger value than 1)
		function returns carry (1) (if it was)
	*/
	template<uint value_size>
	uint UInt<value_size>::Add(const UInt<value_size> & ss2, uint c)
	{
	uint b = value_size;
	uint * p1 = table;
	const uint * p2 = ss2.table;

		// we don't have to use TTMATH_REFERENCE_ASSERT here
		// this algorithm doesn't require it

		#ifndef __GNUC__
			c = ttmath_adc_x64(p1,p2,b,c);
		#endif

		#ifdef __GNUC__
		uint dummy, dummy2;

			/*
				this part should be compiled with gcc
			*/
			__asm__ __volatile__(
	
				"xorq %%rdx, %%rdx				\n"
				"negq %%rax						\n"     // CF=1 if rax!=0 , CF=0 if rax==0

			"1:									\n"
				"movq (%%rsi,%%rdx,8), %%rax	\n"
				"adcq %%rax, (%%rbx,%%rdx,8)	\n"
			
				"incq %%rdx						\n"
				"decq %%rcx						\n"
			"jnz 1b								\n"

				"adcq %%rcx, %%rcx				\n"

				: "=c" (c), "=a" (dummy), "=d" (dummy2)
				: "0" (b),  "1" (c), "b" (p1), "S" (p2)
				: "cc", "memory" );

		#endif

		TTMATH_LOGC("UInt::Add", c)
	
	return c;
	}



	/*!
		this method adds one word (at a specific position)
		and returns a carry (if it was)

		***this method is created only on a 64bit platform***


		if we've got (value_size=3):
			table[0] = 10;
			table[1] = 30;
			table[2] = 5;	
		and we call:
			AddInt(2,1)
		then it'll be:
			table[0] = 10;
			table[1] = 30 + 2;
			table[2] = 5;

		of course if there was a carry from table[2] it would be returned
	*/
	template<uint value_size>
	uint UInt<value_size>::AddInt(uint value, uint index)
	{
	uint b = value_size;
	uint * p1 = table;
	uint c;

		TTMATH_ASSERT( index < value_size )

		#ifndef __GNUC__
			c = ttmath_addindexed_x64(p1,b,index,value);
		#endif


		#ifdef __GNUC__
		uint dummy, dummy2;
	
			__asm__ __volatile__(

				"subq %%rdx, %%rcx 				\n"

			"1:									\n"
				"addq %%rax, (%%rbx,%%rdx,8)	\n"
			"jnc 2f								\n"
				
				"movq $1, %%rax					\n"
				"incq %%rdx						\n"
				"decq %%rcx						\n"
			"jnz 1b								\n"

			"2:									\n"
				"setc %%al						\n"
				"movzx %%al, %%rdx				\n"

				: "=d" (c),    "=a" (dummy), "=c" (dummy2)
				: "0" (index), "1" (value),  "2" (b), "b" (p1)
				: "cc", "memory" );

		#endif

		TTMATH_LOGC("UInt::AddInt", c)
	
	return c;
	}



	/*!
		this method adds only two unsigned words to the existing value
		and these words begin on the 'index' position
		(it's used in the multiplication algorithm 2)

		***this method is created only on a 64bit platform***

		index should be equal or smaller than value_size-2 (index <= value_size-2)
		x1 - lower word, x2 - higher word

		for example if we've got value_size equal 4 and:
			table[0] = 3
			table[1] = 4
			table[2] = 5
			table[3] = 6
		then let
			x1 = 10
			x2 = 20
		and
			index = 1

		the result of this method will be:
			table[0] = 3
			table[1] = 4 + x1 = 14
			table[2] = 5 + x2 = 25
			table[3] = 6
		
		and no carry at the end of table[3]

		(of course if there was a carry in table[2](5+20) then 
		this carry would be passed to the table[3] etc.)
	*/
	template<uint value_size>
	uint UInt<value_size>::AddTwoInts(uint x2, uint x1, uint index)
	{
	uint b = value_size;
	uint * p1 = table;
	uint c;

		TTMATH_ASSERT( index < value_size - 1 )

		#ifndef __GNUC__
			c = ttmath_addindexed2_x64(p1,b,index,x1,x2);
		#endif


		#ifdef __GNUC__
		uint dummy, dummy2;

			__asm__ __volatile__(
			
				"subq %%rdx, %%rcx 				\n"
				
				"addq %%rsi, (%%rbx,%%rdx,8) 	\n"
				"incq %%rdx						\n"
				"decq %%rcx						\n"

			"1:									\n"
				"adcq %%rax, (%%rbx,%%rdx,8)	\n"
			"jnc 2f								\n"

				"mov $0, %%rax					\n"
				"incq %%rdx						\n"
				"decq %%rcx						\n"
			"jnz 1b								\n"

			"2:									\n"
				"setc %%al						\n"
				"movzx %%al, %%rax				\n"

				: "=a" (c), "=c" (dummy), "=d" (dummy2)
				: "0" (x2), "1" (b),      "2" (index), "b" (p1), "S" (x1)
				: "cc", "memory" );

		#endif

		TTMATH_LOGC("UInt::AddTwoInts", c)

	return c;
	}



	/*!
		this static method addes one vector to the other
		'ss1' is larger in size or equal to 'ss2'

		ss1 points to the first (larger) vector
		ss2 points to the second vector
		ss1_size - size of the ss1 (and size of the result too)
		ss2_size - size of the ss2
		result - is the result vector (which has size the same as ss1: ss1_size)

		Example:  ss1_size is 5, ss2_size is 3
		ss1:      ss2:   result (output):
		  5        1         5+1
		  4        3         4+3
		  2        7         2+7
		  6                  6
		  9                  9
	  of course the carry is propagated and will be returned from the last item
	  (this method is used by the Karatsuba multiplication algorithm)
	*/
	template<uint value_size>
	uint UInt<value_size>::AddVector(const uint * ss1, const uint * ss2, uint ss1_size, uint ss2_size, uint * result)
	{
		TTMATH_ASSERT( ss1_size >= ss2_size )

		uint c;

		#ifndef __GNUC__
			 c = ttmath_addvector_x64(ss1, ss2, ss1_size, ss2_size, result);
		#endif


		#ifdef __GNUC__
		uint dummy1, dummy2, dummy3;	
		uint rest = ss1_size - ss2_size;
			
			//	this part should be compiled with gcc
		
			__asm__ __volatile__(
				"mov %%rdx, %%r8					\n"
				"xor %%rdx, %%rdx					\n"   // rdx = 0, cf = 0
			"1:										\n"
				"mov (%%rsi,%%rdx,8), %%rax			\n"
				"adc (%%rbx,%%rdx,8), %%rax			\n"
				"mov %%rax, (%%rdi,%%rdx,8)			\n"

				"inc %%rdx							\n"
				"dec %%rcx							\n"
			"jnz 1b									\n"

				"adc %%rcx, %%rcx					\n"   // rcx has the cf state

				"or %%r8, %%r8						\n"
				"jz 3f								\n"
				
				"xor %%rbx, %%rbx					\n"   // ebx = 0
				"neg %%rcx							\n"   // setting cf from rcx
				"mov %%r8, %%rcx					\n"   // rcx=rest and is != 0
			"2:										\n"
				"mov (%%rsi, %%rdx, 8), %%rax		\n"
				"adc %%rbx, %%rax 					\n"
				"mov %%rax, (%%rdi, %%rdx, 8)		\n"

				"inc %%rdx							\n"
				"dec %%rcx							\n"
			"jnz 2b									\n"

				"adc %%rcx, %%rcx					\n"
			"3:										\n"

				: "=a" (dummy1), "=b" (dummy2), "=c" (c),       "=d" (dummy3)
				:                "1" (ss2),     "2" (ss2_size), "3" (rest),   "S" (ss1),  "D" (result)
				: "%r8", "cc", "memory" );

		#endif

		TTMATH_VECTOR_LOGC("UInt::AddVector", c, result, ss1_size)

	return c;
	}



	/*!
		this method's subtracting ss2 from the 'this' and subtracting
		carry if it has been defined
		(this = this - ss2 - c)

		***this method is created only on a 64bit platform***

		c must be zero or one (might be a bigger value than 1)
		function returns carry (1) (if it was)
	*/
	template<uint value_size>
	uint UInt<value_size>::Sub(const UInt<value_size> & ss2, uint c)
	{
	uint b = value_size;
	uint * p1 = table;
	const uint * p2 = ss2.table;
	
		// we don't have to use TTMATH_REFERENCE_ASSERT here
		// this algorithm doesn't require it

		#ifndef __GNUC__
			c = ttmath_sbb_x64(p1,p2,b,c);
		#endif


		#ifdef __GNUC__
		uint dummy, dummy2;

			__asm__  __volatile__(
	
				"xorq %%rdx, %%rdx				\n"
				"negq %%rax						\n"     // CF=1 if rax!=0 , CF=0 if rax==0

			"1:									\n"
				"movq (%%rsi,%%rdx,8), %%rax	\n"
				"sbbq %%rax, (%%rbx,%%rdx,8)	\n"
			
				"incq %%rdx						\n"
				"decq %%rcx						\n"
			"jnz 1b								\n"

				"adcq %%rcx, %%rcx				\n"

				: "=c" (c), "=a" (dummy), "=d" (dummy2)
				: "0" (b),  "1" (c), "b" (p1), "S" (p2)
				: "cc", "memory" );

		#endif

		TTMATH_LOGC("UInt::Sub", c)

	return c;
	}



	/*!
		this method subtracts one word (at a specific position)
		and returns a carry (if it was)

		***this method is created only on a 64bit platform***

		if we've got (value_size=3):
			table[0] = 10;
			table[1] = 30;
			table[2] = 5;	
		and we call:
			SubInt(2,1)
		then it'll be:
			table[0] = 10;
			table[1] = 30 - 2;
			table[2] = 5;

		of course if there was a carry from table[2] it would be returned
	*/
	template<uint value_size>
	uint UInt<value_size>::SubInt(uint value, uint index)
	{
	uint b = value_size;
	uint * p1 = table;
	uint c;

		TTMATH_ASSERT( index < value_size )

		#ifndef __GNUC__
			c = ttmath_subindexed_x64(p1,b,index,value);
		#endif


		#ifdef __GNUC__
			uint dummy, dummy2;

			__asm__ __volatile__(
			
				"subq %%rdx, %%rcx 				\n"

			"1:									\n"
				"subq %%rax, (%%rbx,%%rdx,8)	\n"
			"jnc 2f								\n"
				
				"movq $1, %%rax					\n"
				"incq %%rdx						\n"
				"decq %%rcx						\n"
			"jnz 1b								\n"

			"2:									\n"
				"setc %%al						\n"
				"movzx %%al, %%rdx				\n"

				: "=d" (c),    "=a" (dummy), "=c" (dummy2)
				: "0" (index), "1" (value),  "2" (b), "b" (p1)
				: "cc", "memory" );

		#endif

		TTMATH_LOGC("UInt::SubInt", c)

	return c;
	}


	/*!
		this static method subtractes one vector from the other
		'ss1' is larger in size or equal to 'ss2'

		ss1 points to the first (larger) vector
		ss2 points to the second vector
		ss1_size - size of the ss1 (and size of the result too)
		ss2_size - size of the ss2
		result - is the result vector (which has size the same as ss1: ss1_size)

		Example:  ss1_size is 5, ss2_size is 3
		ss1:      ss2:   result (output):
		  5        1         5-1
		  4        3         4-3
		  2        7         2-7
		  6                  6-1  (the borrow from previous item)
		  9                  9
		               return (carry): 0
	  of course the carry (borrow) is propagated and will be returned from the last item
	  (this method is used by the Karatsuba multiplication algorithm)
	*/
	template<uint value_size>
	uint UInt<value_size>::SubVector(const uint * ss1, const uint * ss2, uint ss1_size, uint ss2_size, uint * result)
	{
		TTMATH_ASSERT( ss1_size >= ss2_size )

		uint c;

		#ifndef __GNUC__
			c = ttmath_subvector_x64(ss1, ss2, ss1_size, ss2_size, result);
		#endif


		#ifdef __GNUC__
		
		//	the asm code is nearly the same as in AddVector
		//	only two instructions 'adc' are changed to 'sbb'
		
		uint dummy1, dummy2, dummy3;
		uint rest = ss1_size - ss2_size;

			__asm__ __volatile__(
				"mov %%rdx, %%r8					\n"
				"xor %%rdx, %%rdx					\n"   // rdx = 0, cf = 0
			"1:										\n"
				"mov (%%rsi,%%rdx,8), %%rax			\n"
				"sbb (%%rbx,%%rdx,8), %%rax			\n"
				"mov %%rax, (%%rdi,%%rdx,8)			\n"

				"inc %%rdx							\n"
				"dec %%rcx							\n"
			"jnz 1b									\n"

				"adc %%rcx, %%rcx					\n"   // rcx has the cf state

				"or %%r8, %%r8						\n"
				"jz 3f								\n"
				
				"xor %%rbx, %%rbx					\n"   // ebx = 0
				"neg %%rcx							\n"   // setting cf from rcx
				"mov %%r8, %%rcx					\n"   // rcx=rest and is != 0
			"2:										\n"
				"mov (%%rsi, %%rdx, 8), %%rax		\n"
				"sbb %%rbx, %%rax 					\n"
				"mov %%rax, (%%rdi, %%rdx, 8)		\n"

				"inc %%rdx							\n"
				"dec %%rcx							\n"
			"jnz 2b									\n"

				"adc %%rcx, %%rcx					\n"
			"3:										\n"

				: "=a" (dummy1), "=b" (dummy2), "=c" (c),       "=d" (dummy3)
				:                "1" (ss2),     "2" (ss2_size), "3" (rest),   "S" (ss1),  "D" (result)
				: "%r8", "cc", "memory" );

		#endif

		TTMATH_VECTOR_LOGC("UInt::SubVector", c, result, ss1_size)

	return c;
	}


	/*!
		this method moves all bits into the left hand side
		return value <- this <- c

		the lowest *bit* will be held the 'c' and
		the state of one additional bit (on the left hand side)
		will be returned

		for example:
		let this is 001010000
		after Rcl2_one(1) there'll be 010100001 and Rcl2_one returns 0
	
		***this method is created only on a 64bit platform***
	*/
	template<uint value_size>
	uint UInt<value_size>::Rcl2_one(uint c)
	{
	sint b = value_size;
	uint * p1 = table;


		#ifndef __GNUC__
			c = ttmath_rcl_x64(p1,b,c);
		#endif


		#ifdef __GNUC__
		uint dummy, dummy2;

		__asm__  __volatile__(
		
			"xorq %%rdx, %%rdx			\n"   // rdx=0
			"negq %%rax					\n"   // CF=1 if rax!=0 , CF=0 if rax==0

		"1:								\n"
			"rclq $1, (%%rbx, %%rdx, 8)	\n"

			"incq %%rdx					\n"
			"decq %%rcx					\n"
		"jnz 1b							\n"

			"adcq %%rcx, %%rcx			\n"

			: "=c" (c), "=a" (dummy), "=d" (dummy2)
			: "0" (b),  "1" (c), "b" (p1)
			: "cc", "memory" );
	
		#endif

		TTMATH_LOGC("UInt::Rcl2_one", c)

	return c;
	}


	/*!
		this method moves all bits into the right hand side
		c -> this -> return value

		the highest *bit* will be held the 'c' and
		the state of one additional bit (on the right hand side)
		will be returned

		for example:
		let this is 000000010
		after Rcr2_one(1) there'll be 100000001 and Rcr2_one returns 0

		***this method is created only on a 64bit platform***
	*/
	template<uint value_size>
	uint UInt<value_size>::Rcr2_one(uint c)
	{
	sint b = value_size;
	uint * p1 = table;
	

		#ifndef __GNUC__
			c = ttmath_rcr_x64(p1,b,c);
		#endif


		#ifdef __GNUC__
		uint dummy;

		__asm__  __volatile__(

			"negq %%rax						\n"   // CF=1 if rax!=0 , CF=0 if rax==0

		"1:									\n"
			"rcrq $1, -8(%%rbx, %%rcx, 8)	\n"

			"decq %%rcx						\n"
		"jnz 1b								\n"

			"adcq %%rcx, %%rcx				\n"

			: "=c" (c), "=a" (dummy)
			: "0" (b),  "1" (c), "b" (p1)
			: "cc", "memory" );

		#endif

		TTMATH_LOGC("UInt::Rcr2_one", c)

	return c;
	}



	/*!
		this method moves all bits into the left hand side
		return value <- this <- c

		the lowest *bits* will be held the 'c' and
		the state of one additional bit (on the left hand side)
		will be returned

		for example:
		let this is 001010000
		after Rcl2(3, 1) there'll be 010000111 and Rcl2 returns 1
	
		***this method is created only on a 64bit platform***
	*/
	template<uint value_size>
	uint UInt<value_size>::Rcl2(uint bits, uint c)
	{
	TTMATH_ASSERT( bits>0 && bits<TTMATH_BITS_PER_UINT )

	uint b = value_size;
	uint * p1 = table;


		#ifndef __GNUC__
			c = ttmath_rcl2_x64(p1,b,bits,c);
		#endif


		#ifdef __GNUC__
		uint dummy, dummy2, dummy3;

		__asm__  __volatile__(
		
			"movq %%rcx, %%rsi				\n"
			"movq $64, %%rcx				\n"
			"subq %%rsi, %%rcx				\n"
			"movq $-1, %%rdx				\n"
			"shrq %%cl, %%rdx				\n"
			"movq %%rdx, %%r8 				\n"
			"movq %%rsi, %%rcx				\n"

			"xorq %%rdx, %%rdx				\n"
			"movq %%rdx, %%rsi				\n"
			"orq %%rax, %%rax				\n"
			"cmovnz %%r8, %%rsi				\n"

		"1:									\n"
			"rolq %%cl, (%%rbx,%%rdx,8)		\n"

			"movq (%%rbx,%%rdx,8), %%rax	\n"
			"andq %%r8, %%rax				\n"
			"xorq %%rax, (%%rbx,%%rdx,8)	\n"
			"orq  %%rsi, (%%rbx,%%rdx,8)	\n"
			"movq %%rax, %%rsi				\n"
			
			"incq %%rdx						\n"
			"decq %%rdi						\n"
		"jnz 1b								\n"
			
			"and $1, %%rax					\n"

			: "=a" (c), "=D" (dummy), "=S" (dummy2), "=d" (dummy3)
			: "0" (c),  "1" (b), "b" (p1), "c" (bits)
			: "%r8", "cc", "memory" );

		#endif

		TTMATH_LOGC("UInt::Rcl2", c)

	return c;
	}


	/*!
		this method moves all bits into the right hand side
		C -> this -> return value

		the highest *bits* will be held the 'c' and
		the state of one additional bit (on the right hand side)
		will be returned

		for example:
		let this is 000000010
		after Rcr2(2, 1) there'll be 110000000 and Rcr2 returns 1

		***this method is created only on a 64bit platform***
	*/
	template<uint value_size>
	uint UInt<value_size>::Rcr2(uint bits, uint c)
	{
	TTMATH_ASSERT( bits>0 && bits<TTMATH_BITS_PER_UINT )

	sint b = value_size;
	uint * p1 = table;


		#ifndef __GNUC__
			c = ttmath_rcr2_x64(p1,b,bits,c);
		#endif


		#ifdef __GNUC__
			uint dummy, dummy2, dummy3;

			__asm__  __volatile__(

			"movq %%rcx, %%rsi				\n"
			"movq $64, %%rcx				\n"
			"subq %%rsi, %%rcx				\n"
			"movq $-1, %%rdx				\n"
			"shlq %%cl, %%rdx				\n"
			"movq %%rdx, %%R8				\n"
			"movq %%rsi, %%rcx				\n"

			"xorq %%rdx, %%rdx				\n"
			"movq %%rdx, %%rsi				\n"
			"addq %%rdi, %%rdx				\n"
			"decq %%rdx						\n"
			"orq %%rax, %%rax				\n"
			"cmovnz %%R8, %%rsi				\n"

		"1:									\n"
			"rorq %%cl, (%%rbx,%%rdx,8)		\n"

			"movq (%%rbx,%%rdx,8), %%rax	\n"
			"andq %%R8, %%rax				\n"
			"xorq %%rax, (%%rbx,%%rdx,8)	\n"
			"orq  %%rsi, (%%rbx,%%rdx,8)	\n"
			"movq %%rax, %%rsi				\n"
			
			"decq %%rdx						\n"
			"decq %%rdi						\n"
		"jnz 1b								\n"
			
			"rolq $1, %%rax					\n"
			"andq $1, %%rax					\n"

			: "=a" (c), "=D" (dummy), "=S" (dummy2), "=d" (dummy3)
			: "0" (c), "1" (b), "b" (p1), "c" (bits)
			: "%r8", "cc", "memory" );

		#endif

		TTMATH_LOGC("UInt::Rcr2", c)

	return c;
	}


	/*
		this method returns the number of the highest set bit in one 64-bit word
		if the 'x' is zero this method returns '-1'

		***this method is created only on a 64bit platform***
	*/
	template<uint value_size>
	sint UInt<value_size>::FindLeadingBitInWord(uint x)
	{
	sint result;

	
		#ifndef __GNUC__

			unsigned long nIndex = 0;

			if( _BitScanReverse64(&nIndex,x) == 0 )
				result = -1;
			else
				result = nIndex;

		#endif


		#ifdef __GNUC__
		uint dummy;

				__asm__ (

				"movq $-1, %1          \n"
				"bsrq %2, %0           \n"
				"cmovz %1, %0          \n"

				: "=r" (result), "=&r" (dummy)
				: "r" (x)
				: "cc" );

		#endif


	return result;
	}


	/*
		this method returns the number of the highest set bit in one 64-bit word
		if the 'x' is zero this method returns '-1'

		***this method is created only on a 64bit platform***
	*/
	template<uint value_size>
	sint UInt<value_size>::FindLowestBitInWord(uint x)
	{
	sint result;

	
		#ifndef __GNUC__

			unsigned long nIndex = 0;

			if( _BitScanForward64(&nIndex,x) == 0 )
				result = -1;
			else
				result = nIndex;

		#endif


		#ifdef __GNUC__
		uint dummy;

				__asm__ (

				"movq $-1, %1          \n"
				"bsfq %2, %0           \n"
				"cmovz %1, %0          \n"

				: "=r" (result), "=&r" (dummy)
				: "r" (x)
				: "cc" );

		#endif


	return result;
	}


	/*!
		this method sets a special bit in the 'value'
		and returns the last state of the bit (zero or one)

		***this method is created only on a 64bit platform***

		bit is from <0,63>

		e.g.
		 uint x = 100;
		 uint bit = SetBitInWord(x, 3);
		 now: x = 108 and bit = 0
	*/
	template<uint value_size>
	uint UInt<value_size>::SetBitInWord(uint & value, uint bit)
	{
		TTMATH_ASSERT( bit < TTMATH_BITS_PER_UINT )
		
		uint old_bit;
		uint v = value;


		#ifndef __GNUC__
			old_bit = _bittestandset64((__int64*)&value,bit) != 0;
		#endif


		#ifdef __GNUC__

			__asm__ (

			"btsq %%rbx, %%rax		\n"
			"setc %%bl				\n"
			"movzx %%bl, %%rbx		\n"
			
			: "=a" (v), "=b" (old_bit)
			: "0" (v),  "1" (bit)
			: "cc" );

		#endif

		value = v;

	return old_bit;
	}


	/*!
	 *
	 * Multiplication
	 *
	 *
	*/


	/*!
		multiplication: result_high:result_low = a * b
		result_high - higher word of the result
		result_low  - lower word of the result
	
		this methos never returns a carry
		this method is used in the second version of the multiplication algorithms

		***this method is created only on a 64bit platform***
	*/
	template<uint value_size>
	void UInt<value_size>::MulTwoWords(uint a, uint b, uint * result_high, uint * result_low)
	{
	/*
		we must use these temporary variables in order to inform the compilator
		that value pointed with result1 and result2 has changed

		this has no effect in visual studio but it's usefull when
		using gcc and options like -O
	*/
	uint result1_;
	uint result2_;


		#ifndef __GNUC__
			result1_ = _umul128(a,b,&result2_);
		#endif


		#ifdef __GNUC__

		__asm__ (
		
			"mulq %%rdx			\n"

			: "=a" (result1_), "=d" (result2_)
			: "0" (a),         "1" (b)
			: "cc" );

		#endif


		*result_low  = result1_;
		*result_high = result2_;
	}




	/*!
	 *
	 * Division
	 *
	 *
	*/
	

	/*!
		this method calculates 64bits word a:b / 32bits c (a higher, b lower word)
		r = a:b / c and rest - remainder
		
		***this method is created only on a 64bit platform***

		*
		* WARNING:
		* if r (one word) is too small for the result or c is equal zero
		* there'll be a hardware interruption (0)
		* and probably the end of your program
		*
	*/
	template<uint value_size>
	void UInt<value_size>::DivTwoWords(uint a,uint b, uint c, uint * r, uint * rest)
	{
		uint r_;
		uint rest_;
		/*
			these variables have similar meaning like those in
			the multiplication algorithm MulTwoWords
		*/

		TTMATH_ASSERT( c != 0 )


		#ifndef __GNUC__

			ttmath_div_x64(&a,&b,c);
			r_    = a;
			rest_ = b;
			
		#endif


		#ifdef __GNUC__
		
			__asm__ (

			"divq %%rcx				\n"

			: "=a" (r_), "=d" (rest_)
			: "d" (a), "a" (b), "c" (c)
			: "cc" );

		#endif


		*r = r_;
		*rest = rest_;
	}

} //namespace


#endif //ifdef TTMATH_PLATFORM64
#endif //ifndef TTMATH_NOASM
#endif


