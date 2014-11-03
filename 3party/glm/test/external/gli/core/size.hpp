///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Image Copyright (c) 2008 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2010-09-08
// Updated : 2010-09-08
// Licence : This source is under MIT License
// File    : gli/core/size.hpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef GLI_CORE_SIZE_INCLUDED
#define GLI_CORE_SIZE_INCLUDED

#include "texture2d.hpp"

namespace gli
{
	//template <size_type sizeType>
	image2D::size_type size(
		image2D const & Image, 
		image2D::size_type const & SizeType);

	//template <size_type sizeType>
	texture2D::size_type size(
		texture2D const & Texture, 
		texture2D::size_type const & SizeType);

}//namespace gli

#include "size.inl"

#endif//GLI_CORE_SIZE_INCLUDED
