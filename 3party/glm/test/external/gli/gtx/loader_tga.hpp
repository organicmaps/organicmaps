///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Image Copyright (c) 2008 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2010-09-08
// Updated : 2010-09-27
// Licence : This source is under MIT License
// File    : gli/gtx/loader_tga.hpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef GLI_GTX_LOADER_TGA_INCLUDED
#define GLI_GTX_LOADER_TGA_INCLUDED

#include "../gli.hpp"
#include <string>
#include <fstream>

namespace gli{
namespace gtx{
namespace loader_tga
{
	texture2D loadTGA(
		std::string const & Filename);

	void saveTGA(
		texture2D const & Image, 
		std::string const & Filename);

}//namespace loader_tga
}//namespace gtx
}//namespace gli

namespace gli{using namespace gtx::loader_tga;}

#include "loader_tga.inl"

#endif//GLI_GTX_LOADER_TGA_INCLUDED
