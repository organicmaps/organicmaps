/*
 * This file is a part of TTMath Mathematical Library
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


#ifndef headerfilettmathobject
#define headerfilettmathobject

/*!
	\file ttmathobjects.h
    \brief Mathematic functions.
*/

#include <string>
#include <vector>
#include <list>
#include <map>

#include "ttmathtypes.h"
#include "ttmathmisc.h"


namespace ttmath
{

/*!
	objects of this class are used with the mathematical parser
	they hold variables or functions defined by a user

	each object has its own table in which we're keeping variables or functions
*/
class Objects
{
public:


	/*!
		one item (variable or function)
		'items' will be on the table
	*/
	struct Item
	{
		// name of a variable of a function
		// internally we store variables and funcions as std::string (not std::wstring even when wide characters are used)
		std::string value;

		// number of parameters required by the function
		// (if there's a variable this 'param' is ignored)
		int param;

		Item() {}
		Item(const std::string & v, int p) : value(v), param(p) {}
	};

	// 'Table' is the type of our table
	typedef std::map<std::string, Item> Table;
	typedef	Table::iterator Iterator;
	typedef	Table::const_iterator CIterator;



	/*!
		this method returns true if a character 'c' is a character
		which can be in a name
		
		if 'can_be_digit' is true that means when the 'c' is a digit this 
		method returns true otherwise it returns false
	*/
	static bool CorrectCharacter(int c, bool can_be_digit)
	{
		if( (c>='a' && c<='z') || (c>='A' && c<='Z') )
			return true;

		if( can_be_digit && ((c>='0' && c<='9') || c=='_') )
			return true;

	return false;
	}


	/*!
		this method returns true if the name can be as a name of an object
	*/
	template<class string_type>
	static bool IsNameCorrect(const string_type & name)
	{
		if( name.empty() )
			return false;

		if( !CorrectCharacter(name[0], false) )
			return false;

		typename string_type::const_iterator i = name.begin();

		for(++i ; i!=name.end() ; ++i)
			if( !CorrectCharacter(*i, true) )
				return false;
		
	return true;
	}


	/*!
		this method returns true if such an object is defined (name exists)
	*/
	bool IsDefined(const std::string & name)
	{
		Iterator i = table.find(name);

		if( i != table.end() )
			// we have this object in our table
			return true;

	return false;
	}



#ifndef TTMATH_DONT_USE_WCHAR

	/*!
		this method returns true if such an object is defined (name exists)
	*/
	bool IsDefined(const std::wstring & name)
	{
		// we should check whether the name (in wide characters) are correct
		// before calling AssignString() function
		if( !IsNameCorrect(name) )
			return false;

		Misc::AssignString(str_tmp1, name);

	return IsDefined(str_tmp1);
	}

#endif


	/*!
		this method adds one object (variable of function) into the table
	*/
	ErrorCode Add(const std::string & name, const std::string & value, int param = 0)
	{
		if( !IsNameCorrect(name) )
			return err_incorrect_name;

		Iterator i = table.find(name);

		if( i != table.end() )
			// we have this object in our table
			return err_object_exists;

		table.insert( std::make_pair(name, Item(value, param)) );

	return err_ok;
	}


#ifndef TTMATH_DONT_USE_WCHAR

	/*!
		this method adds one object (variable of function) into the table
	*/
	ErrorCode Add(const std::wstring & name, const std::wstring & value, int param = 0)
	{
		// we should check whether the name (in wide characters) are correct
		// before calling AssignString() function
		if( !IsNameCorrect(name) )
			return err_incorrect_name;

		Misc::AssignString(str_tmp1, name);
		Misc::AssignString(str_tmp2, value);
		
	return Add(str_tmp1, str_tmp2, param);
	}

#endif


	/*!
		this method returns 'true' if the table is empty
	*/
	bool Empty() const
	{
		return table.empty();
	}


	/*!
		this method clears the table
	*/
	void Clear()
	{
		return table.clear();
	}


	/*!
		this method returns 'const_iterator' on the first item on the table
	*/
	CIterator Begin() const
	{
		return table.begin();
	}


	/*!
		this method returns 'const_iterator' pointing at the space after last item
		(returns table.end())
	*/
	CIterator End() const
	{
		return table.end();
	}


	/*!
		this method changes the value and the number of parameters for a specific object
	*/
	ErrorCode EditValue(const std::string & name, const std::string & value, int param = 0)
	{
		if( !IsNameCorrect(name) )
			return err_incorrect_name;

		Iterator i = table.find(name);

		if( i == table.end() )
			return err_unknown_object;
	
		i->second.value = value;
		i->second.param = param;

	return err_ok;
	}


#ifndef TTMATH_DONT_USE_WCHAR


	/*!
		this method changes the value and the number of parameters for a specific object
	*/
	ErrorCode EditValue(const std::wstring & name, const std::wstring & value, int param = 0)
	{
		// we should check whether the name (in wide characters) are correct
		// before calling AssignString() function
		if( !IsNameCorrect(name) )
			return err_incorrect_name;

		Misc::AssignString(str_tmp1, name);
		Misc::AssignString(str_tmp2, value);
		
	return EditValue(str_tmp1, str_tmp2, param);
	}

#endif


	/*!
		this method changes the name of a specific object
	*/
	ErrorCode EditName(const std::string & old_name, const std::string & new_name)
	{
		if( !IsNameCorrect(old_name) || !IsNameCorrect(new_name) )
			return err_incorrect_name;

		Iterator old_i = table.find(old_name);
		if( old_i == table.end() )
			return err_unknown_object;
		
		if( old_name == new_name )
			// the new name is the same as the old one
			// we treat it as a normal situation
			return err_ok;

		ErrorCode err = Add(new_name, old_i->second.value, old_i->second.param);
		
		if( err == err_ok ) 
		{
			old_i = table.find(old_name);
			TTMATH_ASSERT( old_i != table.end() )

			table.erase(old_i);
		}

	return err;
	}



#ifndef TTMATH_DONT_USE_WCHAR


	/*!
		this method changes the name of a specific object
	*/
	ErrorCode EditName(const std::wstring & old_name, const std::wstring & new_name)
	{
		// we should check whether the name (in wide characters) are correct
		// before calling AssignString() function
		if( !IsNameCorrect(old_name) || !IsNameCorrect(new_name) )
			return err_incorrect_name;

		Misc::AssignString(str_tmp1, old_name);
		Misc::AssignString(str_tmp2, new_name);

	return EditName(str_tmp1, str_tmp2);
	}

#endif


	/*!
		this method deletes an object
	*/
	ErrorCode Delete(const std::string & name)
	{
		if( !IsNameCorrect(name) )
			return err_incorrect_name;

		Iterator i = table.find(name);

		if( i == table.end() )
			return err_unknown_object;

		table.erase( i );

	return err_ok;
	}


#ifndef TTMATH_DONT_USE_WCHAR


	/*!
		this method deletes an object
	*/
	ErrorCode Delete(const std::wstring & name)
	{
		// we should check whether the name (in wide characters) are correct
		// before calling AssignString() function
		if( !IsNameCorrect(name) )
			return err_incorrect_name;

		Misc::AssignString(str_tmp1, name);

	return Delete(str_tmp1);
	}	
		
#endif


	/*!
		this method gets the value of a specific object
	*/
	ErrorCode GetValue(const std::string & name, std::string & value) const
	{
		if( !IsNameCorrect(name) )
			return err_incorrect_name;

		CIterator i = table.find(name);

		if( i == table.end() )
		{
			value.clear();
			return err_unknown_object;
		}

		value = i->second.value;

	return err_ok;
	}


#ifndef TTMATH_DONT_USE_WCHAR

	/*!
		this method gets the value of a specific object
	*/
	ErrorCode GetValue(const std::wstring & name, std::wstring & value)
	{
		// we should check whether the name (in wide characters) are correct
		// before calling AssignString() function
		if( !IsNameCorrect(name) )
			return err_incorrect_name;

		Misc::AssignString(str_tmp1, name);
		ErrorCode err = GetValue(str_tmp1, str_tmp2);
		Misc::AssignString(value, str_tmp2);

	return err;
	}

#endif


	/*!
		this method gets the value of a specific object
		(this version is used for not copying the whole string)
	*/
	ErrorCode GetValue(const std::string & name, const char ** value) const
	{
		if( !IsNameCorrect(name) )
			return err_incorrect_name;

		CIterator i = table.find(name);

		if( i == table.end() )
		{
			*value = 0;
			return err_unknown_object;
		}

		*value = i->second.value.c_str();

	return err_ok;
	}


#ifndef TTMATH_DONT_USE_WCHAR

	/*!
		this method gets the value of a specific object
		(this version is used for not copying the whole string)
	*/
	ErrorCode GetValue(const std::wstring & name, const char ** value)
	{
		// we should check whether the name (in wide characters) are correct
		// before calling AssignString() function
		if( !IsNameCorrect(name) )
			return err_incorrect_name;

		Misc::AssignString(str_tmp1, name);

	return GetValue(str_tmp1, value);
	}

#endif


	/*!
		this method gets the value and the number of parameters
		of a specific object
	*/
	ErrorCode GetValueAndParam(const std::string & name, std::string & value, int * param) const
	{
		if( !IsNameCorrect(name) )
			return err_incorrect_name;

		CIterator i = table.find(name);

		if( i == table.end() )
		{
			value.empty();
			*param = 0;
			return err_unknown_object;
		}

		value = i->second.value;
		*param = i->second.param;

	return err_ok;
	}


#ifndef TTMATH_DONT_USE_WCHAR

	/*!
		this method gets the value and the number of parameters
		of a specific object
	*/
	ErrorCode GetValueAndParam(const std::wstring & name, std::wstring & value, int * param)
	{
		// we should check whether the name (in wide characters) are correct
		// before calling AssignString() function
		if( !IsNameCorrect(name) )
			return err_incorrect_name;

		Misc::AssignString(str_tmp1, name);
		ErrorCode err = GetValueAndParam(str_tmp1, str_tmp2, param);
		Misc::AssignString(value, str_tmp2);

	return err;
	}

#endif


	/*!
		this method sets the value and the number of parameters
		of a specific object
		(this version is used for not copying the whole string)
	*/
	ErrorCode GetValueAndParam(const std::string & name, const char ** value, int * param) const
	{
		if( !IsNameCorrect(name) )
			return err_incorrect_name;

		CIterator i = table.find(name);

		if( i == table.end() )
		{
			*value = 0;
			*param = 0;
			return err_unknown_object;
		}

		*value = i->second.value.c_str();
		*param = i->second.param;

	return err_ok;
	}


#ifndef TTMATH_DONT_USE_WCHAR


	/*!
		this method sets the value and the number of parameters
		of a specific object
		(this version is used for not copying the whole string
		but in fact we make one copying during AssignString())
	*/
	ErrorCode GetValueAndParam(const std::wstring & name, const char ** value, int * param)
	{
		// we should check whether the name (in wide characters) are correct
		// before calling AssignString() function
		if( !IsNameCorrect(name) )
			return err_incorrect_name;

		Misc::AssignString(str_tmp1, name);

	return GetValueAndParam(str_tmp1, value, param);
	}


#endif


	/*!
		this method returns a pointer into the table
	*/
	Table * GetTable()
	{
		return &table;
	}


private:

	Table table;
	std::string str_tmp1, str_tmp2;

}; // end of class Objects







/*!
	objects of the class History are used to keep values in functions
	which take a lot of time during calculating, for instance in the 
	function Factorial(x)

	it means that when we're calculating e.g. Factorial(1000) and the 
	Factorial finds that we have calculated it before, the value (result)
	is taken from the history
*/
template<class ValueType>
class History
{
	/*!
		one item in the History's object holds a key, a value for the key
		and a corresponding error code
	*/
	struct Item
	{
		ValueType key, value;
		ErrorCode err;
	};


	/*!
		we use std::list for simply deleting the first item
		but because we're searching through the whole container
		(in the method Get) the container should not be too big
		(linear time of searching)
	*/
	typedef std::list<Item> buffer_type;
	buffer_type buffer;
	typename buffer_type::size_type buffer_max_size;

public:
	
	/*!
		default constructor
		default max size of the History's container is 15 items
	*/
	History()
	{
		buffer_max_size = 15;
	}


	/*!
		a constructor which takes another value of the max size
		of the History's container
	*/
	History(typename buffer_type::size_type new_size)
	{
		buffer_max_size = new_size;
	}


	/*!
		this method adds one item into the History
		if the size of the container is greater than buffer_max_size
		the first item will be removed
	*/
	void Add(const ValueType & key, const ValueType & value, ErrorCode err)
	{
		Item item;
		item.key   = key;
		item.value = value;
		item.err   = err;

		buffer.insert( buffer.end(), item );

		if( buffer.size() > buffer_max_size )
			buffer.erase(buffer.begin());
	}


	/*!
		this method checks whether we have an item which has the key equal 'key'

		if there's such item the method sets the 'value' and the 'err'
		and returns true otherwise it returns false and 'value' and 'err'
		remain unchanged
	*/
	bool Get(const ValueType & key, ValueType & value, ErrorCode & err)
	{
		typename buffer_type::iterator i = buffer.begin();

		for( ; i != buffer.end() ; ++i )
		{
			if( i->key == key )
			{
				value = i->value;
				err   = i->err;
				return true;
			}
		}

	return false;
	}


	/*!
		this methods deletes an item

		we assume that there is only one item with the 'key'
		(this methods removes the first one)
	*/
	bool Remove(const ValueType & key)
	{
		typename buffer_type::iterator i = buffer.begin();

		for( ; i != buffer.end() ; ++i )
		{
			if( i->key == key )
			{
				buffer.erase(i);
				return true;
			}
		}

	return false;
	}


}; // end of class History



/*!
	this is an auxiliary class used when calculating Gamma() or Factorial()

	in multithreaded environment you can provide an object of this class to
	the Gamma() or Factorial() function, e.g;
		typedef Big<1, 3> MyBig;
		MyBig x = 123456;
		CGamma<MyBig> cgamma;
		std::cout << Gamma(x, cgamma);
	each thread should have its own CGamma<> object

	in a single-thread environment a CGamma<> object is a static variable
	in a second version of Gamma() and you don't have to explicitly use it, e.g.
		typedef Big<1, 3> MyBig;
		MyBig x = 123456;
		std::cout << Gamma(x);
*/
template<class ValueType>
struct CGamma
{
	/*!
		this table holds factorials
			1
			1
			2
			6
			24
			120
			720
			.......
	*/
	std::vector<ValueType> fact;


	/*!
		this table holds Bernoulli numbers
			1
			-0.5
			0.166666666666666666666666667
			0
			-0.0333333333333333333333333333
			0
			0.0238095238095238095238095238
			0
			-0.0333333333333333333333333333
			0
			0.075757575757575757575757576
			.....
	*/
	std::vector<ValueType> bern;


	/*!
		here we store some calculated values
		(this is for speeding up, if the next argument of Gamma() or Factorial()
		is in the 'history' then the result we are not calculating but simply
		return from the 'history' object)
	*/
	History<ValueType> history;


	/*!
		this method prepares some coefficients: factorials and Bernoulli numbers
		stored in 'fact' and 'bern' objects
		
		how many values should be depends on the size of the mantissa - if
		the mantissa is larger then we must calculate more values
		    for a mantissa which consists of 256 bits (8 words on a 32bit platform)
			we have to calculate about 30 values (the size of fact and bern will be 30),
			and for a 2048 bits mantissa we have to calculate 306 coefficients

		you don't have to call this method, these coefficients will be automatically calculated
		when they are needed

		you must note that calculating these coefficients is a little time-consuming operation,
		(especially when the mantissa is large) and first call to Gamma() or Factorial()
		can take more time than next calls, and in the end this is the point when InitAll()
		comes in handy: you can call this method somewhere at the beginning of your program
	*/
	void InitAll();
	// definition is in ttmath.h
};




} // namespace

#endif
