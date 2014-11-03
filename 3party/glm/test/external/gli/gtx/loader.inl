///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Image Copyright (c) 2008 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2010-09-08
// Updated : 2010-09-27
// Licence : This source is under MIT License
// File    : gli/gtx/loader.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace gli{
namespace gtx{
namespace loader
{
	inline texture2D load
	(
		std::string const & Filename
	)
	{
		if(Filename.find(".dds") != std::string::npos)
			return loadDDS10(Filename);
		else if(Filename.find(".tga") != std::string::npos)
			return loadTGA(Filename);
		else
		{
			assert(0); // File format not supported
			return texture2D();
		}
	}

	inline void save
	(
		texture2D const & Image, 
		std::string const & Filename
	)
	{
		if(Filename.find(".dds") != std::string::npos)
			saveDDS10(Image, Filename);
		else if(Filename.find(".tga") != std::string::npos)
			saveTGA(Image, Filename);
		else
			assert(0); // File format not supported
	}

}//namespace loader
}//namespace gtx
}//namespace gli
