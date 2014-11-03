///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Image Copyright (c) 2008 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2008-12-19
// Updated : 2010-09-08
// Licence : This source is under MIT License
// File    : gli/core/operation.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <cstring>

namespace gli
{
	namespace detail
	{
		inline image2D duplicate(image2D const & Mipmap2D)
		{
			image2D Result(Mipmap2D.dimensions(), Mipmap2D.format());
			memcpy(Result.data(), Mipmap2D.data(), Mipmap2D.capacity());
			return Result;	
		}

		inline image2D flip(image2D const & Mipmap2D)
		{
			image2D Result(Mipmap2D.dimensions(), Mipmap2D.format());
			
			std::size_t ValueSize = Result.value_size();
			glm::byte * DstPtr = Result.data();
			glm::byte const * const SrcPtr = Mipmap2D.data();

			for(std::size_t j = 0; j < Result.dimensions().y; ++j)
			for(std::size_t i = 0; i < Result.dimensions().x; ++i)
			{
				std::size_t DstIndex = (i + j * Result.dimensions().y) * ValueSize;
				std::size_t SrcIndex = (i + (Result.dimensions().y - j) * Result.dimensions().x) * ValueSize;
				memcpy(DstPtr + DstIndex, SrcPtr + SrcIndex, ValueSize);
			}

			return Result;
		}

		inline image2D mirror(image2D const & Mipmap2D)
		{
			image2D Result(Mipmap2D.dimensions(), Mipmap2D.format());

			std::size_t ValueSize = Mipmap2D.value_size();
			glm::byte * DstPtr = Result.data();
			glm::byte const * const SrcPtr = Mipmap2D.data();

			for(std::size_t j = 0; j < Result.dimensions().y; ++j)
			for(std::size_t i = 0; i < Result.dimensions().x; ++i)
			{
				std::size_t DstIndex = (i + j * Result.dimensions().x) * ValueSize;
				std::size_t SrcIndex = ((Result.dimensions().x - i) + j * Result.dimensions().x) * ValueSize;
				memcpy(DstPtr + DstIndex, SrcPtr + SrcIndex, ValueSize);
			}

			return Result;
		}

		inline image2D swizzle
		(
			image2D const & Mipmap, 
			glm::uvec4 const & Channel
		)
		{
			image2D Result = detail::duplicate(Mipmap);

			glm::byte * DataDst = Result.data();
			glm::byte const * const DataSrc = Mipmap.data();

			gli::texture2D::size_type CompSize = Mipmap.value_size() / Mipmap.components();
			gli::texture2D::size_type TexelCount = Mipmap.capacity() / Mipmap.value_size();

			for(gli::texture2D::size_type t = 0; t < TexelCount; ++t)
			for(gli::texture2D::size_type c = 0; c < Mipmap.components(); ++c)
			{
				gli::texture2D::size_type IndexSrc = t * Mipmap.components() + Channel[static_cast<int>(c)];
				gli::texture2D::size_type IndexDst = t * Mipmap.components() + c;

				memcpy(DataDst + IndexDst, DataSrc + IndexSrc, CompSize);
			}

			return Result;
		}

		inline image2D crop
		(
			image2D const & Image, 
			image2D::dimensions_type const & Position, 
			image2D::dimensions_type const & Size
		)
		{
			assert((Position.x + Size.x) <= Image.dimensions().x && (Position.y + Size.y) <= Image.dimensions().y);

			image2D Result(Size, Image.format());

			glm::byte* DstData = Result.data();
			glm::byte const * const SrcData = Image.data();

			for(std::size_t j = 0; j < Size.y; ++j)
			{
				std::size_t DstIndex = 0                                + (0          + j) * Size.x         * Image.value_size();
				std::size_t SrcIndex = Position.x * Image.value_size() + (Position.y + j) * Image.dimensions().x * Image.value_size();
				memcpy(DstData + DstIndex, SrcData + SrcIndex, Image.value_size() * Size.x);	
			}

			return Result;
		}

		inline image2D copy
		(
			image2D const & SrcMipmap, 
			image2D::dimensions_type const & SrcPosition,
			image2D::dimensions_type const & SrcSize,
			image2D & DstMipmap, 
			image2D::dimensions_type const & DstPosition
		)
		{
			assert((SrcPosition.x + SrcSize.x) <= SrcMipmap.dimensions().x && (SrcPosition.y + SrcSize.y) <= SrcMipmap.dimensions().y);
			assert(SrcMipmap.format() == DstMipmap.format());

			glm::byte * DstData = DstMipmap.data();
			glm::byte const * const SrcData = SrcMipmap.data();

			std::size_t SizeX = glm::min(std::size_t(SrcSize.x + SrcPosition.x), std::size_t(DstMipmap.dimensions().x  + DstPosition.x));
			std::size_t SizeY = glm::min(std::size_t(SrcSize.y + SrcPosition.y), std::size_t(DstMipmap.dimensions().y + DstPosition.y));

			for(std::size_t j = 0; j < SizeY; ++j)
			{
				std::size_t DstIndex = DstPosition.x * DstMipmap.value_size() + (DstPosition.y + j) * DstMipmap.dimensions().x * DstMipmap.value_size();
				std::size_t SrcIndex = SrcPosition.x * SrcMipmap.value_size() + (SrcPosition.y + j) * SrcMipmap.dimensions().x * SrcMipmap.value_size();
				memcpy(DstData + DstIndex, SrcData + SrcIndex, SrcMipmap.value_size() * SizeX);	
			}

			return DstMipmap;
		}

	}//namespace detail

	inline texture2D duplicate(texture2D const & Texture2D)
	{
		texture2D Result(Texture2D.levels());
		for(texture2D::level_type Level = 0; Level < Texture2D.levels(); ++Level)
			Result[Level] = detail::duplicate(Texture2D[Level]);
		return Result;
	}

	inline texture2D flip(texture2D const & Texture2D)
	{
		texture2D Result(Texture2D.levels());
		for(texture2D::level_type Level = 0; Level < Texture2D.levels(); ++Level)
			Result[Level] = detail::flip(Texture2D[Level]);
		return Result;
	}

	inline texture2D mirror(texture2D const & Texture2D)
	{
		texture2D Result(Texture2D.levels());
		for(texture2D::level_type Level = 0; Level < Texture2D.levels(); ++Level)
			Result[Level] = detail::mirror(Texture2D[Level]);
		return Result;
	}

	inline texture2D crop
	(
		texture2D const & Texture2D,
		texture2D::dimensions_type const & Position,
		texture2D::dimensions_type const & Size
	)
	{
		texture2D Result(Texture2D.levels());
		for(texture2D::level_type Level = 0; Level < Texture2D.levels(); ++Level)
			Result[Level] = detail::crop(
				Texture2D[Level], 
				Position >> texture2D::dimensions_type(Level), 
				Size >> texture2D::dimensions_type(Level));
		return Result;
	}

	inline texture2D swizzle
	(
		texture2D const & Texture2D,
		glm::uvec4 const & Channel
	)
	{
		texture2D Result(Texture2D.levels());
		for(texture2D::level_type Level = 0; Level < Texture2D.levels(); ++Level)
			Result[Level] = detail::swizzle(Texture2D[Level], Channel);
		return Result;
	}

	inline texture2D copy
	(
		texture2D const & SrcImage, 
		texture2D::level_type const & SrcLevel,
		texture2D::dimensions_type const & SrcPosition,
		texture2D::dimensions_type const & SrcDimensions,
		texture2D & DstMipmap, 
		texture2D::level_type const & DstLevel,
		texture2D::dimensions_type const & DstDimensions
	)
	{
		detail::copy(
			SrcImage[SrcLevel], 
			SrcPosition, 
			SrcDimensions,
			DstMipmap[DstLevel],
			DstDimensions);
		return DstMipmap;
	}

	//inline image operator+(image const & MipmapA, image const & MipmapB)
	//{
	//	
	//}

	//inline image operator-(image const & MipmapA, image const & MipmapB)
	//{
	//	
	//}

	//inline image operator*(image const & MipmapA, image const & MipmapB)
	//{
	//	
	//}

	//inline image operator/(image const & MipmapA, image const & MipmapB)
	//{
	//	
	//}

}//namespace gli
