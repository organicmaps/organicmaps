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
	inline texture2DArray::texture2DArray()
	{}

	inline texture2DArray::texture2DArray
	(
		texture2DArray::layer_type const & Layers, 
		texture2DArray::level_type const & Levels
	)
	{
		this->Arrays.resize(Layers);
		for(texture2DArray::size_type i = 0; i < this->Arrays.size(); ++i)
			this->Arrays[i].resize(Levels);
	}

	inline texture2DArray::~texture2DArray()
	{}

	inline texture2D & texture2DArray::operator[] 
	(
		layer_type const & Layer
	)
	{
		return this->Arrays[Layer];
	}

	inline texture2D const & texture2DArray::operator[] 
	(
		layer_type const & Layer
	) const
	{
		return this->Arrays[Layer];
	}

	inline bool texture2DArray::empty() const
	{
		return this->Arrays.empty();
	}

	inline texture2DArray::format_type texture2DArray::format() const
	{
		return this->Arrays.empty() ? FORMAT_NULL : this->Arrays[0].format();
	}

	inline texture2DArray::layer_type texture2DArray::layers() const
	{
		return this->Arrays.size();
	}

	inline texture2DArray::level_type texture2DArray::levels() const
	{
		if(this->empty())
			return 0;
		return this->Arrays[0].levels();
	}

	inline void texture2DArray::resize
	(
		texture2DArray::layer_type const & Layers, 
		texture2DArray::level_type const & Levels
	)
	{
		this->Arrays.resize(Layers);
		for(texture2DArray::layer_type i = 0; i < this->Arrays.size(); ++i)
			this->Arrays[i].resize(Levels);
	}

}//namespace gli
