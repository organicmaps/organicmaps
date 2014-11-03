///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Image Copyright (c) 2008 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2011-04-06
// Updated : 2011-04-06
// Licence : This source is under MIT License
// File    : gli/core/texture_cube_array.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace gli
{
	inline textureCubeArray::textureCubeArray()
	{}

	inline textureCubeArray::textureCubeArray
	(
		layer_type const & Layers, 
		level_type const & Levels
	)
	{
		this->Arrays.resize(Layers);
		for(textureCubeArray::size_type i = 0; i < this->Arrays.size(); ++i)
			this->Arrays[i].resize(Levels);
	}

	inline textureCubeArray::~textureCubeArray()
	{}

	inline textureCube & textureCubeArray::operator[] 
	(
		layer_type const & Layer
	)
	{
		return this->Arrays[Layer];
	}

	inline textureCube const & textureCubeArray::operator[] 
	(
		layer_type const & Layer
	) const
	{
		return this->Arrays[Layer];
	}

	inline bool textureCubeArray::empty() const
	{
		return this->Arrays.empty();
	}

	inline textureCubeArray::format_type textureCubeArray::format() const
	{
		return this->Arrays.empty() ? FORMAT_NULL : this->Arrays[0].format();
	}

	inline textureCubeArray::level_type textureCubeArray::levels() const
	{
		if(this->empty())
			return 0;
		return this->Arrays[0].levels();
	}

	inline void textureCubeArray::resize
	(
		layer_type const & Layers, 
		level_type const & Levels
	)
	{
		for(textureCubeArray::size_type i = 0; i < this->Arrays.size(); ++i)
			this->Arrays[i].resize(Levels);
	}

}//namespace gli
