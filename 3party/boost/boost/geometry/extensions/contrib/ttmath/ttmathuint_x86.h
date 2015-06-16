/*
 * This file is a part of TTMath Bignum Library
 * and is distributed under the (new) BSD licence.
 * Author: Tomasz Sowa <t.sowa@ttmath.org>
 */

/* 
 * Copyright (c) 2006-2009, Tomasz Sowa
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



#ifndef headerfilettmathuint_x86
#define headerfilettmathuint_x86


#ifndef TTMATH_NOASM
#ifdef TTMATH_PLATFORM32


/*!
	\file ttmathuint_x86.h
    \brief template class UInt<uint> with assembler code for 32bit x86 processors

	this file is included at the end of ttmathuint.h
*/



/*!
    \brief a namespace for the TTMath library
*/
namespace ttmath
{

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
			static const char info[] = "asm_vc_32";
		#endif		

		#ifdef __GNUC__
			static const char info[] = "asm_gcc_32";
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
			LibTypeCode info = asm_vc_32;
		#endif		

		#ifdef __GNUC__
			LibTypeCode info = asm_gcc_32;
		#endif

	return info;
	}



	/*!
	*
	*	basic mathematic functions
	*
	*/


	/*!
		adding ss2 to the this and adding carry if it's defined
		(this = this + ss2 + c)

		c must be zero or one (might be a bigger value than 1)
		function returns carry (1) (if it has been)
	*/
	template<uint value_size>
	uint UInt<value_size>::Add(const UInt<value_size> & ss2, uint c)
	{
	uint b = value_size;
	uint * p1 = table;
	uint * p2 = const_cast<uint*>(ss2.table);

		// we don't have to use TTMATH_REFERENCE_ASSERT here
		// this algorithm doesn't require it

		#ifndef __GNUC__
			
			//	this part might be compiled with for example visual c

			__asm
			{
				push eax
				push ebx
				push ecx
				push edx
				push esi

				mov ecx,[b]
				
				mov ebx,[p1]
				mov esi,[p2]

				xor edx,edx          // edx=0
				mov eax,[c]
				neg eax              // CF=1 if rax!=0 , CF=0 if rax==0

			ttmath_loop:
				mov eax,[esi+edx*4]
				adc [ebx+edx*4],eax

				inc edx
				dec ecx
			jnz ttmath_loop

				adc ecx, ecx
				mov [c], ecx

				pop esi
				pop edx
				pop ecx
				pop ebx
				pop eax
			}



		#endif		
			

		#ifdef __GNUC__
		uint dummy, dummy2;
			//	this part should be compiled with gcc
			
			__asm__ __volatile__(

				"xorl %%edx, %%edx				\n"
				"negl %%eax						\n"  // CF=1 if rax!=0 , CF=0 if rax==0

			"1:									\n"
				"movl (%%esi,%%edx,4), %%eax	\n"
				"adcl %%eax, (%%ebx,%%edx,4)	\n"
			
				"incl %%edx						\n"
				"decl %%ecx						\n"
			"jnz 1b								\n"

				"adc %%ecx, %%ecx				\n"

				: "=c" (c), "=a" (dummy), "=d" (dummy2)
				: "0" (b),  "1" (c), "b" (p1), "S" (p2)
				: "cc", "memory" );
		#endif

		TTMATH_LOGC("UInt::Add", c)

	return c;
	}



	/*!
		adding one word (at a specific position)
		and returning a carry (if it has been)

		e.g.

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

			__asm
			{
				push eax
				push ebx
				push ecx
				push edx

				mov ecx, [b]
				sub ecx, [index]				

				mov edx, [index]
				mov ebx, [p1]

				mov eax, [value]

			ttmath_loop:
				add [ebx+edx*4], eax
			jnc ttmath_end

				mov eax, 1
				inc edx
				dec ecx
			jnz ttmath_loop

			ttmath_end:
				setc al
				movzx edx, al
				mov [c], edx

				pop edx
				pop ecx
				pop ebx
				pop eax
			}

		#endif		
			

		#ifdef __GNUC__
		uint dummy, dummy2;

			__asm__ __volatile__(
			
				"subl %%edx, %%ecx 				\n"

			"1:									\n"
				"addl %%eax, (%%ebx,%%edx,4)	\n"
			"jnc 2f								\n"
				
				"movl $1, %%eax					\n"
				"incl %%edx						\n"
				"decl %%ecx						\n"
			"jnz 1b								\n"

			"2:									\n"
				"setc %%al						\n"
				"movzx %%al, %%edx				\n"

				: "=d" (c),    "=a" (dummy), "=c" (dummy2)
				: "0" (index), "1" (value),  "2" (b), "b" (p1)
				: "cc", "memory" );

		#endif
	
		TTMATH_LOGC("UInt::AddInt", c)

	return c;
	}




	/*!
		adding only two unsigned words to the existing value
		and these words begin on the 'index' position
		(it's used in the multiplication algorithm 2)

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
			__asm
			{
				push eax
				push ebx
				push ecx
				push edx

				mov ecx, [b]
				sub ecx, [index]				

				mov ebx, [p1]
				mov edx, [index]

				mov eax, [x1]
				add [ebx+edx*4], eax
				inc edx
				dec ecx

				mov eax, [x2]
			
			ttmath_loop:
				adc [ebx+edx*4], eax
			jnc ttmath_end

				mov eax, 0
				inc edx
				dec ecx
			jnz ttmath_loop

			ttmath_end:
				setc al
				movzx edx, al
				mov [c], edx
				
				pop edx
				pop ecx
				pop ebx
				pop eax

			}
		#endif		
			

		#ifdef __GNUC__
		uint dummy, dummy2;

			__asm__ __volatile__(
			
				"subl %%edx, %%ecx 				\n"
				
				"addl %%esi, (%%ebx,%%edx,4) 	\n"
				"incl %%edx						\n"
				"decl %%ecx						\n"

			"1:									\n"
				"adcl %%eax, (%%ebx,%%edx,4)	\n"
			"jnc 2f								\n"

				"mov $0, %%eax					\n"
				"incl %%edx						\n"
				"decl %%ecx						\n"
			"jnz 1b								\n"

			"2:									\n"
				"setc %%al						\n"
				"movzx %%al, %%eax				\n"

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

		uint rest = ss1_size - ss2_size;
		uint c;

		#ifndef __GNUC__

			//	this part might be compiled with for example visual c
			__asm
			{
				pushad

				mov ecx, [ss2_size]
				xor edx, edx               // edx = 0, cf = 0

				mov esi, [ss1]
				mov ebx, [ss2]
				mov edi, [result]

			ttmath_loop:
				mov eax, [esi+edx*4]
				adc eax, [ebx+edx*4]
				mov [edi+edx*4], eax

				inc edx
				dec ecx
			jnz ttmath_loop

				adc ecx, ecx             // ecx has the cf state

				mov ebx, [rest]
				or ebx, ebx
				jz ttmath_end
				
				xor ebx, ebx             // ebx = 0
				neg ecx                  // setting cf from ecx
				mov ecx, [rest]          // ecx is != 0
			
			ttmath_loop2:
				mov eax, [esi+edx*4]
				adc eax, ebx 
				mov [edi+edx*4], eax

				inc edx
				dec ecx
			jnz ttmath_loop2

				adc ecx, ecx

			ttmath_end:
				mov [c], ecx

				popad
			}

		#endif		
			

		#ifdef __GNUC__
			
		//	this part should be compiled with gcc
		uint dummy1, dummy2, dummy3;

			__asm__ __volatile__(
				"push %%edx							\n"
				"xor %%edx, %%edx					\n"   // edx = 0, cf = 0
			"1:										\n"
				"mov (%%esi,%%edx,4), %%eax			\n"
				"adc (%%ebx,%%edx,4), %%eax			\n"
				"mov %%eax, (%%edi,%%edx,4)			\n"

				"inc %%edx							\n"
				"dec %%ecx							\n"
			"jnz 1b									\n"

				"adc %%ecx, %%ecx					\n"   // ecx has the cf state
				"pop %%eax							\n"   // eax = rest

				"or %%eax, %%eax					\n"
				"jz 3f								\n"
				
				"xor %%ebx, %%ebx					\n"   // ebx = 0
				"neg %%ecx							\n"   // setting cf from ecx
				"mov %%eax, %%ecx					\n"   // ecx=rest and is != 0
			"2:										\n"
				"mov (%%esi, %%edx, 4), %%eax		\n"
				"adc %%ebx, %%eax 					\n"
				"mov %%eax, (%%edi, %%edx, 4)		\n"

				"inc %%edx							\n"
				"dec %%ecx							\n"
			"jnz 2b									\n"

				"adc %%ecx, %%ecx					\n"
			"3:										\n"

				: "=a" (dummy1), "=b" (dummy2), "=c" (c),       "=d" (dummy3)
				:                "1" (ss2),     "2" (ss2_size), "3" (rest),   "S" (ss1),  "D" (result)
				: "cc", "memory" );

		#endif

		TTMATH_VECTOR_LOGC("UInt::AddVector", c, result, ss1_size)

	return c;
	}


	/*!
		subtracting ss2 from the 'this' and subtracting
		carry if it has been defined
		(this = this - ss2 - c)

		c must be zero or one (might be a bigger value than 1)
		function returns carry (1) (if it has been)
	*/
	template<uint value_size>
	uint UInt<value_size>::Sub(const UInt<value_size> & ss2, uint c)
	{
	uint b = value_size;
	uint * p1 = table;
	uint * p2 = const_cast<uint*>(ss2.table);

		// we don't have to use TTMATH_REFERENCE_ASSERT here
		// this algorithm doesn't require it

		#ifndef __GNUC__

			__asm
			{
				push eax
				push ebx
				push ecx
				push edx
				push esi

				mov ecx,[b]
				
				mov ebx,[p1]
				mov esi,[p2]

				xor edx,edx          // edx=0
				mov eax,[c]
				neg eax              // CF=1 if rax!=0 , CF=0 if rax==0

			ttmath_loop:
				mov eax,[esi+edx*4]
				sbb [ebx+edx*4],eax

				inc edx
				dec ecx
			jnz ttmath_loop

				adc ecx, ecx
				mov [c], ecx

				pop esi
				pop edx
				pop ecx
				pop ebx
				pop eax
			}

		#endif


		#ifdef __GNUC__
		uint dummy, dummy2;

			__asm__  __volatile__(

				"xorl %%edx, %%edx				\n"
				"negl %%eax						\n"  // CF=1 if rax!=0 , CF=0 if rax==0

			"1:									\n"
				"movl (%%esi,%%edx,4), %%eax	\n"
				"sbbl %%eax, (%%ebx,%%edx,4)	\n"
			
				"incl %%edx						\n"
				"decl %%ecx						\n"
			"jnz 1b								\n"

				"adc %%ecx, %%ecx				\n"

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

		e.g.

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

			__asm
			{
				push eax
				push ebx
				push ecx
				push edx

				mov ecx, [b]
				sub ecx, [index]				

				mov edx, [index]
				mov ebx, [p1]

				mov eax, [value]

			ttmath_loop:
				sub [ebx+edx*4], eax
			jnc ttmath_end

				mov eax, 1
				inc edx
				dec ecx
			jnz ttmath_loop

			ttmath_end:
				setc al
				movzx edx, al
				mov [c], edx

				pop edx
				pop ecx
				pop ebx
				pop eax
			}

		#endif		
			

		#ifdef __GNUC__
		uint dummy, dummy2;

			__asm__ __volatile__(
			
				"subl %%edx, %%ecx 				\n"

			"1:									\n"
				"subl %%eax, (%%ebx,%%edx,4)	\n"
			"jnc 2f								\n"
				
				"movl $1, %%eax					\n"
				"incl %%edx						\n"
				"decl %%ecx						\n"
			"jnz 1b								\n"

			"2:									\n"
				"setc %%al						\n"
				"movzx %%al, %%edx				\n"

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

		uint rest = ss1_size - ss2_size;
		uint c;

		#ifndef __GNUC__
			
			//	this part might be compiled with for example visual c

			/*
				the asm code is nearly the same as in AddVector
				only two instructions 'adc' are changed to 'sbb'
			*/
			__asm
			{
				pushad

				mov ecx, [ss2_size]
				xor edx, edx               // edx = 0, cf = 0

				mov esi, [ss1]
				mov ebx, [ss2]
				mov edi, [result]

			ttmath_loop:
				mov eax, [esi+edx*4]
				sbb eax, [ebx+edx*4]
				mov [edi+edx*4], eax

				inc edx
				dec ecx
			jnz ttmath_loop

				adc ecx, ecx             // ecx has the cf state

				mov ebx, [rest]
				or ebx, ebx
				jz ttmath_end
				
				xor ebx, ebx             // ebx = 0
				neg ecx                  // setting cf from ecx
				mov ecx, [rest]          // ecx is != 0

			ttmath_loop2:
				mov eax, [esi+edx*4]
				sbb eax, ebx 
				mov [edi+edx*4], eax

				inc edx
				dec ecx
			jnz ttmath_loop2

				adc ecx, ecx

			ttmath_end:
				mov [c], ecx

				popad
			}

		#endif		
			

		#ifdef __GNUC__
			
		//	this part should be compiled with gcc
		uint dummy1, dummy2, dummy3;

			__asm__ __volatile__(
				"push %%edx							\n"
				"xor %%edx, %%edx					\n"   // edx = 0, cf = 0
			"1:										\n"
				"mov (%%esi,%%edx,4), %%eax			\n"
				"sbb (%%ebx,%%edx,4), %%eax			\n"
				"mov %%eax, (%%edi,%%edx,4)			\n"

				"inc %%edx							\n"
				"dec %%ecx							\n"
			"jnz 1b									\n"

				"adc %%ecx, %%ecx					\n"   // ecx has the cf state
				"pop %%eax							\n"   // eax = rest

				"or %%eax, %%eax					\n"
				"jz 3f								\n"
				
				"xor %%ebx, %%ebx					\n"   // ebx = 0
				"neg %%ecx							\n"   // setting cf from ecx
				"mov %%eax, %%ecx					\n"   // ecx=rest and is != 0
			"2:										\n"
				"mov (%%esi, %%edx, 4), %%eax		\n"
				"sbb %%ebx, %%eax 					\n"
				"mov %%eax, (%%edi, %%edx, 4)		\n"

				"inc %%edx							\n"
				"dec %%ecx							\n"
			"jnz 2b									\n"

				"adc %%ecx, %%ecx					\n"
			"3:										\n"

				: "=a" (dummy1), "=b" (dummy2), "=c" (c),       "=d" (dummy3)
				:                "1" (ss2),     "2" (ss2_size), "3" (rest),   "S" (ss1),  "D" (result)
				: "cc", "memory" );

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
	*/
	template<uint value_size>
	uint UInt<value_size>::Rcl2_one(uint c)
	{
	uint b = value_size;
	uint * p1 = table;

		#ifndef __GNUC__
			__asm
			{
				push ebx
				push ecx
				push edx

				mov ebx, [p1]
				xor edx, edx
				mov ecx, [c]
				neg ecx
				mov ecx, [b]

			ttmath_loop:
				rcl dword ptr [ebx+edx*4], 1
				
				inc edx
				dec ecx
			jnz ttmath_loop

				adc ecx, ecx
				mov [c], ecx
				
				pop edx
				pop ecx
				pop ebx
			}
		#endif


		#ifdef __GNUC__
		uint dummy, dummy2;

		__asm__  __volatile__(

			"xorl %%edx, %%edx			\n"   // edx=0
			"negl %%eax					\n"   // CF=1 if eax!=0 , CF=0 if eax==0

		"1:								\n"
			"rcll $1, (%%ebx, %%edx, 4)	\n"

			"incl %%edx					\n"
			"decl %%ecx					\n"
		"jnz 1b							\n"

			"adcl %%ecx, %%ecx			\n"

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
	*/
	template<uint value_size>
	uint UInt<value_size>::Rcr2_one(uint c)
	{
	uint b = value_size;
	uint * p1 = table;

		#ifndef __GNUC__
			__asm
			{
				push ebx
				push ecx

				mov ebx, [p1]
				mov ecx, [c]
				neg ecx
				mov ecx, [b]

			ttmath_loop:
				rcr dword ptr [ebx+ecx*4-4], 1
				
				dec ecx
			jnz ttmath_loop

				adc ecx, ecx
				mov [c], ecx

				pop ecx
				pop ebx
			}
		#endif


		#ifdef __GNUC__
		uint dummy;

		__asm__  __volatile__(

			"negl %%eax						\n"   // CF=1 if eax!=0 , CF=0 if eax==0

		"1:									\n"
			"rcrl $1, -4(%%ebx, %%ecx, 4)	\n"

			"decl %%ecx						\n"
		"jnz 1b								\n"

			"adcl %%ecx, %%ecx				\n"

			: "=c" (c), "=a" (dummy)
			: "0" (b),  "1" (c), "b" (p1)
			: "cc", "memory" );

		#endif

		TTMATH_LOGC("UInt::Rcr2_one", c)

	return c;
	}



#ifdef _MSC_VER
#pragma warning (disable : 4731)
//warning C4731: frame pointer register 'ebp' modified by inline assembly code
#endif
	


	/*!
		this method moves all bits into the left hand side
		return value <- this <- c

		the lowest *bits* will be held the 'c' and
		the state of one additional bit (on the left hand side)
		will be returned

		for example:
		let this is 001010000
		after Rcl2(3, 1) there'll be 010000111 and Rcl2 returns 1
	*/
	template<uint value_size>
	uint UInt<value_size>::Rcl2(uint bits, uint c)
	{
	TTMATH_ASSERT( bits>0 && bits<TTMATH_BITS_PER_UINT )
		
	uint b = value_size;
	uint * p1 = table;

		#ifndef __GNUC__
			__asm
			{
				push eax
				push ebx
				push ecx
				push edx
				push esi
				push edi
				push ebp

				mov edi, [b]

				mov ecx, 32
				sub ecx, [bits]
				mov edx, -1
				shr edx, cl

				mov ecx, [bits]
				mov ebx, [p1]
				mov eax, [c]

				mov ebp, edx         // ebp = mask (modified ebp - don't read/write to variables)

				xor edx, edx         // edx = 0
				mov esi, edx
				or eax, eax
				cmovnz esi, ebp      // if(c) esi=mask else esi=0

			ttmath_loop:
				rol dword ptr [ebx+edx*4], cl
				
				mov eax, [ebx+edx*4]
				and eax, ebp
				xor [ebx+edx*4], eax // clearing bits
				or [ebx+edx*4], esi  // saving old value
				mov esi, eax

				inc edx
				dec edi
			jnz ttmath_loop

				pop ebp              // restoring ebp

				and eax, 1
				mov [c], eax

				pop edi
				pop esi
				pop edx
				pop ecx
				pop ebx
				pop eax
			}
		#endif


		#ifdef __GNUC__
		uint dummy, dummy2, dummy3;

		__asm__  __volatile__(

			"push %%ebp						\n"
			
			"movl %%ecx, %%esi				\n"
			"movl $32, %%ecx				\n"
			"subl %%esi, %%ecx				\n"    // ecx = 32 - bits
			"movl $-1, %%edx				\n"    // edx = -1 (all bits set to one)
			"shrl %%cl, %%edx				\n"    // shifting (0 -> edx -> cf)  (cl times)
			"movl %%edx, %%ebp				\n"    // ebp = edx = mask
			"movl %%esi, %%ecx				\n"

			"xorl %%edx, %%edx				\n"
			"movl %%edx, %%esi				\n"
			"orl %%eax, %%eax				\n"
			"cmovnz %%ebp, %%esi			\n"    // if(c) esi=mask else esi=0

		"1:									\n"
			"roll %%cl, (%%ebx,%%edx,4)		\n"

			"movl (%%ebx,%%edx,4), %%eax	\n"
			"andl %%ebp, %%eax				\n"
			"xorl %%eax, (%%ebx,%%edx,4)	\n"
			"orl  %%esi, (%%ebx,%%edx,4)	\n"
			"movl %%eax, %%esi				\n"
			
			"incl %%edx						\n"
			"decl %%edi						\n"
		"jnz 1b								\n"
			
			"and $1, %%eax					\n"

			"pop %%ebp						\n"

			: "=a" (c), "=D" (dummy), "=S" (dummy2), "=d" (dummy3)
			: "0" (c),  "1" (b), "b" (p1), "c" (bits)
			: "cc", "memory" );

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
	*/
	template<uint value_size>
	uint UInt<value_size>::Rcr2(uint bits, uint c)
	{
	TTMATH_ASSERT( bits>0 && bits<TTMATH_BITS_PER_UINT )

	uint b = value_size;
	uint * p1 = table;

		#ifndef __GNUC__
			__asm
			{
				push eax
				push ebx
				push ecx
				push edx
				push esi
				push edi
				push ebp

				mov edi, [b]

				mov ecx, 32
				sub ecx, [bits]
				mov edx, -1
				shl edx, cl

				mov ecx, [bits]
				mov ebx, [p1]
				mov eax, [c]

				mov ebp, edx         // ebp = mask (modified ebp - don't read/write to variables)

				xor edx, edx         // edx = 0
				mov esi, edx
				add edx, edi
				dec edx              // edx is pointing at the end of the table (on last word)
				or eax, eax
				cmovnz esi, ebp      // if(c) esi=mask else esi=0

			ttmath_loop:
				ror dword ptr [ebx+edx*4], cl
				
				mov eax, [ebx+edx*4]
				and eax, ebp 
				xor [ebx+edx*4], eax // clearing bits
				or [ebx+edx*4], esi  // saving old value
				mov esi, eax

				dec edx
				dec edi
			jnz ttmath_loop

				pop ebp              // restoring ebp

				rol eax, 1           // 31bit will be first
				and eax, 1  
				mov [c], eax

				pop edi
				pop esi
				pop edx
				pop ecx
				pop ebx
				pop eax
			}
		#endif


		#ifdef __GNUC__
		uint dummy, dummy2, dummy3;

			__asm__  __volatile__(

			"push %%ebp						\n"
			
			"movl %%ecx, %%esi				\n"
			"movl $32, %%ecx				\n"
			"subl %%esi, %%ecx				\n"    // ecx = 32 - bits
			"movl $-1, %%edx				\n"    // edx = -1 (all bits set to one)
			"shll %%cl, %%edx				\n"    // shifting (cf <- edx <- 0)  (cl times)
			"movl %%edx, %%ebp				\n"    // ebp = edx = mask
			"movl %%esi, %%ecx				\n"

			"xorl %%edx, %%edx				\n"
			"movl %%edx, %%esi				\n"
			"addl %%edi, %%edx				\n"
			"decl %%edx						\n"    // edx is pointing at the end of the table (on last word)
			"orl %%eax, %%eax				\n"
			"cmovnz %%ebp, %%esi			\n"    // if(c) esi=mask else esi=0

		"1:									\n"
			"rorl %%cl, (%%ebx,%%edx,4)		\n"

			"movl (%%ebx,%%edx,4), %%eax	\n"
			"andl %%ebp, %%eax				\n"
			"xorl %%eax, (%%ebx,%%edx,4)	\n"
			"orl  %%esi, (%%ebx,%%edx,4)	\n"
			"movl %%eax, %%esi				\n"
			
			"decl %%edx						\n"
			"decl %%edi						\n"
		"jnz 1b								\n"
			
			"roll $1, %%eax					\n"
			"andl $1, %%eax					\n"

			"pop %%ebp						\n"

			: "=a" (c), "=D" (dummy), "=S" (dummy2), "=d" (dummy3)
			: "0" (c),  "1" (b), "b" (p1), "c" (bits)
			: "cc", "memory" );

		#endif

		TTMATH_LOGC("UInt::Rcr2", c)

	return c;
	}


#ifdef _MSC_VER
#pragma warning (default : 4731)
#endif


	/*
		this method returns the number of the highest set bit in one 32-bit word
		if the 'x' is zero this method returns '-1'
	*/
	template<uint value_size>
	sint UInt<value_size>::FindLeadingBitInWord(uint x)
	{
	sint result;

		#ifndef __GNUC__
			__asm
			{
				push eax
				push edx

				mov edx,-1
				bsr eax,[x]
				cmovz eax,edx
				mov [result], eax

				pop edx
				pop eax
			}
		#endif


		#ifdef __GNUC__
		uint dummy;

				__asm__ (

				"movl $-1, %1          \n"
				"bsrl %2, %0           \n"
				"cmovz %1, %0          \n"

				: "=r" (result), "=&r" (dummy)
				: "r" (x)
				: "cc" );

		#endif

	return result;
	}



	/*
		this method returns the number of the smallest set bit in one 32-bit word
		if the 'x' is zero this method returns '-1'
	*/
	template<uint value_size>
	sint UInt<value_size>::FindLowestBitInWord(uint x)
	{
	sint result;

		#ifndef __GNUC__
			__asm
			{
				push eax
				push edx

				mov edx,-1
				bsf eax,[x]
				cmovz eax,edx
				mov [result], eax

				pop edx
				pop eax
			}
		#endif


		#ifdef __GNUC__
		uint dummy;

				__asm__ (

				"movl $-1, %1          \n"
				"bsfl %2, %0           \n"
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

		bit is from <0,31>
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
			__asm
			{
			push ebx
			push eax

			mov eax, [v]
			mov ebx, [bit]
			bts eax, ebx
			mov [v], eax

			setc bl
			movzx ebx, bl
			mov [old_bit], ebx

			pop eax
			pop ebx
			}
		#endif


		#ifdef __GNUC__
			__asm__ (

			"btsl %%ebx, %%eax		\n"
			"setc %%bl				\n"
			"movzx %%bl, %%ebx		\n"
			
			: "=a" (v), "=b" (old_bit)
			: "0" (v),  "1" (bit)
			: "cc" );

		#endif

		value = v;

	return old_bit;
	}




	/*!
		multiplication: result_high:result_low = a * b
		result_high - higher word of the result
		result_low  - lower word of the result
	
		this methos never returns a carry
		this method is used in the second version of the multiplication algorithms
	*/
	template<uint value_size>
	void UInt<value_size>::MulTwoWords(uint a, uint b, uint * result_high, uint * result_low)
	{
	/*
		we must use these temporary variables in order to inform the compilator
		that value pointed with result1 and result2 has changed

		this has no effect in visual studio but it's useful when
		using gcc and options like -Ox
	*/
	uint result1_;
	uint result2_;

		#ifndef __GNUC__

			__asm
			{
			push eax
			push edx

			mov eax, [a]
			mul dword ptr [b]

			mov [result2_], edx
			mov [result1_], eax

			pop edx
			pop eax
			}

		#endif


		#ifdef __GNUC__

		__asm__ (
		
			"mull %%edx			\n"

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

		*
		* WARNING:
		* if r (one word) is too small for the result or c is equal zero
		* there'll be a hardware interruption (0)
		* and probably the end of your program
		*
	*/
	template<uint value_size>
	void UInt<value_size>::DivTwoWords(uint a, uint b, uint c, uint * r, uint * rest)
	{
		uint r_;
		uint rest_;
		/*
			these variables have similar meaning like those in
			the multiplication algorithm MulTwoWords
		*/

		TTMATH_ASSERT( c != 0 )

		#ifndef __GNUC__
			__asm
			{
				push eax
				push edx

				mov edx, [a]
				mov eax, [b]
				div dword ptr [c]

				mov [r_], eax
				mov [rest_], edx

				pop edx
				pop eax
			}
		#endif


		#ifdef __GNUC__
		
			__asm__ (

			"divl %%ecx				\n"

			: "=a" (r_), "=d" (rest_)
			: "0" (b),   "1" (a), "c" (c)
			: "cc" );

		#endif


		*r = r_;
		*rest = rest_;

	}



} //namespace



#endif //ifdef TTMATH_PLATFORM32
#endif //ifndef TTMATH_NOASM
#endif
