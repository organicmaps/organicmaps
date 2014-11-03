///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Image Copyright (c) 2008 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2011-04-06
// Updated : 2011-04-06
// Licence : This source is under MIT License
// File    : gli/core/texture2d_array.hpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef GLI_CORE_TEXTURE2D_ARRAY_INCLUDED
#define GLI_CORE_TEXTURE2D_ARRAY_INCLUDED

#include "texture2d.hpp"

namespace gli
{
	class texture2DArray
	{
	public:
		typedef texture2D::dimensions_type dimensions_type;
		typedef texture2D::texcoord_type texcoord_type;
		typedef texture2D::size_type size_type;
		typedef texture2D::value_type value_type;
		typedef texture2D::format_type format_type;
		typedef texture2D::data_type data_type;
		typedef texture2D::level_type level_type;
		typedef std::vector<texture2D>::size_type layer_type;

	public:
		texture2DArray();

		explicit texture2DArray(
			layer_type const & Layers, 
			level_type const & Levels);

		~texture2DArray();

		texture2D & operator[] (
			layer_type const & Layer);
		texture2D const & operator[] (
			layer_type const & Layer) const;

		bool empty() const;
		format_type format() const;
		layer_type layers() const;
		level_type levels() const;
		void resize(
			layer_type const & Layers, 
			level_type const & Levels);

	private:
		std::vector<texture2D> Arrays;
	};

}//namespace gli

#include "texture2d_array.inl"

#endif//GLI_CORE_TEXTURE2D_ARRAY_INCLUDED
