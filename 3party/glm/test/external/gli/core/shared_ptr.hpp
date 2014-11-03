///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Image Copyright (c) 2008 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2008-12-19
// Updated : 2005-06-13
// Licence : This source is under MIT License
// File    : gli/fetch.hpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef GLI_SHARED_PTR_INCLUDED
#define GLI_SHARED_PTR_INCLUDED

namespace gli
{
    template <typename T>
    class shared_ptr
    {
    public:
        shared_ptr();
        shared_ptr(shared_ptr const & SmartPtr);
        shared_ptr(T* pPointer);
        ~shared_ptr();

        T& operator*();
        T* operator->();
        const T& operator*() const;
        const T* operator->() const;
        shared_ptr& operator=(shared_ptr const & SmartPtr);
        shared_ptr& operator=(T* pPointer);
	    bool operator==(shared_ptr const & SmartPtr) const;
	    bool operator!=(shared_ptr const & SmartPtr) const;

    private:
        int* m_pReference;
        T* m_pPointer;
    };
}//namespace gli

#include "shared_ptr.inl"

#endif //GLI_SHARED_PTR_INCLUDED
