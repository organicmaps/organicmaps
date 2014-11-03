///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Image Copyright (c) 2008 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2008-12-19
// Updated : 2005-06-13
// Licence : This source is under MIT License
// File    : gli/shared_array.hpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef GLI_SHARED_ARRAY_INCLUDED
#define GLI_SHARED_ARRAY_INCLUDED

namespace gli
{
    template <typename T>
    class shared_array
    {
    public:

        shared_array();
        shared_array(shared_array const & SharedArray);
		shared_array(T * Pointer);
        virtual ~shared_array();

		void reset();
		void reset(T * Pointer);

        T & operator*();
        T * operator->();
        T const & operator*() const;
        T const * const operator->() const;

		T * get();
		T const * const get() const;

        shared_array & operator=(shared_array const & SharedArray);
	    bool operator==(shared_array const & SharedArray) const;
	    bool operator!=(shared_array const & SharedArray) const;

    private:
        int * Counter;
        T * Pointer;
    };
}//namespace gli

#include "shared_array.inl"

#endif //GLI_SHARED_ARRAY_INCLUDED
