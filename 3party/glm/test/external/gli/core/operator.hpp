///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Image Copyright (c) 2008 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2010-01-19
// Updated : 2010-01-19
// Licence : This source is under MIT License
// File    : gli/core/operator.hpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef GLI_OPERATOR_INCLUDED
#define GLI_OPERATOR_INCLUDED

#include "texture2d.hpp"

namespace gli{
namespace detail
{

}//namespace detail

	texture2D operator+(texture2D const & TextureA, texture2D const & TextureB);
	texture2D operator-(texture2D const & TextureA, texture2D const & TextureB);

}//namespace gli

#include "operator.inl"

#endif//GLI_OPERATOR_INCLUDED
