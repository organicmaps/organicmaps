///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Image Copyright (c) 2008 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2011-04-06
// Updated : 2011-04-06
// Licence : This source is under MIT License
// File    : gli/core/texture_cube.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace gli
{
	inline textureCube::textureCube()
	{}

	inline textureCube::textureCube
	(
		level_type const & Levels
	)
	{
		this->Faces.resize(FACE_MAX);
		for(textureCube::size_type i = 0; i < FACE_MAX; ++i)
			this->Faces[i].resize(Levels);
	}

	inline textureCube::~textureCube()
	{}

	inline texture2D & textureCube::operator[] 
	(
		face_type const & Face
	)
	{
		return this->Faces[Face];
	}

	inline texture2D const & textureCube::operator[] 
	(
		face_type const & Face
	) const
	{
		return this->Faces[Face];
	}

	inline bool textureCube::empty() const
	{
		return this->Faces.size() == 0;
	}

	inline textureCube::format_type textureCube::format() const
	{
		return this->Faces.empty() ? FORMAT_NULL : this->Faces[0].format();
	}

	inline textureCube::level_type textureCube::levels() const
	{
		if(this->empty())
			return 0;
		return this->Faces[POSITIVE_X].levels();
	}

	inline void textureCube::resize
	(
		level_type const & Levels
	)
	{
		for(textureCube::size_type i = 0; i < FACE_MAX; ++i)
			this->Faces[i].resize(Levels);
	}

}//namespace gli
