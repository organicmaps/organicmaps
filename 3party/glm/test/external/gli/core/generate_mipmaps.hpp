///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Image Copyright (c) 2008 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2010-09-27
// Updated : 2010-09-27
// Licence : This source is under MIT License
// File    : gli/core/generate_mipmaps.hpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef GLI_GENERATE_MIPMAPS_INCLUDED
#define GLI_GENERATE_MIPMAPS_INCLUDED

#include "texture2d.hpp"

namespace gli
{
	texture2D generateMipmaps(
		texture2D const & Texture, 
		texture2D::level_type const & BaseLevel);

}//namespace gli

#include "generate_mipmaps.inl"

#endif//GLI_GENERATE_MIPMAPS_INCLUDED
