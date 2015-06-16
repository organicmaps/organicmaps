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

#ifndef headerfilettmathmisc
#define headerfilettmathmisc


/*!
	\file ttmathmisc.h
    \brief some helpful functions
*/


#include <string>


namespace ttmath
{

/*!
	some helpful functions
*/
class Misc
{
public:


/*
 *
 *	AssignString(result, str)
 *	result = str
 *
 */

/*!
	result = str
*/
static void AssignString(std::string & result, const char * str)
{
	result = str;
}


#ifndef TTMATH_DONT_USE_WCHAR

/*!
	result = str
*/
static void AssignString(std::wstring & result, const char * str)
{
	result.clear();

	for( ; *str ; ++str )
		result += *str;
}


/*!
	result = str
*/
static void AssignString(std::wstring & result, const std::string & str)
{
	return AssignString(result, str.c_str());
}


/*!
	result = str
*/
static void AssignString(std::string & result, const wchar_t * str)
{
	result.clear();

	for( ; *str ; ++str )
		result += static_cast<char>(*str);
}


/*!
	result = str
*/
static void AssignString(std::string & result, const std::wstring & str)
{
	return AssignString(result, str.c_str());
}

#endif


/*
 *
 *	AddString(result, str)
 *	result += str
 *
 */


/*!
	result += str
*/
static void AddString(std::string & result, const char * str)
{
	result += str;
}


#ifndef TTMATH_DONT_USE_WCHAR

/*!
	result += str
*/
static void AddString(std::wstring & result, const char * str)
{
	for( ; *str ; ++str )
		result += *str;
}

#endif


/*
	this method omits any white characters from the string
	char_type is char or wchar_t
*/
template<class char_type>
static void SkipWhiteCharacters(const char_type * & c)
{
	// 13 is at the end in a DOS text file (\r\n)
	while( (*c==' ' ) || (*c=='\t') || (*c==13 ) || (*c=='\n') )
		++c;
}




/*!
	this static method converts one character into its value

	for example:
		1 -> 1
		8 -> 8
		A -> 10
		f -> 15

	this method don't check whether c is correct or not
*/
static uint CharToDigit(uint c)
{
	if(c>='0' && c<='9')
		return c-'0';

	if(c>='a' && c<='z')
		return c-'a'+10;

return c-'A'+10;
}


/*!
	this method changes a character 'c' into its value
	(if there can't be a correct value it returns -1)

	for example:
	c=2, base=10 -> function returns 2
	c=A, base=10 -> function returns -1
	c=A, base=16 -> function returns 10
*/
static sint CharToDigit(uint c, uint base)
{
	if( c>='0' && c<='9' )
		c=c-'0';
	else
	if( c>='a' && c<='z' )
		c=c-'a'+10;
	else
	if( c>='A' && c<='Z' )
		c=c-'A'+10;
	else
		return -1;


	if( c >= base )
		return -1;


return sint(c);
}



/*!
	this method converts a digit into a char
	digit should be from <0,F>
	(we don't have to get a base)
	
	for example:
		1  -> 1
		8  -> 8
		10 -> A
		15 -> F
*/
static uint DigitToChar(uint digit)
{
	if( digit < 10 )
		return digit + '0';

return digit - 10 + 'A';
}


}; // struct Misc

}


#endif
