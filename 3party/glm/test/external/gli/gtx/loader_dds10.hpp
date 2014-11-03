///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Image Copyright (c) 2008 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2010-09-26
// Updated : 2010-09-27
// Licence : This source is under MIT License
// File    : gli/gtx/loader_dds10.hpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef GLI_GTX_LOADER_DDS10_INCLUDED
#define GLI_GTX_LOADER_DDS10_INCLUDED

#include "../gli.hpp"
#include <fstream>

namespace gli{
namespace gtx{
namespace loader_dds10
{
	texture2D loadDDS10(
		std::string const & Filename);

	void saveDDS10(
		texture2D const & Image, 
		std::string const & Filename);

}//namespace loader_dds10
}//namespace gtx
}//namespace gli

namespace gli{using namespace gtx::loader_dds10;}

#include "loader_dds10.inl"

#endif//GLI_GTX_LOADER_DDS10_INCLUDED
