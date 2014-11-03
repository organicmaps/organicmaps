///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Image Copyright (c) 2008 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2008-12-19
// Updated : 2005-06-13
// Licence : This source is under MIT License
// File    : gli/shared_array.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace gli
{
	template <typename T>
	shared_array<T>::shared_array() :
		Counter(0),
		Pointer(0)
	{}

	template <typename T>
	shared_array<T>::shared_array
	(
		shared_array<T> const & SharedArray
	)
	{
		this->Counter = SharedArray.Counter;
		this->Pointer = SharedArray.Pointer;
		(*this->Counter)++;
	}

	template <typename T>
	shared_array<T>::shared_array
	(
		T * Pointer
	)
	{
		this->reset(Pointer);
	}

	template <typename T>
	shared_array<T>::~shared_array()
	{
		this->reset();
	}

	template <typename T>
	void shared_array<T>::reset()
	{
		if(this->Pointer)
		{
			(*this->Counter)--;
			if(*this->Counter <= 0)
			{
				delete this->Counter;
				this->Counter = 0;
				delete[] this->Pointer;
				this->Pointer = 0;
			}
		}
	}

	template <typename T>
	void shared_array<T>::reset(T * Pointer)
	{
		this->Counter = new int;
		this->Pointer = Pointer;
		*this->Counter = 1;
	}

	template <typename T>
	shared_array<T>& shared_array<T>::operator=
	(
		shared_array<T> const & SharedArray
	)
	{
		this->reset();

		this->Counter = SharedArray.Counter;
		this->Pointer = SharedArray.Pointer;
		(*this->Counter)++;

		return *this;
	}

	//template <typename T>
	//shared_array<T> & shared_array<T>::operator=(T * Pointer)
	//{
	//	if(this->Pointer)
	//	{
	//		(*this->Counter)--;
	//		if(*this->Counter <= 0)
	//		{
	//			delete this->Counter;
	//			delete[] this->Pointer;
	//		}
	//	}

	//	this->Counter = new int;
	//	this->Pointer = this->Pointer;
	//	(*this->Counter) = 1;

	//	return *this;
	//}

	template <typename T>
	bool shared_array<T>::operator==(shared_array<T> const & SharedArray) const
	{
		return this->Pointer == SharedArray.Pointer;
	}

	template <typename T>
	bool shared_array<T>::operator!=(shared_array<T> const & SharedArray) const
	{
		return this->Pointer != SharedArray.Pointer;
	}

	template <typename T>
	T & shared_array<T>::operator*()
	{
		return *this->Pointer;
	}

	template <typename T>
	T * shared_array<T>::operator->()
	{
		return this->Pointer;
	}

	template <typename T>
	T const & shared_array<T>::operator*() const
	{
		return * this->Pointer;
	}

	template <typename T>
	T const * const shared_array<T>::operator->() const
	{
		return this->Pointer;
	}

	template <typename T>
	T * shared_array<T>::get()
	{
		return this->Pointer;
	}

	template <typename T>
	T const * const shared_array<T>::get() const
	{
		return this->Pointer;
	}

}//namespace gli
