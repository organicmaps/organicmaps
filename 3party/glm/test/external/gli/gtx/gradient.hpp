///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Image Copyright (c) 2008 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2008-12-19
// Updated : 2010-01-09
// Licence : This source is under MIT License
// File    : gli/gtx/gradient.hpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef GLI_GTX_GRADIENT_INCLUDED
#define GLI_GTX_GRADIENT_INCLUDED

#include "../gli.hpp"

namespace gli{
namespace gtx{
namespace gradient
{
	texture2D radial(
		texture2D::dimensions_type const & Size, 
		texture2D::texcoord_type const & Center,
		float const & Radius,
		texture2D::texcoord_type const & Focal);

	texture2D linear(
		texture2D::dimensions_type const & Size, 
		texture2D::texcoord_type const & Point0, 
		texture2D::texcoord_type const & Point1);

}//namespace gradient
}//namespace gtx
}//namespace gli

namespace gli{using namespace gtx::gradient;}

#include "gradient.inl"

#endif//GLI_GTX_GRADIENT_INCLUDED
