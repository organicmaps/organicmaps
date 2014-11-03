///////////////////////////////////////////////////////////////////////////////////
/// OpenGL Mathematics (glm.g-truc.net)
///
/// Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
/// 
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
/// 
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
/// THE SOFTWARE.
///////////////////////////////////////////////////////////////////////////////////

#ifndef GLM_GTX_int_10_10_10_2
#define GLM_GTX_int_10_10_10_2

// Dependency:
#include "../glm.hpp"
#include "../gtx/raw_data.hpp"

#if(defined(GLM_MESSAGES))
#	pragma message("GLM: GLM_GTX_int_10_10_10_2 extension is deprecated, include GLM_GTC_packing (glm/gtc/packing.hpp) instead")
#endif

namespace glm
{
	//! Deprecated, use packUnorm3x10_1x2 instead.
	GLM_DEPRECATED GLM_FUNC_DECL dword uint10_10_10_2_cast(glm::vec4 const & v);

}//namespace glm

#include "int_10_10_10_2.inl"

#endif//GLM_GTX_int_10_10_10_2
