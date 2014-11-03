///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Image Copyright (c) 2008 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2008-12-19
// Updated : 2010-01-09
// Licence : This source is under MIT License
// File    : gli/operation.hpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef GLI_OPERATION_INCLUDED
#define GLI_OPERATION_INCLUDED

#include "texture2d.hpp"

namespace gli
{
	texture2D duplicate(texture2D const & Texture);
	texture2D flip(texture2D const & Texture);
	texture2D mirror(texture2D const & Texture);
	texture2D swizzle(
		texture2D const & Texture, 
		glm::uvec4 const & Channel);
	texture2D crop(
		texture2D const & Texture, 
		texture2D::dimensions_type const & Position,
		texture2D::dimensions_type const & Size);

	image2D crop(
		image2D const & Image, 
		texture2D::dimensions_type const & Position,
		texture2D::dimensions_type const & Size);

	image2D copy(
		image2D const & SrcImage, 
		image2D::dimensions_type const & SrcPosition,
		image2D::dimensions_type const & SrcSize,
		image2D & DstImage, 
		image2D::dimensions_type const & DstPosition);

	//image operator+(image const & MipmapA, image const & MipmapB);
	//image operator-(image const & MipmapA, image const & MipmapB);
	//image operator*(image const & MipmapA, image const & MipmapB);
	//image operator/(image const & MipmapA, image const & MipmapB);

	//namespace wip
	//{
	//	template <typename GENTYPE, template <typename> class SURFACE>
	//	GENTYPE fetch(SURFACE<GENTYPE> const & Image)
	//	{
	//		return GENTYPE();
	//	}

	//	template
	//	<
	//		typename GENTYPE, 
	//		template 
	//		<
	//			typename
	//		>
	//		class SURFACE,
	//		template 
	//		<
	//			typename, 
	//			template 
	//			<
	//				typename
	//			>
	//			class
	//		> 
	//		class IMAGE
	//	>
	//	GENTYPE fetch(IMAGE<GENTYPE, SURFACE> const & Image)
	//	{
	//		return GENTYPE();
	//	}
	//}//namespace wip

}//namespace gli

#include "operation.inl"

#endif//GLI_OPERATION_INCLUDED
