///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Image Copyright (c) 2008 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2010-09-08
// Updated : 2010-09-27
// Licence : This source is under MIT License
// File    : gli/gtx/loader_dds9.hpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef GLI_GTX_LOADER_DDS9_INCLUDED
#define GLI_GTX_LOADER_DDS9_INCLUDED

#include "../gli.hpp"
#include <fstream>

namespace gli{
namespace gtx{
namespace loader_dds9
{
	texture2D loadDDS9(
		std::string const & Filename);

	void saveDDS9(
		texture2D const & Texture, 
		std::string const & Filename);

	void saveTextureCubeDDS9(
		textureCube const & Texture, 
		std::string const & Filename);

}//namespace loader_dds9
}//namespace gtx
}//namespace gli

namespace gli{using namespace gtx::loader_dds9;}

#include "loader_dds9.inl"

#endif//GLI_GTX_LOADER_DDS9_INCLUDED
