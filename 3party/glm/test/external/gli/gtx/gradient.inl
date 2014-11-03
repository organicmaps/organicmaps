///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Image Copyright (c) 2008 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2008-12-19
// Updated : 2010-01-09
// Licence : This source is under MIT License
// File    : gli/gtx/gradient.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace gli{
namespace gtx{
namespace gradient
{
	inline texture2D radial
	(
		texture2D::dimensions_type const & Size, 
		texture2D::texcoord_type const & Center,
		float const & Radius,
		texture2D::texcoord_type const & Focal
	)
	{
		image2D Result(texture2D::dimensions_type(Size), gli::RGB8U);
		glm::u8vec3 * DstData = (glm::u8vec3 *)Result.data();

		for(std::size_t y = 0; y < Result.dimensions().y; ++y)
		for(std::size_t x = 0; x < Result.dimensions().x; ++x)
		{
			float Value = glm::radialGradient(
				Center * glm::vec2(Size), 
				Radius, 
				Focal * glm::vec2(Size),
				glm::vec2(x, y));

			std::size_t Index = x + y * Result.dimensions().x;

			*(DstData + Index) = glm::u8vec3(glm::u8(glm::clamp(Value * 255.f, 0.f, 255.f)));
		}

		gli::texture2D Image(1);
		Image[0] = Result;
		return Image;
	}

	inline texture2D linear
	(
		texture2D::dimensions_type const & Size, 
		texture2D::texcoord_type const & Point0, 
		texture2D::texcoord_type const & Point1
	)
	{
		image2D Result(texture2D::dimensions_type(Size), gli::RGB8U);
		glm::u8vec3 * DstData = (glm::u8vec3 *)Result.data();

		for(std::size_t y = 0; y < Result.dimensions().y; ++y)
		for(std::size_t x = 0; x < Result.dimensions().x; ++x)
		{
			float Value = glm::linearGradient(
				Point0 * glm::vec2(Size), 
				Point1 * glm::vec2(Size),
				texture2D::texcoord_type(x, y));

			std::size_t Index = x + y * Result.dimensions().x;

			*(DstData + Index) = glm::u8vec3(glm::u8(glm::clamp(Value * 255.f, 0.f, 255.f)));
		}

		gli::texture2D Image(1);
		Image[0] = Result;
		return Image;
	}

}//namespace gradient
}//namespace gtx
}//namespace gli
