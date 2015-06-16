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



#ifndef headerfilettmathmathtt
#define headerfilettmathmathtt

/*!
	\file ttmath.h
    \brief Mathematics functions.
*/

#ifdef _MSC_VER
//warning C4127: conditional expression is constant
#pragma warning( disable: 4127 )
//warning C4702: unreachable code
#pragma warning( disable: 4702 )
//warning C4800: forcing value to bool 'true' or 'false' (performance warning)
#pragma warning( disable: 4800 )
#endif


#include "ttmathbig.h"
#include "ttmathobjects.h"


namespace ttmath
{
	/*
 	 *
	 *  functions defined here are used only with Big<> types
	 *
	 *
	 */


	/*
 	 *
	 *  functions for rounding
	 *
	 *
	 */


	/*!
		this function skips the fraction from x
		e.g  2.2  = 2
		     2.7  = 2
			 -2.2 = 2
			 -2.7 = 2
	*/
	template<class ValueType>
	ValueType SkipFraction(const ValueType & x)
	{
		ValueType result( x );
		result.SkipFraction();

	return result;
	}


	/*!
		this function rounds to the nearest integer value
		e.g  2.2  = 2
		     2.7  = 3
			 -2.2 = -2
			 -2.7 = -3
	*/
	template<class ValueType>
	ValueType Round(const ValueType & x, ErrorCode * err = 0)
	{
		if( x.IsNan() )
		{
			if( err )
				*err = err_improper_argument;

		return x; // NaN
		}

		ValueType result( x );
		uint c = result.Round();

		if( err )
			*err = c ? err_overflow : err_ok;

	return result;
	}



	/*!
		this function returns a value representing the smallest integer
		that is greater than or equal to x

		Ceil(-3.7) = -3
		Ceil(-3.1) = -3
		Ceil(-3.0) = -3
		Ceil(4.0)  = 4
		Ceil(4.2)  = 5
		Ceil(4.8)  = 5
	*/
	template<class ValueType>
	ValueType Ceil(const ValueType & x, ErrorCode * err = 0)
	{
		if( x.IsNan() )
		{
			if( err )
				*err = err_improper_argument;

		return x; // NaN
		}

		ValueType result(x);
		uint c = 0;

		result.SkipFraction();

		if( result != x )
		{
			// x is with fraction
			// if x is negative we don't have to do anything
			if( !x.IsSign() )
			{
				ValueType one;
				one.SetOne();

				c += result.Add(one);
			}
		}

		if( err )
			*err = c ? err_overflow : err_ok;

	return result;	
	}


	/*!
		this function returns a value representing the largest integer
		that is less than or equal to x

		Floor(-3.6) = -4
		Floor(-3.1) = -4
		Floor(-3)   = -3
		Floor(2)    = 2
		Floor(2.3)  = 2
		Floor(2.8)  = 2
	*/
	template<class ValueType>
	ValueType Floor(const ValueType & x, ErrorCode * err = 0)
	{
		if( x.IsNan() )
		{
			if( err )
				*err = err_improper_argument;

		return x; // NaN
		}

		ValueType result(x);
		uint c = 0;

		result.SkipFraction();

		if( result != x )
		{
			// x is with fraction
			// if x is positive we don't have to do anything
			if( x.IsSign() )
			{
				ValueType one;
				one.SetOne();

				c += result.Sub(one);
			}
		}

		if( err )
			*err = c ? err_overflow : err_ok;

	return result;	
	}



	/*
 	 *
	 *  logarithms and the exponent
	 *
	 *
	 */

	
	/*!
		this function calculates the natural logarithm (logarithm with the base 'e')
	*/
	template<class ValueType>
	ValueType Ln(const ValueType & x, ErrorCode * err = 0)
	{
		if( x.IsNan() )
		{
			if( err )
				*err = err_improper_argument;

		return x; // NaN
		}

		ValueType result;
		uint state = result.Ln(x);

		if( err )
		{
			switch( state )
			{
			case 0:
				*err = err_ok;
				break;
			case 1:
				*err = err_overflow;
				break;
			case 2:
				*err = err_improper_argument;
				break;
			default:
				*err = err_internal_error;
				break;
			}
		}


	return result;
	}


	/*!
		this function calculates the logarithm
	*/
	template<class ValueType>
	ValueType Log(const ValueType & x, const ValueType & base, ErrorCode * err = 0)
	{
		if( x.IsNan() )
		{
			if( err ) *err = err_improper_argument;
			return x;
		}

		if( base.IsNan() )
		{
			if( err ) *err = err_improper_argument;
			return base;
		}

		ValueType result;
		uint state = result.Log(x, base);

		if( err )
		{
			switch( state )
			{
			case 0:
				*err = err_ok;
				break;
			case 1:
				*err = err_overflow;
				break;
			case 2:
			case 3:
				*err = err_improper_argument;
				break;
			default:
				*err = err_internal_error;
				break;
			}
		}

	return result;
	}


	/*!
		this function calculates the expression e^x
	*/
	template<class ValueType>
	ValueType Exp(const ValueType & x, ErrorCode * err = 0)
	{
		if( x.IsNan() )
		{
			if( err )
				*err = err_improper_argument;

		return x; // NaN
		}

		ValueType result;
		uint c = result.Exp(x);

		if( err )
			*err = c ? err_overflow : err_ok;

	return result;
	}


	/*!
	*
	*	trigonometric functions
	*
	*/


	/*
		this namespace consists of auxiliary functions
		(something like 'private' in a class)
	*/
	namespace auxiliaryfunctions
	{

	/*!
		an auxiliary function for calculating the Sine
		(you don't have to call this function) 
	*/
	template<class ValueType>
	uint PrepareSin(ValueType & x, bool & change_sign)
	{
	ValueType temp;

		change_sign = false;
	
		if( x.IsSign() )
		{
			// we're using the formula 'sin(-x) = -sin(x)'
			change_sign = !change_sign;
			x.ChangeSign();
		}
	
		// we're reducing the period 2*PI
		// (for big values there'll always be zero)
		temp.Set2Pi();
		
		if( x.Mod(temp) )
			return 1;
		

		// we're setting 'x' as being in the range of <0, 0.5PI>

		temp.SetPi();

		if( x > temp )
		{
			// x is in (pi, 2*pi>
			x.Sub( temp );
			change_sign = !change_sign;
		}
		
		temp.Set05Pi();

		if( x > temp )
		{
			// x is in (0.5pi, pi>
			x.Sub( temp );
			x = temp - x;
		}

	return 0;
	}

	
	/*!
		an auxiliary function for calculating the Sine
		(you don't have to call this function) 

		it returns Sin(x) where 'x' is from <0, PI/2>
		we're calculating the Sin with using Taylor series in zero or PI/2
		(depending on which point of these two points is nearer to the 'x')

		Taylor series:
		sin(x) = sin(a) + cos(a)*(x-a)/(1!)
		         - sin(a)*((x-a)^2)/(2!) - cos(a)*((x-a)^3)/(3!)
				 + sin(a)*((x-a)^4)/(4!) + ...

		when a=0 it'll be:
		sin(x) = (x)/(1!) - (x^3)/(3!) + (x^5)/(5!) - (x^7)/(7!) + (x^9)/(9!) ...

		and when a=PI/2:
		sin(x) = 1 - ((x-PI/2)^2)/(2!) + ((x-PI/2)^4)/(4!) - ((x-PI/2)^6)/(6!) ...
	*/
	template<class ValueType>
	ValueType Sin0pi05(const ValueType & x)
	{
	ValueType result;
	ValueType numerator, denominator;
	ValueType d_numerator, d_denominator;
	ValueType one, temp, old_result;

		// temp = pi/4
		temp.Set05Pi();
		temp.exponent.SubOne();

		one.SetOne();

		if( x < temp ) 
		{	
			// we're using the Taylor series with a=0
			result    = x;
			numerator = x;
			denominator = one;

			// d_numerator = x^2
			d_numerator = x;
			d_numerator.Mul(x);

			d_denominator = 2;
		}
		else
		{
			// we're using the Taylor series with a=PI/2
			result = one;
			numerator = one;
			denominator = one;

			// d_numerator = (x-pi/2)^2
			ValueType pi05;
			pi05.Set05Pi();

			temp = x;
			temp.Sub( pi05 );
			d_numerator = temp;
			d_numerator.Mul( temp );

			d_denominator = one;
		}

		uint c = 0;
		bool addition = false;

		old_result = result;
		for(uint i=1 ; i<=TTMATH_ARITHMETIC_MAX_LOOP ; ++i)
		{
			// we're starting from a second part of the formula
			c += numerator.    Mul( d_numerator );
			c += denominator.  Mul( d_denominator );
			c += d_denominator.Add( one );
			c += denominator.  Mul( d_denominator );
			c += d_denominator.Add( one );
			temp = numerator;
			c += temp.Div(denominator);

			if( c )
				// Sin is from <-1,1> and cannot make an overflow
				// but the carry can be from the Taylor series
				// (then we only break our calculations)
				break;

			if( addition )
				result.Add( temp );
			else
				result.Sub( temp );


			addition = !addition;
	
			// we're testing whether the result has changed after adding
			// the next part of the Taylor formula, if not we end the loop
			// (it means 'x' is zero or 'x' is PI/2 or this part of the formula
			// is too small)
			if( result == old_result )
				break;

			old_result = result;
		}

	return result;
	}

	} // namespace auxiliaryfunctions



	/*!
		this function calculates the Sine
	*/
	template<class ValueType>
	ValueType Sin(ValueType x, ErrorCode * err = 0)
	{
	using namespace auxiliaryfunctions;

	ValueType one, result;
	bool change_sign;	
	
		if( x.IsNan() )
		{
			if( err )
				*err = err_improper_argument;

		return x;
		}

		if( err )
			*err = err_ok;

		if( PrepareSin( x, change_sign ) )
		{
			// x is too big, we cannnot reduce the 2*PI period
			// prior to version 0.8.5 the result was zero
			
			// result has NaN flag set by default

			if( err )
				*err = err_overflow; // maybe another error code? err_improper_argument?

		return result; // NaN is set by default
		}

		result = Sin0pi05( x );
	
		one.SetOne();

		// after calculations there can be small distortions in the result
		if( result > one )
			result = one;
		else
		if( result.IsSign() )
			// we've calculated the sin from <0, pi/2> and the result
			// should be positive
			result.SetZero();

		if( change_sign )
			result.ChangeSign();	
		
	return result;
	}

	
	/*!
		this function calulates the Cosine
		we're using the formula cos(x) = sin(x + PI/2)
	*/
	template<class ValueType>
	ValueType Cos(ValueType x, ErrorCode * err = 0)
	{
		if( x.IsNan() )
		{
			if( err )
				*err = err_improper_argument;

		return x; // NaN
		}

		ValueType pi05;
		pi05.Set05Pi();

		uint c = x.Add( pi05 );

		if( c )
		{
			if( err )
				*err = err_overflow;
	
		return ValueType(); // result is undefined (NaN is set by default)
		}

	return Sin(x, err);
	}
	

	/*!
		this function calulates the Tangent
		we're using the formula tan(x) = sin(x) / cos(x)

		it takes more time than calculating the Tan directly
		from for example Taylor series but should be a bit preciser
		because Tan receives its values from -infinity to +infinity
		and when we calculate it from any series then we can make
		a greater mistake than calculating 'sin/cos'
	*/
	template<class ValueType>
	ValueType Tan(const ValueType & x, ErrorCode * err = 0)
	{
		ValueType result = Cos(x, err);
		
		if( err && *err != err_ok )
			return result;

		if( result.IsZero() )
		{
			if( err )
				*err = err_improper_argument;

			result.SetNan();

		return result;
		}

	return Sin(x, err) / result;
	}


	/*!
		this function calulates the Tangent
		look at the description of Tan(...)

		(the abbreviation of Tangent can be 'tg' as well)
	*/
	template<class ValueType>
	ValueType Tg(const ValueType & x, ErrorCode * err = 0)
	{
		return Tan(x, err);
	}


	/*!
		this function calulates the Cotangent
		we're using the formula tan(x) = cos(x) / sin(x)

		(why do we make it in this way? 
		look at information in Tan() function)
	*/
	template<class ValueType>
	ValueType Cot(const ValueType & x, ErrorCode * err = 0)
	{
		ValueType result = Sin(x, err);

		if( err && *err != err_ok )
			return result;

		if( result.IsZero() )
		{
			if( err )
				*err = err_improper_argument;

			result.SetNan();

		return result;
		}
	
	return Cos(x, err) / result;
	}


	/*!
		this function calulates the Cotangent
		look at the description of Cot(...)

		(the abbreviation of Cotangent can be 'ctg' as well)
	*/
	template<class ValueType>
	ValueType Ctg(const ValueType & x, ErrorCode * err = 0)
	{
		return Cot(x, err);
	}


	/*
 	 *
	 *  inverse trigonometric functions
	 *
	 *
	 */

	namespace auxiliaryfunctions
	{

	/*!
		an auxiliary function for calculating the Arc Sine

		we're calculating asin from the following formula:
		asin(x) = x + (1*x^3)/(2*3) + (1*3*x^5)/(2*4*5) + (1*3*5*x^7)/(2*4*6*7) + ... 
		where abs(x) <= 1

		we're using this formula when x is from <0, 1/2>
	*/
	template<class ValueType>
	ValueType ASin_0(const ValueType & x)
	{
	ValueType nominator, denominator, nominator_add, nominator_x, denominator_add, denominator_x;
	ValueType two, result(x), x2(x);
	ValueType nominator_temp, denominator_temp, old_result = result;
	uint c = 0;

	x2.Mul(x);
	two = 2;

	nominator.SetOne();
	denominator     = two;
	nominator_add   = nominator;
	denominator_add = denominator;
	nominator_x     = x;
	denominator_x   = 3;

		for(uint i=1 ; i<=TTMATH_ARITHMETIC_MAX_LOOP ; ++i)
		{
			c += nominator_x.Mul(x2);
			nominator_temp = nominator_x;	
			c += nominator_temp.Mul(nominator);
			denominator_temp = denominator;
			c += denominator_temp.Mul(denominator_x);
			c += nominator_temp.Div(denominator_temp);

			// if there is a carry somewhere we only break the calculating
			// the result should be ok -- it's from <-pi/2, pi/2>
			if( c ) 
				break;

			result.Add(nominator_temp);
			
			if( result == old_result )
				 // there's no sense to calculate more
				break;

			old_result = result;


			c += nominator_add.Add(two);
			c += denominator_add.Add(two);
			c += nominator.Mul(nominator_add);
			c += denominator.Mul(denominator_add);
			c += denominator_x.Add(two);
		}

	return result;
	}



	/*!
		an auxiliary function for calculating the Arc Sine

		we're calculating asin from the following formula:
		asin(x) = pi/2 - sqrt(2)*sqrt(1-x) * asin_temp
		asin_temp = 1 + (1*(1-x))/((2*3)*(2)) + (1*3*(1-x)^2)/((2*4*5)*(4)) + (1*3*5*(1-x)^3)/((2*4*6*7)*(8)) + ... 

		where abs(x) <= 1

		we're using this formula when x is from (1/2, 1>
	*/
	template<class ValueType>
	ValueType ASin_1(const ValueType & x)
	{
	ValueType nominator, denominator, nominator_add, nominator_x, nominator_x_add, denominator_add, denominator_x;
	ValueType denominator2;
	ValueType one, two, result;
	ValueType nominator_temp, denominator_temp, old_result;
	uint c = 0;

	two = 2;

	one.SetOne();
	nominator		= one;
	result			= one;
	old_result		= result;
	denominator     = two;
	nominator_add   = nominator;
	denominator_add = denominator;
	nominator_x     = one;
	nominator_x.Sub(x);
	nominator_x_add = nominator_x;
	denominator_x   = 3;
	denominator2	= two;


		for(uint i=1 ; i<=TTMATH_ARITHMETIC_MAX_LOOP ; ++i)
		{
			nominator_temp = nominator_x;	
			c += nominator_temp.Mul(nominator);
			denominator_temp = denominator;
			c += denominator_temp.Mul(denominator_x);
			c += denominator_temp.Mul(denominator2);
			c += nominator_temp.Div(denominator_temp);

			// if there is a carry somewhere we only break the calculating
			// the result should be ok -- it's from <-pi/2, pi/2>
			if( c ) 
				break;

			result.Add(nominator_temp);
			
			if( result == old_result )
				 // there's no sense to calculate more
				break;

			old_result = result;

			c += nominator_x.Mul(nominator_x_add);
			c += nominator_add.Add(two);
			c += denominator_add.Add(two);
			c += nominator.Mul(nominator_add);
			c += denominator.Mul(denominator_add);
			c += denominator_x.Add(two);
			c += denominator2.Mul(two);
		}

		
		nominator_x_add.exponent.AddOne(); // *2
		one.exponent.SubOne(); // =0.5
		nominator_x_add.Pow(one); // =sqrt(nominator_x_add)
		result.Mul(nominator_x_add);

		one.Set05Pi();
		one.Sub(result);

	return one;
	}


	} // namespace auxiliaryfunctions


	/*!
		this function calculates the Arc Sine
		x is from <-1,1>
	*/
	template<class ValueType>
	ValueType ASin(ValueType x, ErrorCode * err = 0)
	{
	using namespace auxiliaryfunctions;

		ValueType result, one;
		one.SetOne();
		bool change_sign = false;

		if( x.IsNan() )
		{
			if( err )
				*err = err_improper_argument;

		return x;
		}

		if( x.GreaterWithoutSignThan(one) )
		{
			if( err )
				*err = err_improper_argument;

			return result; // NaN is set by default
		}

		if( x.IsSign() )
		{
			change_sign = true;
			x.Abs();
		}

		one.exponent.SubOne(); // =0.5

		// asin(-x) = -asin(x)
		if( x.GreaterWithoutSignThan(one) )
			result = ASin_1(x);	
		else
			result = ASin_0(x);

		if( change_sign )
			result.ChangeSign();

		if( err )
			*err = err_ok;

	return result;
	}


	/*!
		this function calculates the Arc Cosine

		we're using the formula:
		acos(x) = pi/2 - asin(x)
	*/
	template<class ValueType>
	ValueType ACos(const ValueType & x, ErrorCode * err = 0)
	{
	ValueType temp;

		temp.Set05Pi();
		temp.Sub(ASin(x, err));

	return temp;
	}



	namespace auxiliaryfunctions
	{

	/*!
		an auxiliary function for calculating the Arc Tangent

		arc tan (x) where x is in <0; 0.5)
		(x can be in (-0.5 ; 0.5) too)

		we're using the Taylor series expanded in zero:
		atan(x) = x - (x^3)/3 + (x^5)/5 - (x^7)/7 + ...
	*/
	template<class ValueType>
	ValueType ATan0(const ValueType & x)
	{
		ValueType nominator, denominator, nominator_add, denominator_add, temp;
		ValueType result, old_result;
		bool adding = false;
		uint c = 0;

		result        = x;
		old_result    = result;
		nominator     = x;
		nominator_add = x;
		nominator_add.Mul(x);

		denominator.SetOne();
		denominator_add = 2;

		for(uint i=1 ; i<=TTMATH_ARITHMETIC_MAX_LOOP ; ++i)
		{
			c += nominator.Mul(nominator_add);
			c += denominator.Add(denominator_add);
	
			temp = nominator;
			c += temp.Div(denominator);

			if( c )
				// the result should be ok
				break;

			if( adding )
				result.Add(temp);
			else
				result.Sub(temp);

			if( result == old_result )
				 // there's no sense to calculate more
				break;

			old_result = result;
			adding     = !adding;
		}

	return result;
	}


	/*!
		an auxiliary function for calculating the Arc Tangent

		where x is in <0 ; 1>
	*/
	template<class ValueType>
	ValueType ATan01(const ValueType & x)
	{
		ValueType half;
		half.Set05();

		/*
			it would be better if we chose about sqrt(2)-1=0.41... instead of 0.5 here

			because as you can see below:
			when x = sqrt(2)-1
			abs(x) = abs( (x-1)/(1+x) )
			so when we're calculating values around x
			then they will be better converged to each other

			for example if we have x=0.4999 then during calculating ATan0(0.4999)
			we have to make about 141 iterations but when we have x=0.5
			then during calculating ATan0( (x-1)/(1+x) ) we have to make 
			only about 89 iterations (both for Big<3,9>)

			in the future this 0.5 can be changed
		*/
		if( x.SmallerWithoutSignThan(half) )
			return ATan0(x);


		/*
			x>=0.5 and x<=1
			(x can be even smaller than 0.5)

			y = atac(x)
			x = tan(y)

			tan(y-b) = (tan(y)-tab(b)) / (1+tan(y)*tan(b))
			y-b      = atan( (tan(y)-tab(b)) / (1+tan(y)*tan(b)) )
			y        = b + atan( (x-tab(b)) / (1+x*tan(b)) )

			let b = pi/4
			tan(b) = tan(pi/4) = 1
			y = pi/4 + atan( (x-1)/(1+x) )

			so
			atac(x) = pi/4 + atan( (x-1)/(1+x) )
			when x->1 (x converges to 1) the (x-1)/(1+x) -> 0
			and we can use ATan0() function here
		*/

		ValueType n(x),d(x),one,result;

		one.SetOne();
		n.Sub(one);
		d.Add(one);
		n.Div(d);

		result = ATan0(n);

		n.Set05Pi();
		n.exponent.SubOne(); // =pi/4
		result.Add(n);

	return result;
	}


	/*!
		an auxiliary function for calculating the Arc Tangent
		where x > 1

		we're using the formula:
		atan(x) = pi/2 - atan(1/x) for x>0
	*/
	template<class ValueType>
	ValueType ATanGreaterThanPlusOne(const ValueType & x)
	{
	ValueType temp, atan;

		temp.SetOne();
		
		if( temp.Div(x) )
		{
			// if there was a carry here that means x is very big
			// and atan(1/x) fast converged to 0
			atan.SetZero();
		}
		else
			atan = ATan01(temp);
		
		temp.Set05Pi();
		temp.Sub(atan);

	return temp;
	}

	} // namespace auxiliaryfunctions


	/*!
		this function calculates the Arc Tangent
	*/
	template<class ValueType>
	ValueType ATan(ValueType x)
	{
	using namespace auxiliaryfunctions;

		ValueType one, result;
		one.SetOne();
		bool change_sign = false;

		if( x.IsNan() )
			return x;

		// if x is negative we're using the formula:
		// atan(-x) = -atan(x)
		if( x.IsSign() )
		{
			change_sign = true;
			x.Abs();
		}

		if( x.GreaterWithoutSignThan(one) )
			result = ATanGreaterThanPlusOne(x);
		else
			result = ATan01(x);

		if( change_sign )
			result.ChangeSign();

	return result;
	}


	/*!
		this function calculates the Arc Tangent
		look at the description of ATan(...)

		(the abbreviation of Arc Tangent can be 'atg' as well)
	*/
	template<class ValueType>
	ValueType ATg(const ValueType & x)
	{
		return ATan(x);
	}


	/*!
		this function calculates the Arc Cotangent
	
		we're using the formula:
		actan(x) = pi/2 - atan(x)
	*/
	template<class ValueType>
	ValueType ACot(const ValueType & x)
	{
	ValueType result;

		result.Set05Pi();
		result.Sub(ATan(x));

	return result;
	}


	/*!
		this function calculates the Arc Cotangent
		look at the description of ACot(...)

		(the abbreviation of Arc Cotangent can be 'actg' as well)
	*/
	template<class ValueType>
	ValueType ACtg(const ValueType & x)
	{
		return ACot(x);
	}


	/*
 	 *
	 *  hyperbolic functions
	 *
	 *
	 */


	/*!
		this function calculates the Hyperbolic Sine

		we're using the formula sinh(x)= ( e^x - e^(-x) ) / 2
	*/
	template<class ValueType>
	ValueType Sinh(const ValueType & x, ErrorCode * err = 0)
	{
		if( x.IsNan() )
		{
			if( err )
				*err = err_improper_argument;

		return x; // NaN
		}

		ValueType ex, emx;
		uint c = 0;

		c += ex.Exp(x);
		c += emx.Exp(-x);

		c += ex.Sub(emx);
		c += ex.exponent.SubOne();

		if( err )
			*err = c ? err_overflow : err_ok;

	return ex;
	}


	/*!
		this function calculates the Hyperbolic Cosine

		we're using the formula cosh(x)= ( e^x + e^(-x) ) / 2
	*/
	template<class ValueType>
	ValueType Cosh(const ValueType & x, ErrorCode * err = 0)
	{
		if( x.IsNan() )
		{
			if( err )
				*err = err_improper_argument;

		return x; // NaN
		}

		ValueType ex, emx;
		uint c = 0;

		c += ex.Exp(x);
		c += emx.Exp(-x);

		c += ex.Add(emx);
		c += ex.exponent.SubOne();

		if( err )
			*err = c ? err_overflow : err_ok;

	return ex;
	}


	/*!
		this function calculates the Hyperbolic Tangent

		we're using the formula tanh(x)= ( e^x - e^(-x) ) / ( e^x + e^(-x) )
	*/
	template<class ValueType>
	ValueType Tanh(const ValueType & x, ErrorCode * err = 0)
	{
		if( x.IsNan() )
		{
			if( err )
				*err = err_improper_argument;

		return x; // NaN
		}

		ValueType ex, emx, nominator, denominator;
		uint c = 0;

		c += ex.Exp(x);
		c += emx.Exp(-x);

		nominator = ex;
		c += nominator.Sub(emx);
		denominator = ex;
		c += denominator.Add(emx);
		
		c += nominator.Div(denominator);

		if( err )
			*err = c ? err_overflow : err_ok;

	return nominator;
	}


	/*!
		this function calculates the Hyperbolic Tangent
		look at the description of Tanh(...)

		(the abbreviation of Hyperbolic Tangent can be 'tgh' as well)
	*/
	template<class ValueType>
	ValueType Tgh(const ValueType & x, ErrorCode * err = 0)
	{
		return Tanh(x, err);
	}

	/*!
		this function calculates the Hyperbolic Cotangent

		we're using the formula coth(x)= ( e^x + e^(-x) ) / ( e^x - e^(-x) )
	*/
	template<class ValueType>
	ValueType Coth(const ValueType & x, ErrorCode * err = 0)
	{
		if( x.IsNan() )
		{
			if( err )
				*err = err_improper_argument;

		return x; // NaN
		}

		if( x.IsZero() )
		{
			if( err )
				*err = err_improper_argument;

			return ValueType(); // NaN is set by default
		}

		ValueType ex, emx, nominator, denominator;
		uint c = 0;

		c += ex.Exp(x);
		c += emx.Exp(-x);

		nominator = ex;
		c += nominator.Add(emx);
		denominator = ex;
		c += denominator.Sub(emx);
		
		c += nominator.Div(denominator);

		if( err )
			*err = c ? err_overflow : err_ok;

	return nominator;
	}


	/*!
		this function calculates the Hyperbolic Cotangent
		look at the description of Coth(...)

		(the abbreviation of Hyperbolic Cotangent can be 'ctgh' as well)
	*/
	template<class ValueType>
	ValueType Ctgh(const ValueType & x, ErrorCode * err = 0)
	{
		return Coth(x, err);
	}


	/*
 	 *
	 *  inverse hyperbolic functions
	 *
	 *
	 */


	/*!
		inverse hyperbolic sine

		asinh(x) = ln( x + sqrt(x^2 + 1) )
	*/
	template<class ValueType>
	ValueType ASinh(const ValueType & x, ErrorCode * err = 0)
	{
		if( x.IsNan() )
		{
			if( err )
				*err = err_improper_argument;

		return x; // NaN
		}

		ValueType xx(x), one, result;
		uint c = 0;
		one.SetOne();

		c += xx.Mul(x);
		c += xx.Add(one);
		one.exponent.SubOne(); // one=0.5
		// xx is >= 1 
		c += xx.PowFrac(one); // xx=sqrt(xx)
		c += xx.Add(x);
		c += result.Ln(xx); // xx > 0

		// here can only be a carry
		if( err )
			*err = c ? err_overflow : err_ok;

	return result;
	}


	/*!
		inverse hyperbolic cosine

		acosh(x) = ln( x + sqrt(x^2 - 1) )  x in <1, infinity)
	*/
	template<class ValueType>
	ValueType ACosh(const ValueType & x, ErrorCode * err = 0)
	{
		if( x.IsNan() )
		{
			if( err )
				*err = err_improper_argument;

		return x; // NaN
		}

		ValueType xx(x), one, result;
		uint c = 0;
		one.SetOne();

		if( x < one )
		{
			if( err )
				*err = err_improper_argument;

		return result; // NaN is set by default
		}

		c += xx.Mul(x);
		c += xx.Sub(one);
		// xx is >= 0
		// we can't call a PowFrac when the 'x' is zero
		// if x is 0 the sqrt(0) is 0
		if( !xx.IsZero() )
		{
			one.exponent.SubOne(); // one=0.5
			c += xx.PowFrac(one); // xx=sqrt(xx)
		}
		c += xx.Add(x);
		c += result.Ln(xx); // xx >= 1

		// here can only be a carry
		if( err )
			*err = c ? err_overflow : err_ok;

	return result;
	}


	/*!
		inverse hyperbolic tangent

		atanh(x) = 0.5 * ln( (1+x) / (1-x) )  x in (-1, 1)
	*/
	template<class ValueType>
	ValueType ATanh(const ValueType & x, ErrorCode * err = 0)
	{
		if( x.IsNan() )
		{
			if( err )
				*err = err_improper_argument;

		return x; // NaN
		}

		ValueType nominator(x), denominator, one, result;
		uint c = 0;
		one.SetOne();

		if( !x.SmallerWithoutSignThan(one) )
		{
			if( err )
				*err = err_improper_argument;

		return result; // NaN is set by default
		}

		c += nominator.Add(one);
		denominator = one;
		c += denominator.Sub(x);
		c += nominator.Div(denominator);
		c += result.Ln(nominator);
		c += result.exponent.SubOne();

		// here can only be a carry
		if( err )
			*err = c ? err_overflow : err_ok;

	return result;
	}


	/*!
		inverse hyperbolic tantent
	*/
	template<class ValueType>
	ValueType ATgh(const ValueType & x, ErrorCode * err = 0)
	{
		return ATanh(x, err);
	}


	/*!
		inverse hyperbolic cotangent

		acoth(x) = 0.5 * ln( (x+1) / (x-1) )  x in (-infinity, -1) or (1, infinity)
	*/
	template<class ValueType>
	ValueType ACoth(const ValueType & x, ErrorCode * err = 0)
	{
		if( x.IsNan() )
		{
			if( err )
				*err = err_improper_argument;

		return x; // NaN
		}

		ValueType nominator(x), denominator(x), one, result;
		uint c = 0;
		one.SetOne();

		if( !x.GreaterWithoutSignThan(one) )
		{
			if( err )
				*err = err_improper_argument;

		return result; // NaN is set by default
		}

		c += nominator.Add(one);
		c += denominator.Sub(one);
		c += nominator.Div(denominator);
		c += result.Ln(nominator);
		c += result.exponent.SubOne();

		// here can only be a carry
		if( err )
			*err = c ? err_overflow : err_ok;

	return result;
	}


	/*!
		inverse hyperbolic cotantent
	*/
	template<class ValueType>
	ValueType ACtgh(const ValueType & x, ErrorCode * err = 0)
	{
		return ACoth(x, err);
	}





	/*
 	 *
	 *  functions for converting between degrees, radians and gradians
	 *
	 *
	 */


	/*!
		this function converts degrees to radians
		
		it returns: x * pi / 180
	*/
	template<class ValueType>
	ValueType DegToRad(const ValueType & x, ErrorCode * err = 0)
	{
	ValueType result, temp;
	uint c = 0;

		if( x.IsNan() )
		{
			if( err )
				*err = err_improper_argument;

		return x;
		}

		result = x;

		// it is better to make division first and then multiplication
		// the result is more accurate especially when x is: 90,180,270 or 360
		temp = 180;
		c += result.Div(temp);

		temp.SetPi();
		c += result.Mul(temp);

		if( err )
			*err = c ? err_overflow : err_ok;

	return result;
	}


	/*!
		this function converts radians to degrees
		
		it returns: x * 180 / pi
	*/
	template<class ValueType>
	ValueType RadToDeg(const ValueType & x, ErrorCode * err = 0)
	{
	ValueType result, delimiter;
	uint c = 0;

		if( x.IsNan() )
		{
			if( err )
				*err = err_improper_argument;

		return x;
		}

		result = 180;
		c += result.Mul(x);

		delimiter.SetPi();
		c += result.Div(delimiter);

		if( err )
			*err = c ? err_overflow : err_ok;

	return result;
	}


	/*!
		this function converts degrees in the long format into one value

		long format: (degrees, minutes, seconds)
		minutes and seconds must be greater than or equal zero

		result: 
		if d>=0 : result= d + ((s/60)+m)/60
		if d<0  : result= d - ((s/60)+m)/60

		((s/60)+m)/60 = (s+60*m)/3600 (second version is faster because 
		there's only one division)

		for example:
		DegToDeg(10, 30, 0) = 10.5
		DegToDeg(10, 24, 35.6)=10.4098(8)
	*/
	template<class ValueType>
	ValueType DegToDeg(	const ValueType & d, const ValueType & m, const ValueType & s,
						ErrorCode * err = 0)
	{
	ValueType delimiter, multipler;
	uint c = 0;

		if( d.IsNan() || m.IsNan() || s.IsNan() || m.IsSign() || s.IsSign() )
		{
			if( err )
				*err = err_improper_argument;

			delimiter.SetZeroNan(); // not needed, only to get rid of GCC warning about an uninitialized variable

		return delimiter;
		}

		multipler = 60;
		delimiter = 3600;

		c += multipler.Mul(m);
		c += multipler.Add(s);
		c += multipler.Div(delimiter);

		if( d.IsSign() )
			multipler.ChangeSign();

		c += multipler.Add(d);

		if( err )
			*err = c ? err_overflow : err_ok;

	return multipler;
	}


	/*!
		this function converts degrees in the long format to radians
	*/
	template<class ValueType>
	ValueType DegToRad(	const ValueType & d, const ValueType & m, const ValueType & s,
						ErrorCode * err = 0)
	{
		ValueType temp_deg = DegToDeg(d,m,s,err);

		if( err && *err!=err_ok )
			return temp_deg;

	return DegToRad(temp_deg, err);
	}


	/*!
		this function converts gradians to radians
		
		it returns: x * pi / 200
	*/
	template<class ValueType>
	ValueType GradToRad(const ValueType & x, ErrorCode * err = 0)
	{
	ValueType result, temp;
	uint c = 0;

		if( x.IsNan() )
		{
			if( err )
				*err = err_improper_argument;

		return x;
		}

		result = x;

		// it is better to make division first and then multiplication
		// the result is more accurate especially when x is: 100,200,300 or 400
		temp = 200;
		c += result.Div(temp);

		temp.SetPi();
		c += result.Mul(temp);

		if( err )
			*err = c ? err_overflow : err_ok;

	return result;
	}


	/*!
		this function converts radians to gradians
		
		it returns: x * 200 / pi
	*/
	template<class ValueType>
	ValueType RadToGrad(const ValueType & x, ErrorCode * err = 0)
	{
	ValueType result, delimiter;
	uint c = 0;

		if( x.IsNan() )
		{
			if( err )
				*err = err_improper_argument;

		return x;
		}

		result = 200;
		c += result.Mul(x);

		delimiter.SetPi();
		c += result.Div(delimiter);

		if( err )
			*err = c ? err_overflow : err_ok;

	return result;
	}


	/*!
		this function converts degrees to gradians
		
		it returns: x * 200 / 180
	*/
	template<class ValueType>
	ValueType DegToGrad(const ValueType & x, ErrorCode * err = 0)
	{
	ValueType result, temp;
	uint c = 0;

		if( x.IsNan() )
		{
			if( err )
				*err = err_improper_argument;

		return x;
		}

		result = x;

		temp = 200;
		c += result.Mul(temp);

		temp = 180;
		c += result.Div(temp);

		if( err )
			*err = c ? err_overflow : err_ok;

	return result;
	}


	/*!
		this function converts degrees in the long format to gradians
	*/
	template<class ValueType>
	ValueType DegToGrad( const ValueType & d, const ValueType & m, const ValueType & s,
						 ErrorCode * err = 0)
	{
		ValueType temp_deg = DegToDeg(d,m,s,err);

		if( err && *err!=err_ok )
			return temp_deg;

	return DegToGrad(temp_deg, err);
	}


	/*!
		this function converts degrees to gradians
		
		it returns: x * 180 / 200
	*/
	template<class ValueType>
	ValueType GradToDeg(const ValueType & x, ErrorCode * err = 0)
	{
	ValueType result, temp;
	uint c = 0;

		if( x.IsNan() )
		{
			if( err )
				*err = err_improper_argument;

		return x;
		}

		result = x;

		temp = 180;
		c += result.Mul(temp);

		temp = 200;
		c += result.Div(temp);

		if( err )
			*err = c ? err_overflow : err_ok;

	return result;
	}




	/*
 	 *
	 *  another functions
	 *
	 *
	 */


	/*!
		this function calculates the square root

		Sqrt(9) = 3
	*/
	template<class ValueType>
	ValueType Sqrt(ValueType x, ErrorCode * err = 0)
	{
		if( x.IsNan() || x.IsSign() )
		{
			if( err )
				*err = err_improper_argument;

			x.SetNan();

		return x;
		}

		uint c = x.Sqrt();

		if( err )
			*err = c ? err_overflow : err_ok;

	return x;
	}



	namespace auxiliaryfunctions
	{

	template<class ValueType>
	bool RootCheckIndexSign(ValueType & x, const ValueType & index, ErrorCode * err)
	{
		if( index.IsSign() )
		{
			// index cannot be negative
			if( err )
				*err = err_improper_argument;

			x.SetNan();

		return true;
		}

	return false;
	}


	template<class ValueType>
	bool RootCheckIndexZero(ValueType & x, const ValueType & index, ErrorCode * err)
	{
		if( index.IsZero() )
		{
			if( x.IsZero() )
			{
				// there isn't root(0;0) - we assume it's not defined
				if( err )
					*err = err_improper_argument;

				x.SetNan();

			return true;
			}
	
			// root(x;0) is 1 (if x!=0)
			x.SetOne();

			if( err )
				*err = err_ok;

		return true;
		}

	return false;
	}


	template<class ValueType>
	bool RootCheckIndexOne(const ValueType & index, ErrorCode * err)
	{
		ValueType one;
		one.SetOne();

		if( index == one )
		{
			//root(x;1) is x
			// we do it because if we used the PowFrac function
			// we would lose the precision
			if( err )
				*err = err_ok;

		return true;
		}

	return false;
	}


	template<class ValueType>
	bool RootCheckIndexTwo(ValueType & x, const ValueType & index, ErrorCode * err)
	{
		if( index == 2 )
		{
			x = Sqrt(x, err);

		return true;
		}

	return false;
	}


	template<class ValueType>
	bool RootCheckIndexFrac(ValueType & x, const ValueType & index, ErrorCode * err)
	{
		if( !index.IsInteger() )
		{
			// index must be integer
			if( err )
				*err = err_improper_argument;

			x.SetNan();

		return true;
		}

	return false;
	}


	template<class ValueType>
	bool RootCheckXZero(ValueType & x, ErrorCode * err)
	{
		if( x.IsZero() )
		{
			// root(0;index) is zero (if index!=0)
			// RootCheckIndexZero() must be called beforehand
			x.SetZero();

			if( err )
				*err = err_ok;

		return true;
		}

	return false;
	}


	template<class ValueType>
	bool RootCheckIndex(ValueType & x, const ValueType & index, ErrorCode * err, bool * change_sign)
	{
		*change_sign = false;

		if( index.Mod2() )
		{
			// index is odd (1,3,5...)
			if( x.IsSign() )
			{
				*change_sign = true;
				x.Abs();
			}
		}
		else
		{
			// index is even
			// x cannot be negative
			if( x.IsSign() )
			{
				if( err )
					*err = err_improper_argument;

				x.SetNan();

				return true;
			}
		}

	return false;
	}


	template<class ValueType>
	uint RootCorrectInteger(ValueType & old_x, ValueType & x, const ValueType & index)
	{
		if( !old_x.IsInteger() || x.IsInteger() || !index.exponent.IsSign() )
			return 0;

		// old_x is integer,
		// x is not integer,
		// index is relatively small (index.exponent<0 or index.exponent<=0)
		// (because we're using a special powering algorithm Big::PowUInt())

		uint c = 0;

		ValueType temp(x);
		c += temp.Round();

		ValueType temp_round(temp);
		c += temp.PowUInt(index);

		if( temp == old_x )
			x = temp_round;

	return (c==0)? 0 : 1;
	}



	} // namespace auxiliaryfunctions 



	/*!
		indexth Root of x
		index must be integer and not negative <0;1;2;3....)

		if index==0 the result is one
		if x==0 the result is zero and we assume root(0;0) is not defined

		if index is even (2;4;6...) the result is x^(1/index) and x>0
		if index is odd (1;2;3;...) the result is either 
			-(abs(x)^(1/index)) if x<0    or
			       x^(1/index)) if x>0

		(for index==1 the result is equal x)
	*/
	template<class ValueType>
	ValueType Root(ValueType x, const ValueType & index, ErrorCode * err = 0)
	{
		using namespace auxiliaryfunctions;

		if( x.IsNan() || index.IsNan() )
		{
			if( err )
				*err = err_improper_argument;

			x.SetNan();

		return x;
		}

		if( RootCheckIndexSign(x, index, err) ) return x;
		if( RootCheckIndexZero(x, index, err) ) return x;
		if( RootCheckIndexOne (   index, err) ) return x;
		if( RootCheckIndexTwo (x, index, err) ) return x;
		if( RootCheckIndexFrac(x, index, err) ) return x;
		if( RootCheckXZero    (x,        err) ) return x;

		// index integer and index!=0
		// x!=0

		ValueType old_x(x);
		bool change_sign;

		if( RootCheckIndex(x, index, err, &change_sign ) ) return x;

		ValueType temp;
		uint c = 0;

		// we're using the formula: root(x ; n) = exp( ln(x) / n )
		c += temp.Ln(x);
		c += temp.Div(index);
		c += x.Exp(temp);

		if( change_sign )
		{
			// x is different from zero
			x.SetSign();
		}

		c += RootCorrectInteger(old_x, x, index);

		if( err )
			*err = c ? err_overflow : err_ok;

	return x;
	}



	/*!
		absolute value of x
		e.g.  -2 = 2 
		       2 = 2
	*/
	template<class ValueType>
	ValueType Abs(const ValueType & x)
	{
		ValueType result( x );
		result.Abs();

	return result;
	}


	/*!
		it returns the sign of the value
		e.g.  -2 = -1 
		       0 = 0
		      10 = 1
	*/
	template<class ValueType>
	ValueType Sgn(ValueType x)
	{
		x.Sgn();

	return x;
	}


	/*!
		the remainder from a division

		e.g.
		mod( 12.6 ;  3) =  0.6   because 12.6  = 3*4 + 0.6
		mod(-12.6 ;  3) = -0.6   bacause -12.6 = 3*(-4) + (-0.6)
		mod( 12.6 ; -3) =  0.6
		mod(-12.6 ; -3) = -0.6
	*/
	template<class ValueType>
	ValueType Mod(ValueType a, const ValueType & b, ErrorCode * err = 0)
	{
		if( a.IsNan() || b.IsNan() )
		{
			if( err )
				*err = err_improper_argument;

			a.SetNan();

		return a;
		}

		uint c = a.Mod(b);

		if( err )
			*err = c ? err_overflow : err_ok;

	return a;
	}



	namespace auxiliaryfunctions
	{

	/*!
		this function is used to store factorials in a given container
		'more' means how many values should be added at the end

			e.g.
			std::vector<ValueType> fact;
			SetFactorialSequence(fact, 3);
			// now the container has three values: 1  1  2

			SetFactorialSequence(fact, 2);
			// now the container has five values:  1  1  2  6  24
	*/
	template<class ValueType>
	void SetFactorialSequence(std::vector<ValueType> & fact, uint more = 20)
	{
		if( more == 0 )
			more = 1;

		uint start = static_cast<uint>(fact.size());
		fact.resize(fact.size() + more);

		if( start == 0 )
		{
			fact[0] = 1;
			++start;
		}

		for(uint i=start ; i<fact.size() ; ++i)
		{
			fact[i] = fact[i-1];
			fact[i].MulInt(i);
		}
	}


	/*!
		an auxiliary function used to calculate Bernoulli numbers

		this function returns a sum:
		sum(m) = sum_{k=0}^{m-1} {2^k * (m k) * B(k)}    k in [0, m-1]   (m k) means binomial coefficient = (m! / (k! * (m-k)!))

		you should have sufficient factorials in cgamma.fact
		(cgamma.fact should have at least m items)

		n_ should be equal 2
	*/
	template<class ValueType>
	ValueType SetBernoulliNumbersSum(CGamma<ValueType> & cgamma, const ValueType & n_, uint m,
									  const volatile StopCalculating * stop = 0)
	{
	ValueType k_, temp, temp2, temp3, sum;

		sum.SetZero();
		
		for(uint k=0 ; k<m ; ++k)			// k<m means k<=m-1
		{
			if( stop && (k & 15)==0 )		// means: k % 16 == 0
				if( stop->WasStopSignal() )
					return ValueType();		// NaN

			if( k>1 && (k & 1) == 1 )		// for that k the Bernoulli number is zero
				continue;

			k_ = k;

			temp = n_;				// n_ is equal 2
			temp.Pow(k_);
			// temp = 2^k

			temp2 = cgamma.fact[m];
			temp3 = cgamma.fact[k];
			temp3.Mul(cgamma.fact[m-k]);
			temp2.Div(temp3);
			// temp2 = (m k) = m! / ( k! * (m-k)! )

			temp.Mul(temp2);
			temp.Mul(cgamma.bern[k]);

			sum.Add(temp);
			// sum += 2^k * (m k) * B(k)

			if( sum.IsNan() )
				break;
		}

	return sum;
	}


	/*!
		an auxiliary function used to calculate Bernoulli numbers
		start is >= 2

		we use the recurrence formula: 
		   B(m) = 1 / (2*(1 - 2^m)) * sum(m)
		   where sum(m) is calculated by SetBernoulliNumbersSum()
	*/
	template<class ValueType>
	bool SetBernoulliNumbersMore(CGamma<ValueType> & cgamma, uint start, const volatile StopCalculating * stop = 0)
	{
	ValueType denominator, temp, temp2, temp3, m_, sum, sum2, n_, k_;

		const uint n = 2;
		n_ = n;

		// start is >= 2
		for(uint m=start ; m<cgamma.bern.size() ; ++m)
		{
			if( (m & 1) == 1 )
			{
				cgamma.bern[m].SetZero();
			}
			else
			{
				m_ = m;

				temp = n_;				// n_ = 2
				temp.Pow(m_);
				// temp = 2^m

				denominator.SetOne();
				denominator.Sub(temp);
				if( denominator.exponent.AddOne() ) // it means: denominator.MulInt(2)
					denominator.SetNan();

				// denominator = 2 * (1 - 2^m)

				cgamma.bern[m] = SetBernoulliNumbersSum(cgamma, n_, m, stop);

				if( stop && stop->WasStopSignal() )
				{
					cgamma.bern.resize(m);		// valid numbers are in [0, m-1]
					return false;
				}

				cgamma.bern[m].Div(denominator);
			}
		}

	return true;
	}


	/*!
		this function is used to calculate Bernoulli numbers,
		returns false if there was a stop signal,
		'more' means how many values should be added at the end

			e.g.
			typedef Big<1,2> MyBig;
			CGamma<MyBig> cgamma;
			SetBernoulliNumbers(cgamma, 3);
			// now we have three first Bernoulli numbers:  1  -0.5  0.16667
			
			SetBernoulliNumbers(cgamma, 4);
			// now we have 7 Bernoulli numbers:  1  -0.5  0.16667   0   -0.0333   0   0.0238
	*/
	template<class ValueType>
	bool SetBernoulliNumbers(CGamma<ValueType> & cgamma, uint more = 20, const volatile StopCalculating * stop = 0)
	{
		if( more == 0 )
			more = 1;

		uint start = static_cast<uint>(cgamma.bern.size());
		cgamma.bern.resize(cgamma.bern.size() + more);

		if( start == 0 )
		{
			cgamma.bern[0].SetOne();
			++start;
		}

		if( cgamma.bern.size() == 1 )
			return true;

		if( start == 1 )
		{
			cgamma.bern[1].Set05();
			cgamma.bern[1].ChangeSign();
			++start;
		}

		// we should have sufficient factorials in cgamma.fact
		if( cgamma.fact.size() < cgamma.bern.size() )
			SetFactorialSequence(cgamma.fact, static_cast<uint>(cgamma.bern.size() - cgamma.fact.size()));


	return SetBernoulliNumbersMore(cgamma, start, stop);
	}


	/*!
		an auxiliary function used to calculate the Gamma() function

		we calculate a sum:
		   sum(n) = sum_{m=2} { B(m) / ( (m^2 - m) * n^(m-1) )  } = 1/(12*n) - 1/(360*n^3) + 1/(1260*n^5) + ...
	       B(m) means a mth Bernoulli number
		   the sum starts from m=2, we calculate as long as the value will not change after adding a next part
	*/
	template<class ValueType>
	ValueType GammaFactorialHighSum(const ValueType & n, CGamma<ValueType> & cgamma, ErrorCode & err,
									const volatile StopCalculating * stop)
	{
	ValueType temp, temp2, denominator, sum, oldsum;

		sum.SetZero();

		for(uint m=2 ; m<TTMATH_ARITHMETIC_MAX_LOOP ; m+=2)
		{
			if( stop && (m & 3)==0 ) // (m & 3)==0 means: (m % 4)==0
				if( stop->WasStopSignal() )
				{
					err = err_interrupt;
					return ValueType(); // NaN
				}

			temp = (m-1);
			denominator = n;
			denominator.Pow(temp);
			// denominator = n ^ (m-1)

			temp = m;
			temp2 = temp;
			temp.Mul(temp2);
			temp.Sub(temp2);
			// temp = m^2 - m

			denominator.Mul(temp);
			// denominator = (m^2 - m) * n ^ (m-1)

			if( m >= cgamma.bern.size() )
			{
				if( !SetBernoulliNumbers(cgamma, m - cgamma.bern.size() + 1 + 3, stop) ) // 3 more than needed
				{
					// there was the stop signal
					err = err_interrupt;
					return ValueType(); // NaN
				}
			}

			temp = cgamma.bern[m];
			temp.Div(denominator);

			oldsum = sum;
			sum.Add(temp);

			if( sum.IsNan() || oldsum==sum )
				break;
		}

	return sum;
	}


	/*!
		an auxiliary function used to calculate the Gamma() function

		we calculate a helper function GammaFactorialHigh() by using Stirling's series:
		   n! = (n/e)^n * sqrt(2*pi*n) * exp( sum(n) )
		   where n is a real number (not only an integer) and is sufficient large (greater than TTMATH_GAMMA_BOUNDARY)
		   and sum(n) is calculated by GammaFactorialHighSum()
	*/
	template<class ValueType>
	ValueType GammaFactorialHigh(const ValueType & n, CGamma<ValueType> & cgamma, ErrorCode & err,
								 const volatile StopCalculating * stop)
	{
	ValueType temp, temp2, temp3, denominator, sum;

		temp.Set2Pi();
		temp.Mul(n);
		temp2 = Sqrt(temp);
		// temp2 = sqrt(2*pi*n)

		temp = n;
		temp3.SetE();
		temp.Div(temp3);
		temp.Pow(n);
		// temp = (n/e)^n

		sum = GammaFactorialHighSum(n, cgamma, err, stop);
		temp3.Exp(sum);
		// temp3 = exp(sum)

		temp.Mul(temp2);
		temp.Mul(temp3);

	return temp;
	}


	/*!
		an auxiliary function used to calculate the Gamma() function

		Gamma(x) = GammaFactorialHigh(x-1)
	*/
	template<class ValueType>
	ValueType GammaPlusHigh(ValueType n, CGamma<ValueType> & cgamma, ErrorCode & err, const volatile StopCalculating * stop)
	{
	ValueType one;

		one.SetOne();
		n.Sub(one);

	return GammaFactorialHigh(n, cgamma, err, stop);
	}


	/*!
		an auxiliary function used to calculate the Gamma() function
	
		we use this function when n is integer and a small value (from 0 to TTMATH_GAMMA_BOUNDARY]
		we use the formula:
		   gamma(n) = (n-1)! = 1 * 2 * 3 * ... * (n-1) 
	*/
	template<class ValueType>
	ValueType GammaPlusLowIntegerInt(uint n, CGamma<ValueType> & cgamma)
	{
		TTMATH_ASSERT( n > 0 )

		if( n - 1 < static_cast<uint>(cgamma.fact.size()) )
			return cgamma.fact[n - 1];

		ValueType res;
		uint start = 2;

		if( cgamma.fact.size() < 2 )
		{
			res.SetOne();
		}
		else
		{
			start = static_cast<uint>(cgamma.fact.size());
			res   = cgamma.fact[start-1];
		}

		for(uint i=start ; i<n ; ++i)
			res.MulInt(i);

	return res;
	}
	

	/*!
		an auxiliary function used to calculate the Gamma() function

		we use this function when n is integer and a small value (from 0 to TTMATH_GAMMA_BOUNDARY]
	*/
	template<class ValueType>
	ValueType GammaPlusLowInteger(const ValueType & n, CGamma<ValueType> & cgamma)
	{
	sint n_;

		n.ToInt(n_);

	return GammaPlusLowIntegerInt(n_, cgamma);
	}


	/*!
		an auxiliary function used to calculate the Gamma() function

		we use this function when n is a small value (from 0 to TTMATH_GAMMA_BOUNDARY]
		we use a recurrence formula:
		   gamma(z+1) = z * gamma(z)
		   then: gamma(z) = gamma(z+1) / z

		   e.g.
		   gamma(3.89) = gamma(2001.89) / ( 3.89 * 4.89 * 5.89 * ... * 1999.89 * 2000.89 )
	*/
	template<class ValueType>
	ValueType GammaPlusLow(ValueType n, CGamma<ValueType> & cgamma, ErrorCode & err, const volatile StopCalculating * stop)
	{
	ValueType one, denominator, temp, boundary;

		if( n.IsInteger() )
			return GammaPlusLowInteger(n, cgamma);

		one.SetOne();
		denominator = n;
		boundary    = TTMATH_GAMMA_BOUNDARY;

		while( n < boundary )
		{
			n.Add(one);
			denominator.Mul(n);
		}

		n.Add(one);

		// now n is sufficient big
		temp = GammaPlusHigh(n, cgamma, err, stop);
		temp.Div(denominator);

	return temp;
	}


	/*!
		an auxiliary function used to calculate the Gamma() function
	*/
	template<class ValueType>
	ValueType GammaPlus(const ValueType & n, CGamma<ValueType> & cgamma, ErrorCode & err, const volatile StopCalculating * stop)
	{
		if( n > TTMATH_GAMMA_BOUNDARY )
			return GammaPlusHigh(n, cgamma, err, stop);

	return GammaPlusLow(n, cgamma, err, stop);
	}


	/*!
		an auxiliary function used to calculate the Gamma() function

		this function is used when n is negative
		we use the reflection formula:
		   gamma(1-z) * gamma(z) = pi / sin(pi*z)
		   then: gamma(z) = pi / (sin(pi*z) * gamma(1-z))

	*/
	template<class ValueType>
	ValueType GammaMinus(const ValueType & n, CGamma<ValueType> & cgamma, ErrorCode & err, const volatile StopCalculating * stop)
	{
	ValueType pi, denominator, temp, temp2;

		if( n.IsInteger() )
		{
			// gamma function is not defined when n is negative and integer
			err = err_improper_argument;
			return temp; // NaN
		}

		pi.SetPi();

		temp = pi;
		temp.Mul(n);
		temp2 = Sin(temp);
		// temp2 = sin(pi * n)

		temp.SetOne();
		temp.Sub(n);
		temp = GammaPlus(temp, cgamma, err, stop);
		// temp = gamma(1 - n)

		temp.Mul(temp2);
		pi.Div(temp);

	return pi;
	}

	} // namespace auxiliaryfunctions



	/*!
		this function calculates the Gamma function

		it's multithread safe, you should create a CGamma<> object and use it whenever you call the Gamma()
		e.g.
			typedef Big<1,2> MyBig;
			MyBig x=234, y=345.53;
			CGamma<MyBig> cgamma;
			std::cout << Gamma(x, cgamma) << std::endl;
			std::cout << Gamma(y, cgamma) << std::endl;
		in the CGamma<> object the function stores some coefficients (factorials, Bernoulli numbers),
		and they will be reused in next calls to the function

		each thread should have its own CGamma<> object, and you can use these objects with Factorial() function too
	*/
	template<class ValueType>
	ValueType Gamma(const ValueType & n, CGamma<ValueType> & cgamma, ErrorCode * err = 0,
					const volatile StopCalculating * stop = 0)
	{
	using namespace auxiliaryfunctions;

	ValueType result;
	ErrorCode err_tmp;

		if( n.IsNan() )
		{
			if( err )
				*err = err_improper_argument;

		return n;
		}

		if( cgamma.history.Get(n, result, err_tmp) )
		{
			if( err )
				*err = err_tmp;

			return result;
		}

		err_tmp = err_ok;

		if( n.IsSign() )
		{
			result = GammaMinus(n, cgamma, err_tmp, stop);
		}
		else
		if( n.IsZero() )
		{
			err_tmp = err_improper_argument;
			result.SetNan();
		}
		else
		{
			result = GammaPlus(n, cgamma, err_tmp, stop);
		}

		if( result.IsNan() && err_tmp==err_ok )
			err_tmp = err_overflow;

		if( err )
			*err = err_tmp;

		if( stop && !stop->WasStopSignal() )
			cgamma.history.Add(n, result, err_tmp);

	return result;
	}


	/*!
		this function calculates the Gamma function

		note: this function should be used only in a single-thread environment
	*/
	template<class ValueType>
	ValueType Gamma(const ValueType & n, ErrorCode * err = 0)
	{
	// warning: this static object is not thread safe
	static CGamma<ValueType> cgamma;

	return Gamma(n, cgamma, err);
	}



	namespace auxiliaryfunctions
	{

	/*!
		an auxiliary function for calculating the factorial function

		we use the formula:
		   x! = gamma(x+1)
	*/
	template<class ValueType>
	ValueType Factorial2(ValueType x,
						 CGamma<ValueType> * cgamma = 0,
						 ErrorCode * err = 0,
						 const volatile StopCalculating * stop = 0)
	{
	ValueType result, one;

		if( x.IsNan() || x.IsSign() || !x.IsInteger() )
		{
			if( err )
				*err = err_improper_argument;

			x.SetNan();

		return x;
		}

		one.SetOne();
		x.Add(one);

		if( cgamma )
			return Gamma(x, *cgamma, err, stop);

	return Gamma(x, err);
	}
	
	} // namespace auxiliaryfunctions



	/*!
		the factorial from given 'x'
		e.g.
		Factorial(4) = 4! = 1*2*3*4

		it's multithread safe, you should create a CGamma<> object and use it whenever you call the Factorial()
		e.g.
			typedef Big<1,2> MyBig;
			MyBig x=234, y=54345;
			CGamma<MyBig> cgamma;
			std::cout << Factorial(x, cgamma) << std::endl;
			std::cout << Factorial(y, cgamma) << std::endl;
		in the CGamma<> object the function stores some coefficients (factorials, Bernoulli numbers),
		and they will be reused in next calls to the function

		each thread should have its own CGamma<> object, and you can use these objects with Gamma() function too
	*/
	template<class ValueType>
	ValueType Factorial(const ValueType & x, CGamma<ValueType> & cgamma, ErrorCode * err = 0,
						const volatile StopCalculating * stop = 0)
	{
		return auxiliaryfunctions::Factorial2(x, &cgamma, err, stop);
	}


	/*!
		the factorial from given 'x'
		e.g.
		Factorial(4) = 4! = 1*2*3*4

		note: this function should be used only in a single-thread environment
	*/
	template<class ValueType>
	ValueType Factorial(const ValueType & x, ErrorCode * err = 0)
	{
		return auxiliaryfunctions::Factorial2(x, (CGamma<ValueType>*)0, err, 0);
	}


	/*!
		this method prepares some coefficients: factorials and Bernoulli numbers
		stored in 'fact' and 'bern' objects

		we're defining the method here because we're using Gamma() function which
		is not available in ttmathobjects.h

		read the doc info in ttmathobjects.h file where CGamma<> struct is declared
	*/
	template<class ValueType>
	void CGamma<ValueType>::InitAll()
	{
		ValueType x = TTMATH_GAMMA_BOUNDARY + 1;
		
		// history.Remove(x) removes only one object
		// we must be sure that there are not others objects with the key 'x'
		while( history.Remove(x) )
		{
		}

		// the simplest way to initialize is to call the Gamma function with (TTMATH_GAMMA_BOUNDARY + 1)
		// when x is larger then fewer coefficients we need
		Gamma(x, *this);
	}



} // namespace


/*!
	this is for convenience for the user
	he can only use '#include <ttmath/ttmath.h>' even if he uses the parser
*/
#include "ttmathparser.h"


#ifdef _MSC_VER
//warning C4127: conditional expression is constant
#pragma warning( default: 4127 )
//warning C4702: unreachable code
#pragma warning( default: 4702 )
//warning C4800: forcing value to bool 'true' or 'false' (performance warning)
#pragma warning( default: 4800 )
#endif

#endif
