///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Image Copyright (c) 2008 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2011-04-05
// Updated : 2011-04-05
// Licence : This source is under MIT License
// File    : gli/core/image2d.hpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef GLI_CORE_IMAGE2D_INCLUDED
#define GLI_CORE_IMAGE2D_INCLUDED

// STD
#include <vector>
#include <cassert>
#include <cmath>
#include <cstring>

// GLM
#include <glm/glm.hpp>
#include <glm/gtx/number_precision.hpp>
#include <glm/gtx/raw_data.hpp>
#include <glm/gtx/gradient_paint.hpp>
#include <glm/gtx/component_wise.hpp>

namespace gli
{
	enum format
	{
		FORMAT_NULL,

		// Unsigned integer formats
		R8U,
		RG8U,
		RGB8U,
		RGBA8U,

		R16U,
		RG16U,
		RGB16U,
		RGBA16U,

		R32U,
		RG32U,
		RGB32U,
		RGBA32U,

		// Signed integer formats
		R8I,
		RG8I,
		RGB8I,
		RGBA8I,

		R16I,
		RG16I,
		RGB16I,
		RGBA16I,

		R32I,
		RG32I,
		RGB32I,
		RGBA32I,

		// Floating formats
		R16F,
		RG16F,
		RGB16F,
		RGBA16F,

		R32F,
		RG32F,
		RGB32F,
		RGBA32F,

		// Packed formats
		RGBE8,
		RGB9E5,
		RG11B10F,
		R5G6B5,
		RGBA4,
		RGB10A2,

		// Depth formats
		D16,
		D24X8,
		D24S8,
		D32F,
		D32FS8X24,

		// Compressed formats
		DXT1,
		DXT3,
		DXT5,
		ATI1N_UNORM,
		ATI1N_SNORM,
		ATI2N_UNORM,
		ATI2N_SNORM,
		BP_UF16,
		BP_SF16,
		BP,

		FORMAT_MAX
	};

	enum size_type
	{
		LINEAR_SIZE,
		BLOCK_SIZE,
		BIT_PER_PIXEL, 
		COMPONENT
	};

	class image2D
	{
	public:
		typedef glm::uvec2 dimensions_type;
		typedef glm::vec2 texcoord_type;
		typedef glm::uint32 size_type;
		typedef glm::byte value_type;
		typedef gli::format format_type;
		typedef std::vector<value_type> data_type;

	public:
		image2D();
		image2D(
			image2D const & Image);

		explicit image2D(
			dimensions_type const & Dimensions,
			format_type const & Format);

		template <typename genType>
		explicit image2D(
			dimensions_type const & Dimensions,
			format_type const & Format, 
			genType const & Value);

		explicit image2D(
			dimensions_type const & Dimensions,
			format_type const & Format, 
			std::vector<value_type> const & Data);

		~image2D();

		template <typename genType>
		void setPixel(
			dimensions_type const & TexelCoord,
			genType const & TexelData);

		size_type value_size() const;
		size_type capacity() const;
		dimensions_type dimensions() const;
		size_type components() const;
		format_type format() const;
			
		value_type * data();
		value_type const * const data() const;

	private:
		data_type Data;
		dimensions_type Dimensions;
		format_type Format;
	};

}//namespace gli

#include "image2d.inl"

#endif//GLI_CORE_IMAGE2D_INCLUDED
