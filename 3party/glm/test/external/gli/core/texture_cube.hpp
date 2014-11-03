///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Image Copyright (c) 2008 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2011-04-06
// Updated : 2011-04-06
// Licence : This source is under MIT License
// File    : gli/core/texture_cube.hpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef GLI_CORE_TEXTURE_CUBE_INCLUDED
#define GLI_CORE_TEXTURE_CUBE_INCLUDED

#include "texture2d.hpp"

namespace gli
{
	enum face
	{
		POSITIVE_X,
		NEGATIVE_X,
		POSITIVE_Y,
		NEGATIVE_Y,
		POSITIVE_Z,
		NEGATIVE_Z,
		FACE_MAX
	};

	class textureCube
	{
	public:
		typedef texture2D::dimensions_type dimensions_type;
		typedef texture2D::texcoord_type texcoord_type;
		typedef texture2D::size_type size_type;
		typedef texture2D::value_type value_type;
		typedef texture2D::format_type format_type;
		typedef texture2D::data_type data_type;
		typedef texture2D::level_type level_type;
		typedef face face_type;

	public:
		textureCube();

		explicit textureCube(level_type const & Levels);

		~textureCube();

		texture2D & operator[] (
			face_type const & Face);
		texture2D const & operator[] (
			face_type const & Face) const;

		bool empty() const;
		format_type format() const;
		level_type levels() const;
		void resize(level_type const & Levels);

	private:
		std::vector<texture2D> Faces;
	};

}//namespace gli

#include "texture_cube.inl"

#endif//GLI_CORE_TEXTURE_CUBE_INCLUDED
