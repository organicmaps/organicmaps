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



#ifndef headerfilettmathint
#define headerfilettmathint

/*!
	\file ttmathint.h
    \brief template class Int<uint>
*/

#include "ttmathuint.h"

namespace ttmath
{


/*!
	\brief Int implements a big integer value with a sign

	value_size - how many bytes specify our value
		on 32bit platforms: value_size=1 -> 4 bytes -> 32 bits
		on 64bit platforms: value_size=1 -> 8 bytes -> 64 bits
	value_size = 1,2,3,4,5,6....
*/
template<uint value_size>
class Int : public UInt<value_size>
{
public:

	/*!
		this method sets the max value which this class can hold
		(all bits will be one besides the last one)
	*/
	void SetMax()
	{
		UInt<value_size>::SetMax();
		UInt<value_size>::table[value_size-1] = ~ TTMATH_UINT_HIGHEST_BIT;
	}


	/*!
		this method sets the min value which this class can hold
		(all bits will be zero besides the last one which is one)
	*/
	void SetMin()
	{
		UInt<value_size>::SetZero();
		UInt<value_size>::table[value_size-1] = TTMATH_UINT_HIGHEST_BIT;
	}


	/*!
		this method sets -1 as the value
		(-1 is equal the max value in an unsigned type)
	*/
	void SetSignOne()
	{
		UInt<value_size>::SetMax();
	}


	/*!
		we change the sign of the value

		if it isn't possible to change the sign this method returns 1
		else return 0 and changing the sign
	*/
	uint ChangeSign()
	{
		/*
			if the value is equal that one which has been returned from SetMin
			(only the highest bit is set) that means we can't change sign
			because the value is too big (bigger about one)

			e.g. when value_size = 1 and value is -2147483648 we can't change it to the
			2147483648 because the max value which can be held is 2147483647

			we don't change the value and we're using this fact somewhere in some methods
			(if we look on our value without the sign we get the correct value 
			eg. -2147483648 in Int<1> will be 2147483648 on the UInt<1> type)
		*/
		if( UInt<value_size>::IsOnlyTheHighestBitSet() )
			return 1;

		UInt<value_size> temp(*this);
		UInt<value_size>::SetZero();
		UInt<value_size>::Sub(temp);

	return 0;
	}



	/*!	
		this method sets the sign

		e.g. 1  -> -1
		     -2 -> -2
		
		from a positive value we make a negative value,
		if the value is negative we do nothing
	*/
	void SetSign()
	{
		if( IsSign() )
			return;

		ChangeSign();
	}



	/*!
		this method returns true if there's the sign

		(the highest bit will be converted to the bool)
	*/
	bool IsSign() const
	{
		return UInt<value_size>::IsTheHighestBitSet();
	}



	/*!
		it sets an absolute value

		it can return carry (1) (look on ChangeSign() for details)
	*/
	uint Abs()
	{
		if( !IsSign() )
			return 0;

	return ChangeSign();
	}




	/*!
	*
	*	basic mathematic functions
	*
	*/

private:

	uint CorrectCarryAfterAdding(bool p1_is_sign, bool p2_is_sign)
	{
		if( !p1_is_sign && !p2_is_sign )
		{
			if( UInt<value_size>::IsTheHighestBitSet() )
				return 1;
		}

		if( p1_is_sign && p2_is_sign )
		{	
			if( ! UInt<value_size>::IsTheHighestBitSet() )
				return 1;
		}

	return 0;
	}


public:

	/*!
		this method adds two value with a sign and returns a carry

		we're using methods from the base class because values are stored with U2
		we must only make the carry correction

		this = p1(=this) + p2

		when p1>=0 i p2>=0 carry is set when the highest bit of value is set
		when p1<0  i p2<0  carry is set when the highest bit of value is clear
		when p1>=0 i p2<0  carry will never be set
		when p1<0  i p2>=0 carry will never be set
	*/
	uint Add(const Int<value_size> & ss2)
	{
		bool p1_is_sign = IsSign();
		bool p2_is_sign = ss2.IsSign();

		UInt<value_size>::Add(ss2);		

	return CorrectCarryAfterAdding(p1_is_sign, p2_is_sign);
	}


	/*!
		this method adds one *unsigned* word (at a specific position)
		and returns a carry (if it was)

		look at a description in UInt<>::AddInt(...)
	*/
	uint AddInt(uint value, uint index = 0)
	{
		bool p1_is_sign = IsSign();

		UInt<value_size>::AddInt(value, index);		

	return CorrectCarryAfterAdding(p1_is_sign, false);
	}


	/*!
		this method adds two *unsigned* words to the existing value
		and these words begin on the 'index' position

		index should be equal or smaller than value_size-2 (index <= value_size-2)
		x1 - lower word, x2 - higher word

		look at a description in UInt<>::AddTwoInts(...)
	*/
	uint AddTwoInts(uint x2, uint x1, uint index)
	{
		bool p1_is_sign = IsSign();

		UInt<value_size>::AddTwoInts(x2, x1, index);		

	return CorrectCarryAfterAdding(p1_is_sign, false);
	}

private:

	uint CorrectCarryAfterSubtracting(bool p1_is_sign, bool p2_is_sign)
	{
		if( !p1_is_sign && p2_is_sign )
		{
			if( UInt<value_size>::IsTheHighestBitSet() )
				return 1;
		}

		if( p1_is_sign && !p2_is_sign )
		{	
			if( ! UInt<value_size>::IsTheHighestBitSet() )
				return 1;
		}

	return 0;
	}

public:

	/*!	
		this method subtracts two values with a sign

		we don't use the previous Add because the method ChangeSign can
		sometimes return carry 

		this = p1(=this) - p2

		when p1>=0 i p2>=0 carry will never be set
		when p1<0  i p2<0  carry will never be set
		when p1>=0 i p2<0  carry is set when the highest bit of value is set
		when p1<0  i p2>=0 carry is set when the highest bit of value is clear
	*/
	uint Sub(const Int<value_size> & ss2)
	{
		bool p1_is_sign = IsSign();
		bool p2_is_sign = ss2.IsSign();

		UInt<value_size>::Sub(ss2);		

	return CorrectCarryAfterSubtracting(p1_is_sign, p2_is_sign);
	}


	/*!
		this method subtracts one *unsigned* word (at a specific position)
		and returns a carry (if it was)
	*/
	uint SubInt(uint value, uint index = 0)
	{
		bool p1_is_sign = IsSign();

		UInt<value_size>::SubInt(value, index);		

	return CorrectCarryAfterSubtracting(p1_is_sign, false);
	}


	/*!
		this method adds one to the value and returns carry
	*/
	uint AddOne()
	{
		bool p1_is_sign = IsSign();

		UInt<value_size>::AddOne();		

	return CorrectCarryAfterAdding(p1_is_sign, false);
	}


	/*!
		this method subtracts one from the value and returns carry
	*/
	uint SubOne()
	{
		bool p1_is_sign = IsSign();

		UInt<value_size>::SubOne();		

	return CorrectCarryAfterSubtracting(p1_is_sign, false);
	}


private:


	uint CheckMinCarry(bool ss1_is_sign, bool ss2_is_sign)
	{
		/*
			we have to examine the sign of the result now
			but if the result is with the sign then:
				1. if the signs were the same that means the result is too big
				(the result must be without a sign)
				2. if the signs were different that means if the result
				is different from that one which has been returned from SetMin()
				that is carry (result too big) but if the result is equal SetMin()
				there'll be ok (and the next SetSign will has no effect because
				the value is actually negative -- look at description of that case
				in ChangeSign())
		*/
		if( IsSign() )
		{
			if( ss1_is_sign != ss2_is_sign )
			{
				/*
					there can be one case where signs are different and
					the result will be equal the value from SetMin() (only the highest bit is set)
					(this situation is ok)
				*/
				if( !UInt<value_size>::IsOnlyTheHighestBitSet() )
					return 1;
			}
			else
			{
				// signs were the same
				return 1;
			}
		}

	return 0;
	}


public:


	/*!
		multiplication: this = this * ss2

		it can return a carry
	*/
	uint MulInt(sint ss2)
	{
	bool ss1_is_sign, ss2_is_sign;
	uint c;

		ss1_is_sign = IsSign();

		/*
			we don't have to check the carry from Abs (values will be correct
			because next we're using the method MulInt from the base class UInt
			which is without a sign)
		*/
		Abs();

		if( ss2 < 0 )
		{
			ss2 = -ss2;
			ss2_is_sign = true;
		}
		else
		{
			ss2_is_sign = false;
		}

		c  = UInt<value_size>::MulInt((uint)ss2);
		c += CheckMinCarry(ss1_is_sign, ss2_is_sign);

		if( ss1_is_sign != ss2_is_sign )
			SetSign();

	return c;
	}



	/*!
		multiplication this = this * ss2

		it returns carry if the result is too big
		(we're using the method from the base class but we have to make
		one correction in account of signs)
	*/
	uint Mul(Int<value_size> ss2)
	{
	bool ss1_is_sign, ss2_is_sign;
	uint c;

		ss1_is_sign = IsSign();
		ss2_is_sign = ss2.IsSign();

		/*
			we don't have to check the carry from Abs (values will be correct
			because next we're using the method Mul from the base class UInt
			which is without a sign)
		*/
		Abs();
		ss2.Abs();

		c  = UInt<value_size>::Mul(ss2);
		c += CheckMinCarry(ss1_is_sign, ss2_is_sign);

		if( ss1_is_sign != ss2_is_sign )
			SetSign();

	return c;
	}


	/*!
		division this = this / ss2
		returned values:
			0 - ok
			1 - division by zero

		for example: (result means 'this')
			 20 /  3 --> result:  6   remainder:  2
			-20 /  3 --> result: -6   remainder: -2
			 20 / -3 --> result: -6   remainder:  2
			-20 / -3 --> result:  6   remainder: -2

		in other words: this(old) = ss2 * this(new)(result) + remainder
	*/
	uint Div(Int<value_size> ss2, Int<value_size> * remainder = 0)
	{
	bool ss1_is_sign, ss2_is_sign;

		ss1_is_sign = IsSign();
		ss2_is_sign = ss2.IsSign();

		/*
			we don't have to test the carry from Abs as well as in Mul
		*/
		Abs();
		ss2.Abs();

		uint c = UInt<value_size>::Div(ss2, remainder);

		if( ss1_is_sign != ss2_is_sign )
			SetSign();

		if( ss1_is_sign && remainder )
			remainder->SetSign();

	return c;
	}
	
	uint Div(const Int<value_size> & ss2, Int<value_size> & remainder)
	{
		return Div(ss2, &remainder);
	}


	/*!
		division this = this / ss2  (ss2 is int)
		returned values:
			0 - ok
			1 - division by zero

		for example: (result means 'this')
			 20 /  3 --> result:  6   remainder:  2
			-20 /  3 --> result: -6   remainder: -2
			 20 / -3 --> result: -6   remainder:  2
			-20 / -3 --> result:  6   remainder: -2

		in other words: this(old) = ss2 * this(new)(result) + remainder
	*/
	uint DivInt(sint ss2, sint * remainder = 0)
	{
	bool ss1_is_sign, ss2_is_sign;

		ss1_is_sign = IsSign();

		/*
			we don't have to test the carry from Abs as well as in Mul
		*/
		Abs();

		if( ss2 < 0 )
		{
			ss2 = -ss2;
			ss2_is_sign = true;
		}
		else
		{
			ss2_is_sign = false;
		}

		uint rem;
		uint c = UInt<value_size>::DivInt((uint)ss2, &rem);

		if( ss1_is_sign != ss2_is_sign )
			SetSign();

		if( remainder )
		{
			if( ss1_is_sign )
				*remainder = -sint(rem);
			else
				*remainder = sint(rem);
		}

	return c;
	}


	uint DivInt(sint ss2, sint & remainder)
	{
		return DivInt(ss2, &remainder);
	}


private:


	/*!
		power this = this ^ pow
		this can be negative
		pow is >= 0
	*/
	uint Pow2(const Int<value_size> & pow)
	{
		bool was_sign = IsSign();
		uint c = 0;

		if( was_sign )
			c += Abs();

		uint c_temp = UInt<value_size>::Pow(pow);
		if( c_temp > 0 )
			return c_temp; // c_temp can be: 0, 1 or 2
		
		if( was_sign && (pow.table[0] & 1) == 1 )
			// negative value to the power of odd number is negative
			c += ChangeSign();

	return (c==0)? 0 : 1;
	}


public:


	/*!
		power this = this ^ pow

		return values:
		0 - ok
		1 - carry
		2 - incorrect arguments 0^0 or 0^(-something)
	*/
	uint Pow(Int<value_size> pow)
	{
		if( !pow.IsSign() )
			return Pow2(pow);

		if( UInt<value_size>::IsZero() )
			// if 'pow' is negative then
			// 'this' must be different from zero
			return 2;

		if( pow.ChangeSign() )
			return 1;

		Int<value_size> t(*this);
		uint c_temp = t.Pow2(pow);
		if( c_temp > 0 )
			return c_temp;

		UInt<value_size>::SetOne();
		if( Div(t) )
			return 1;

	return 0;
	}



	/*!
	*
	*	convertion methods
	*
	*/
private:


	/*!
		an auxiliary method for converting both from UInt and Int
	*/
	template<uint argument_size>
	uint FromUIntOrInt(const UInt<argument_size> & p, bool UInt_type)
	{
		uint min_size = (value_size < argument_size)? value_size : argument_size;
		uint i;

		for(i=0 ; i<min_size ; ++i)
			UInt<value_size>::table[i] = p.table[i];


		if( value_size > argument_size )
		{	
			uint fill;

			if( UInt_type )
				fill = 0;
			else
				fill = (p.table[argument_size-1] & TTMATH_UINT_HIGHEST_BIT)?
														TTMATH_UINT_MAX_VALUE : 0;

			// 'this' is longer than 'p'
			for( ; i<value_size ; ++i)
				UInt<value_size>::table[i] = fill;
		}
		else
		{
			uint test = (UInt<value_size>::table[value_size-1] & TTMATH_UINT_HIGHEST_BIT)?
																TTMATH_UINT_MAX_VALUE : 0;

			if( UInt_type && test!=0 )
				return 1;

			for( ; i<argument_size ; ++i)
				if( p.table[i] != test )
					return 1;
		}

	return 0;
	}

public:

	/*!
		this method converts an Int<another_size> type into this class

		this operation has mainly sense if the value from p
		can be held in this type

		it returns a carry if the value 'p' is too big
	*/
	template<uint argument_size>
	uint FromInt(const Int<argument_size> & p)
	{
		return FromUIntOrInt(p, false);
	}


	/*!
		this method converts the sint type into this class
	*/
	uint FromInt(sint value)
	{
	uint fill = ( value<0 ) ? TTMATH_UINT_MAX_VALUE : 0;

		for(uint i=1 ; i<value_size ; ++i)
			UInt<value_size>::table[i] = fill;

		UInt<value_size>::table[0] = uint(value);
	
		// there'll never be a carry here
	return 0;
	}


	/*!
		this method converts UInt<another_size> into this class
	*/
	template<uint argument_size>
	uint FromUInt(const UInt<argument_size> & p)
	{
		return FromUIntOrInt(p, true);
	}


	/*!
		this method converts UInt<another_size> into this class
	*/
	template<uint argument_size>
	uint FromInt(const UInt<argument_size> & p)
	{
		return FromUIntOrInt(p, true);
	}


	/*!
		this method converts the uint type into this class
	*/
	uint FromUInt(uint value)
	{
		for(uint i=1 ; i<value_size ; ++i)
			UInt<value_size>::table[i] = 0;

		UInt<value_size>::table[0] = value;

		// there can be a carry here when the size of this value is equal one word
		// and the 'value' has the highest bit set
		if( value_size==1 && (value & TTMATH_UINT_HIGHEST_BIT)!=0 )
			return 1;

	return 0;
	}


	/*!
		this method converts the uint type into this class
	*/
	uint FromInt(uint value)
	{
		return FromUInt(value);
	}


	/*!
		the default assignment operator
	*/
	Int<value_size> & operator=(const Int<value_size> & p)
	{
		FromInt(p);

	return *this;
	}


	/*!
		this operator converts an Int<another_size> type to this class

		it doesn't return a carry
	*/
	template<uint argument_size>
	Int<value_size> & operator=(const Int<argument_size> & p)
	{
		FromInt(p);

	return *this;
	}


	/*!
		this method converts the sint type to this class
	*/
	Int<value_size> & operator=(sint i)
	{
		FromInt(i);

	return *this;
	}


	/*!
		a constructor for converting the uint to this class
	*/
	Int(sint i)
	{
		FromInt(i);
	}


	/*!
		a copy constructor
	*/
	Int(const Int<value_size> & u)
	{
		FromInt(u);
	}


	/*!
		a constructor for copying from another types
	*/
	template<uint argument_size>
	Int(const Int<argument_size> & u)
	{
		// look that 'size' we still set as 'value_size' and not as u.value_size
		FromInt(u);
	}



	/*!
		this operator converts an UInt<another_size> type to this class

		it doesn't return a carry
	*/
	template<uint argument_size>
	Int<value_size> & operator=(const UInt<argument_size> & p)
	{
		FromUInt(p);

	return *this;
	}


	/*!
		this method converts the Uint type to this class
	*/
	Int<value_size> & operator=(uint i)
	{
		FromUInt(i);

	return *this;
	}


	/*!
		a constructor for converting the uint to this class
	*/
	Int(uint i)
	{
		FromUInt(i);
	}


	/*!
		a constructor for copying from another types
	*/
	template<uint argument_size>
	Int(const UInt<argument_size> & u)
	{
		// look that 'size' we still set as 'value_size' and not as u.value_size
		FromUInt(u);
	}
 


#ifdef TTMATH_PLATFORM32


	/*!
		this method converts unsigned 64 bit int type to this class
		***this method is created only on a 32bit platform***
	*/
	uint FromUInt(ulint n)
	{
		uint c = UInt<value_size>::FromUInt(n);

		if( c )
			return 1;

		if( value_size == 1 )
			return ((UInt<value_size>::table[0] & TTMATH_UINT_HIGHEST_BIT) == 0) ? 0 : 1;
		
		if( value_size == 2 )
			return ((UInt<value_size>::table[1] & TTMATH_UINT_HIGHEST_BIT) == 0) ? 0 : 1;

	return 0;
	}


	/*!
		this method converts unsigned 64 bit int type to this class
		***this method is created only on a 32bit platform***
	*/
	uint FromInt(ulint n)
	{
		return FromUInt(n);
	}

		
	/*!
		this method converts signed 64 bit int type to this class
		***this method is created only on a 32bit platform***
	*/
	uint FromInt(slint n)
	{
	uint mask = (n < 0) ? TTMATH_UINT_MAX_VALUE : 0;

		UInt<value_size>::table[0] = (uint)(ulint)n;

		if( value_size == 1 )
		{
			if( uint(ulint(n) >> 32) != mask )
				return 1;

			return ((UInt<value_size>::table[0] & TTMATH_UINT_HIGHEST_BIT) == (mask & TTMATH_UINT_HIGHEST_BIT)) ? 0 : 1;
		}

		UInt<value_size>::table[1] = (uint)(ulint(n) >> 32);

		for(uint i=2 ; i<value_size ; ++i)
			UInt<value_size>::table[i] = mask;

	return 0;
	}


	/*!
		this operator converts unsigned 64 bit int type to this class
		***this operator is created only on a 32bit platform***
	*/
	Int<value_size> & operator=(ulint n)
	{
		FromUInt(n);

	return *this;
	}


	/*!
		a constructor for converting unsigned 64 bit int to this class
		***this constructor is created only on a 32bit platform***
	*/
	Int(ulint n)
	{
		FromUInt(n);
	}


	/*!
		this operator converts signed 64 bit int type to this class
		***this operator is created only on a 32bit platform***
	*/
	Int<value_size> & operator=(slint n)
	{
		FromInt(n);

	return *this;
	}


	/*!
		a constructor for converting signed 64 bit int to this class
		***this constructor is created only on a 32bit platform***
	*/
	Int(slint n)
	{
		FromInt(n);
	}

#endif




#ifdef TTMATH_PLATFORM64

	/*!
		this method converts 32 bit unsigned int type to this class
		***this operator is created only on a 64bit platform***
	*/
	uint FromUInt(unsigned int i)
	{
		return FromUInt(uint(i));
	}


	/*!
		this method converts 32 bit unsigned int type to this class
		***this operator is created only on a 64bit platform***
	*/
	uint FromInt(unsigned int i)
	{
		return FromUInt(i);
	}


	/*!
		this method converts 32 bit signed int type to this class
		***this operator is created only on a 64bit platform***
	*/
	uint FromInt(signed int i)
	{
		return FromInt(sint(i));
	}


	/*!
		this method converts 32 bit unsigned int type to this class
		***this operator is created only on a 64bit platform***
	*/
	Int<value_size> & operator=(unsigned int i)
	{
		FromUInt(i);

	return *this;
	}


	/*!
		a constructor for converting 32 bit unsigned int to this class
		***this constructor is created only on a 64bit platform***
	*/
	Int(unsigned int i)
	{
		FromUInt(i);
	}


	/*!
		this operator converts 32 bit signed int type to this class
		***this operator is created only on a 64bit platform***
	*/
	Int<value_size> & operator=(signed int i)
	{
		FromInt(i);

	return *this;
	}


	/*!
		a constructor for converting 32 bit signed int to this class
		***this constructor is created only on a 64bit platform***
	*/
	Int(signed int i)
	{
		FromInt(i);
	}

#endif



	/*!
		a constructor for converting string to this class (with the base=10)
	*/
	Int(const char * s)
	{
		FromString(s);
	}


	/*!
		a constructor for converting a string to this class (with the base=10)
	*/
	Int(const std::string & s)
	{
		FromString( s.c_str() );
	}


#ifndef TTMATH_DONT_USE_WCHAR

	/*!
		a constructor for converting string to this class (with the base=10)
	*/
	Int(const wchar_t * s)
	{
		FromString(s);
	}


	/*!
		a constructor for converting a string to this class (with the base=10)
	*/
	Int(const std::wstring & s)
	{
		FromString( s.c_str() );
	}

#endif


	/*!
		a default constructor

		we don't clear table etc.
	*/
	Int()
	{
	}


	/*!
		the destructor
	*/
	~Int()
	{
	}


	/*!
		this method returns the lowest value from table with a sign

		we must be sure when we using this method whether the value
		will hold in an sint type or not (the rest value from table must be zero or -1)
	*/
	sint ToInt() const
	{
		return sint( UInt<value_size>::table[0] );
	}


	/*!
		this method converts the value to uint type
		can return a carry if the value is too long to store it in uint type
	*/
	uint ToUInt(uint & result) const
	{
		uint c = UInt<value_size>::ToUInt(result);

		if( value_size == 1 )
			return (result & TTMATH_UINT_HIGHEST_BIT) == 0 ? 0 : 1;

	return c;
	}


	/*!
		this method converts the value to uint type
		can return a carry if the value is too long to store it in uint type
	*/
	uint ToInt(uint & result) const
	{
		return ToUInt(result);
	}


	/*!
		this method converts the value to sint type
		can return a carry if the value is too long to store it in sint type
	*/
	uint ToInt(sint & result) const
	{
		result = sint( UInt<value_size>::table[0] );
		uint mask = IsSign() ? TTMATH_UINT_MAX_VALUE : 0;

		if( (result & TTMATH_UINT_HIGHEST_BIT) != (mask & TTMATH_UINT_HIGHEST_BIT) )
			return 1;

		for(uint i=1 ; i<value_size ; ++i)
			if( UInt<value_size>::table[i] != mask )
				return 1;

	return 0;
	}


#ifdef TTMATH_PLATFORM32

	/*!
		this method converts the value to ulint type (64 bit unsigned integer)
		can return a carry if the value is too long to store it in ulint type
		*** this method is created only on a 32 bit platform ***
	*/
	uint ToUInt(ulint & result) const
	{
		uint c = UInt<value_size>::ToUInt(result);

		if( value_size == 1 )
			return (UInt<value_size>::table[0] & TTMATH_UINT_HIGHEST_BIT) == 0 ? 0 : 1;

		if( value_size == 2 )
			return (UInt<value_size>::table[1] & TTMATH_UINT_HIGHEST_BIT) == 0 ? 0 : 1;

	return c;
	}


	/*!
		this method converts the value to ulint type (64 bit unsigned integer)
		can return a carry if the value is too long to store it in ulint type
		*** this method is created only on a 32 bit platform ***
	*/
	uint ToInt(ulint & result) const
	{
		return ToUInt(result);
	}


	/*!
		this method converts the value to slint type (64 bit signed integer)
		can return a carry if the value is too long to store it in slint type
		*** this method is created only on a 32 bit platform ***
	*/
	uint ToInt(slint & result) const
	{
		if( value_size == 1 )
		{
			result = slint(sint(UInt<value_size>::table[0]));
		}
		else
		{
			uint low  = UInt<value_size>::table[0];
			uint high = UInt<value_size>::table[1];

			result = low;
			result |= (ulint(high) << TTMATH_BITS_PER_UINT);

			uint mask = IsSign() ? TTMATH_UINT_MAX_VALUE : 0;

			if( (high & TTMATH_UINT_HIGHEST_BIT) != (mask & TTMATH_UINT_HIGHEST_BIT) )
				return 1;

			for(uint i=2 ; i<value_size ; ++i)
				if( UInt<value_size>::table[i] != mask )
					return 1;
		}

	return 0;
	}

#endif



#ifdef TTMATH_PLATFORM64

	/*!
		this method converts the value to a 32 bit unsigned integer
		can return a carry if the value is too long to store it in this type
		*** this method is created only on a 64 bit platform ***
	*/
	uint ToUInt(unsigned int & result) const
	{
		uint c = UInt<value_size>::ToUInt(result);

		if( c || IsSign() )
			return 1;

	return 0;
	}


	/*!
		this method converts the value to a 32 bit unsigned integer
		can return a carry if the value is too long to store it in this type
		*** this method is created only on a 64 bit platform ***
	*/
	uint ToInt(unsigned int & result) const
	{
		return ToUInt(result);
	}


	/*!
		this method converts the value to a 32 bit signed integer
		can return a carry if the value is too long to store it in this type
		*** this method is created only on a 64 bit platform ***
	*/
	uint ToInt(int & result) const
	{
		uint first = UInt<value_size>::table[0];

		result = int(first);
		uint mask = IsSign() ? TTMATH_UINT_MAX_VALUE : 0;
	
		if( (first >> 31) != (mask >> 31) )
			return 1;

		for(uint i=1 ; i<value_size ; ++i)
			if( UInt<value_size>::table[i] != mask )
				return 1;

	return 0;
	}

#endif




private:

	/*!	
		an auxiliary method for converting to a string
	*/
	template<class string_type>
	void ToStringBase(string_type & result, uint b = 10) const
	{
		if( IsSign() )
		{
			Int<value_size> temp(*this);
			temp.Abs();
			temp.UInt<value_size>::ToStringBase(result, b, true);
		}
		else
		{
			UInt<value_size>::ToStringBase(result, b, false);
		}
	}

public:

	/*!	
		this method converts the value to a string with a base equal 'b'
	*/
	void ToString(std::string & result, uint b = 10) const
	{
		return ToStringBase(result, b);
	}


	/*!	
		this method converts the value to a string with a base equal 'b'
	*/
	std::string ToString(uint b = 10) const
	{
		std::string result;
		ToStringBase(result, b);

	return result;
	}


#ifndef TTMATH_DONT_USE_WCHAR

	/*!	
		this method converts the value to a string with a base equal 'b'
	*/
	void ToString(std::wstring & result, uint b = 10) const
	{
		return ToStringBase(result, b);
	}


	/*!	
		this method converts the value to a string with a base equal 'b'
	*/
	std::wstring ToWString(uint b = 10) const
	{
		std::wstring result;
		ToStringBase(result, b);

	return result;
	}

#endif



private:

	/*!
		an auxiliary method for converting from a string
	*/
	template<class char_type>
	uint FromStringBase(const char_type * s, uint b = 10, const char_type ** after_source = 0, bool * value_read = 0)
	{
	bool is_sign = false;
	
		Misc::SkipWhiteCharacters(s);

		if( *s == '-' )
		{
			is_sign = true;
			Misc::SkipWhiteCharacters(++s);
		}
		else
		if( *s == '+' )
		{
			Misc::SkipWhiteCharacters(++s);
		}

		if( UInt<value_size>::FromString(s,b,after_source,value_read) )
			return 1;

		if( is_sign )
		{
		Int<value_size> mmin;

			mmin.SetMin();

			/*
				the reference to mmin will be automatically converted to the reference
				to UInt type
				(this value can be equal mmin -- look at a description in ChangeSign())
			*/
			if( UInt<value_size>::operator>( mmin ) )
				return 1;

			/*
				if the value is equal mmin the method ChangeSign() does nothing (only returns 1 but we ignore it)
			*/
			ChangeSign();
		}
		else
		{
		Int<value_size> mmax;

			mmax.SetMax();

			if( UInt<value_size>::operator>( mmax ) )
					return 1;
		}

	return 0;
	}


public:

	/*!
		this method converts a string into its value
		it returns carry=1 if the value will be too big or an incorrect base 'b' is given

		string is ended with a non-digit value, for example:
			"-12" will be translated to -12
			as well as:
			"- 12foo" will be translated to -12 too

		existing first white characters will be ommited
		(between '-' and a first digit can be white characters too)

		after_source (if exists) is pointing at the end of the parsed string

		value_read (if exists) tells whether something has actually been read (at least one digit)
	*/
	uint FromString(const char * s, uint b = 10, const char ** after_source = 0, bool * value_read = 0)
	{
		return FromStringBase(s, b, after_source, value_read);
	}


	/*!
		this method converts a string into its value
	*/
	uint FromString(const wchar_t * s, uint b = 10, const wchar_t ** after_source = 0, bool * value_read = 0)
	{
		return FromStringBase(s, b, after_source, value_read);
	}


	/*!
		this method converts a string into its value
		it returns carry=1 if the value will be too big or an incorrect base 'b' is given
	*/
	uint FromString(const std::string & s, uint b = 10)
	{
		return FromString( s.c_str(), b );
	}


	/*!
		this operator converts a string into its value (with base = 10)
	*/
	Int<value_size> & operator=(const char * s)
	{
		FromString(s);

	return *this;
	}


#ifndef TTMATH_DONT_USE_WCHAR


	/*!
		this method converts a string into its value
		it returns carry=1 if the value will be too big or an incorrect base 'b' is given
	*/
	uint FromString(const std::wstring & s, uint b = 10)
	{
		return FromString( s.c_str(), b );
	}


	/*!
		this operator converts a string into its value (with base = 10)
	*/
	Int<value_size> & operator=(const wchar_t * s)
	{
		FromString(s);

	return *this;
	}


	/*!
		this operator converts a string into its value (with base = 10)
	*/
	Int<value_size> & operator=(const std::wstring & s)
	{
		FromString( s.c_str() );

	return *this;
	}

#endif


	/*!
		this operator converts a string into its value (with base = 10)
	*/
	Int<value_size> & operator=(const std::string & s)
	{
		FromString( s.c_str() );

	return *this;
	}



	/*!
	*
	*	methods for comparing
	*
	*
	*/

	bool operator==(const Int<value_size> & l) const
	{
		return UInt<value_size>::operator==(l);
	}

	bool operator!=(const Int<value_size> & l) const
	{
		return UInt<value_size>::operator!=(l);
	}

	bool operator<(const Int<value_size> & l) const
	{
		sint i=value_size-1;

		sint a1 = sint(UInt<value_size>::table[i]);
		sint a2 = sint(l.table[i]);

		if( a1 != a2 )
			return a1 < a2;


		for(--i ; i>=0 ; --i)
		{
			if( UInt<value_size>::table[i] != l.table[i] )
				// comparison as unsigned int
				return UInt<value_size>::table[i] < l.table[i];
		}

	// they're equal
	return false;
	}


	bool operator>(const Int<value_size> & l) const
	{
		sint i=value_size-1;

		sint a1 = sint(UInt<value_size>::table[i]);
		sint a2 = sint(l.table[i]);

		if( a1 != a2 )
			return a1 > a2;


		for(--i ; i>=0 ; --i)
		{
			if( UInt<value_size>::table[i] != l.table[i] )
				// comparison as unsigned int
				return UInt<value_size>::table[i] > l.table[i];
		}

	// they're equal
	return false;
	}


	bool operator<=(const Int<value_size> & l) const
	{
		sint i=value_size-1;

		sint a1 = sint(UInt<value_size>::table[i]);
		sint a2 = sint(l.table[i]);

		if( a1 != a2 )
			return a1 < a2;


		for(--i ; i>=0 ; --i)
		{
			if( UInt<value_size>::table[i] != l.table[i] )
				// comparison as unsigned int
				return UInt<value_size>::table[i] < l.table[i];
		}

	// they're equal
	return true;
	}


	bool operator>=(const Int<value_size> & l) const
	{
		sint i=value_size-1;

		sint a1 = sint(UInt<value_size>::table[i]);
		sint a2 = sint(l.table[i]);

		if( a1 != a2 )
			return a1 > a2;


		for(--i ; i>=0 ; --i)
		{
			if( UInt<value_size>::table[i] != l.table[i] )
				// comparison as unsigned int
				return UInt<value_size>::table[i] > l.table[i];
		}

	// they're equal
	return true;
	}



	/*!
	*
	*	standard mathematical operators 
	*
	*/


	/*!
		an operator for changing the sign

		it's not changing 'this' but the changed value will be returned
	*/
	Int<value_size> operator-() const
	{
	Int<value_size> temp(*this);

		temp.ChangeSign();
		
	return temp;
	}


	Int<value_size> operator-(const Int<value_size> & p2) const
	{
	Int<value_size> temp(*this);

		temp.Sub(p2);

	return temp;
	}


	Int<value_size> & operator-=(const Int<value_size> & p2)
	{
		Sub(p2);

	return *this;
	}


	Int<value_size> operator+(const Int<value_size> & p2) const
	{
	Int<value_size> temp(*this);

		temp.Add(p2);

	return temp;
	}


	Int<value_size> & operator+=(const Int<value_size> & p2)
	{
		Add(p2);

	return *this;
	}


	Int<value_size> operator*(const Int<value_size> & p2) const
	{
	Int<value_size> temp(*this);

		temp.Mul(p2);

	return temp;
	}


	Int<value_size> & operator*=(const Int<value_size> & p2)
	{
		Mul(p2);

	return *this;
	}


	Int<value_size> operator/(const Int<value_size> & p2) const
	{
	Int<value_size> temp(*this);

		temp.Div(p2);

	return temp;
	}


	Int<value_size> & operator/=(const Int<value_size> & p2)
	{
		Div(p2);

	return *this;
	}


	Int<value_size> operator%(const Int<value_size> & p2) const
	{
	Int<value_size> temp(*this);
	Int<value_size> remainder;
	
		temp.Div(p2, remainder);

	return remainder;
	}


	Int<value_size> & operator%=(const Int<value_size> & p2)
	{
	Int<value_size> remainder;
	
		Div(p2, remainder);
		operator=(remainder);

	return *this;
	}


	/*!
		Prefix operator e.g. ++variable
	*/
	UInt<value_size> & operator++()
	{
		AddOne();

	return *this;
	}


	/*!
		Postfix operator e.g. variable++
	*/
	UInt<value_size> operator++(int)
	{
	UInt<value_size> temp( *this );

		AddOne();

	return temp;
	}


	UInt<value_size> & operator--()
	{
		SubOne();

	return *this;
	}


	UInt<value_size> operator--(int)
	{
	UInt<value_size> temp( *this );

		SubOne();

	return temp;
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
	static ostream_type & OutputToStream(ostream_type & s, const Int<value_size> & l)
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
	friend std::ostream & operator<<(std::ostream & s, const Int<value_size> & l)
	{
		return OutputToStream<std::ostream, std::string>(s, l);
	}


#ifndef TTMATH_DONT_USE_WCHAR

	/*!
		output to standard streams
	*/
	friend std::wostream & operator<<(std::wostream & s, const Int<value_size> & l)
	{
		return OutputToStream<std::wostream, std::wstring>(s, l);
	}

#endif



private:

	/*!
		an auxiliary method for converting from a string
	*/
	template<class istream_type, class string_type, class char_type>
	static istream_type & InputFromStream(istream_type & s, Int<value_size> & l)
	{
	string_type ss;
	
	// char or wchar_t for operator>>
	char_type z;
	
		// operator>> omits white characters if they're set for ommiting
		s >> z;

		if( z=='-' || z=='+' )
		{
			ss += z;
			s >> z; // we're reading a next character (white characters can be ommited)
		}

		// we're reading only digits (base=10)
		while( s.good() && Misc::CharToDigit(z, 10)>=0 )
		{
			ss += z;
			z = static_cast<char_type>(s.get());
		}

		// we're leaving the last readed character
		// (it's not belonging to the value)
		s.unget();

		l.FromString(ss);

	return s;
	}


public:

	/*!
		input from standard streams
	*/
	friend std::istream & operator>>(std::istream & s, Int<value_size> & l)
	{
		return InputFromStream<std::istream, std::string, char>(s, l);
	}


#ifndef TTMATH_DONT_USE_WCHAR

	/*!
		input from standard streams
	*/
	friend std::wistream & operator>>(std::wistream & s, Int<value_size> & l)
	{
		return InputFromStream<std::wistream, std::wstring, wchar_t>(s, l);
	}
#endif


};

} // namespace

#endif
