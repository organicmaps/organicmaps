///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Image Copyright (c) 2008 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2011-04-06
// Updated : 2011-04-06
// Licence : This source is under MIT License
// File    : gli/core/texture_cube_array.hpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef GLI_CORE_TEXTURE_CUBE_ARRAY_INCLUDED
#define GLI_CORE_TEXTURE_CUBE_ARRAY_INCLUDED

#include "texture_cube.hpp"

namespace gli
{
	class textureCubeArray
	{
	public:
		typedef textureCube::dimensions_type dimensions_type;
		typedef textureCube::texcoord_type texcoord_type;
		typedef textureCube::size_type size_type;
		typedef textureCube::value_type value_type;
		typedef textureCube::format_type format_type;
		typedef std::vector<textureCube> data_type;
		typedef textureCube::level_type level_type;
		typedef data_type::size_type layer_type;

	public:
		textureCubeArray();

		explicit textureCubeArray(
			layer_type const & Layers, 
			level_type const & Levels);

		~textureCubeArray();

		textureCube & operator[] (
			layer_type const & Layer);
		textureCube const & operator[] (
			layer_type const & Layer) const;

		bool empty() const;
		format_type format() const;
		layer_type layers() const;
		level_type levels() const;
		void resize(
			layer_type const & Layers, 
			level_type const & Levels);

	private:
		data_type Arrays;
	};

}//namespace gli

#include "texture_cube_array.inl"

#endif//GLI_CORE_TEXTURE_CUBE_ARRAY_INCLUDED
