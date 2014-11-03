///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Image Copyright (c) 2008 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2010-09-27
// Updated : 2010-10-01
// Licence : This source is under MIT License
// File    : gli/gtx/gl_texture2d.hpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef GLI_GTX_GL_TEXTURE2D_INCLUDED
#define GLI_GTX_GL_TEXTURE2D_INCLUDED

#include "../gli.hpp"
#include "../gtx/loader.hpp"

#ifndef GL_VERSION_1_1
#error "ERROR: OpenGL must be included before GLI_GTX_gl_texture2d"
#endif//GL_VERSION_1_1

namespace gli{
namespace gtx{
namespace gl_texture2d
{
	GLuint createTexture2D(std::string const & Filename);
}//namespace gl_texture2d
}//namespace gtx
}//namespace gli

namespace gli{using namespace gtx::gl_texture2d;}

#include "gl_texture2d.inl"

#endif//GLI_GTX_GL_TEXTURE2D_INCLUDED
