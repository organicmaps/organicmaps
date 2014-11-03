///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Image Copyright (c) 2008 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2011-04-05
// Updated : 2011-04-05
// Licence : This source is under MIT License
// File    : gli/core/image2d.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace gli
{
	namespace detail
	{
		struct format_desc
		{
			image2D::size_type BlockSize;
			image2D::size_type BBP;
			image2D::size_type Component;
		};

		inline format_desc getFormatInfo(format const & Format)
		{
			format_desc Desc[FORMAT_MAX] =
			{
				{  0,  0,  0},	//FORMAT_NULL

				// Unsigned integer formats
				{  1,   8,  1},	//R8U,
				{  2,  16,  2},	//RG8U,
				{  3,  24,  3},	//RGB8U,
				{  4,  32,  4},	//RGBA8U,

				{  2,  16,  1},	//R16U,
				{  4,  32,  2},	//RG16U,
				{  6,  48,  3},	//RGB16U,
				{  8,  64,  4},	//RGBA16U,

				{  4,  32,  1},	//R32U,
				{  8,  64,  2},	//RG32U,
				{ 12,  96,  3},	//RGB32U,
				{ 16, 128,  4},	//RGBA32U,

				//// Signed integer formats
				{  4,  32,  1},	//R8I,
				{  8,  64,  2},	//RG8I,
				{ 12,  96,  3},	//RGB8I,
				{ 16, 128,  4},	//RGBA8I,

				{  2,  16,  1},	//R16I,
				{  4,  32,  2},	//RG16I,
				{  6,  48,  3},	//RGB16I,
				{  8,  64,  4},	//RGBA16I,

				{  4,  32,  1},	//R32I,
				{  8,  64,  2},	//RG32I,
				{ 12,  96,  3},	//RGB32I,
				{ 16, 128,  4},	//RGBA32I,

				//// Floating formats
				{  2,  16,  1},	//R16F,
				{  4,  32,  2},	//RG16F,
				{  6,  48,  3},	//RGB16F,
				{  8,  64,  4},	//RGBA16F,

				{  4,  32,  1},	//R32F,
				{  8,  64,  2},	//RG32F,
				{ 12,  96,  3},	//RGB32F,
				{ 16, 128,  4},	//RGBA32F,

				//// Packed formats
				{  4,  32,  3},	//RGBE8,
				{  4,  32,  3},	//RGB9E5,
				{  4,  32,  3},	//RG11B10F,
				{  2,  16,  3},	//R5G6B5,
				{  2,  16,  4},	//RGBA4,
				{  4,  32,  3},	//RGB10A2,

				//// Depth formats
				{  2,  16,  1},	//D16,
				{  4,  32,  1},	//D24X8,
				{  4,  32,  2},	//D24S8,
				{  4,  32,  1},	//D32F,
				{  8,  64,  2},	//D32FS8X24,

				//// Compressed formats
				{  8,   4,  4},	//DXT1,
				{ 16,   8,  4},	//DXT3,
				{ 16,   8,  4},	//DXT5,
				{  8,   4,  1},	//ATI1N_UNORM,
				{  8,   4,  1},	//ATI1N_SNORM,
				{ 16,   8,  2},	//ATI2N_UNORM,
				{ 16,   8,  2},	//ATI2N_SNORM,
				{ 16,   8,  3},	//BP_UF16,
				{ 16,   8,  3},	//BP_SF16,
				{ 16,   8,  4},	//BP,
			};

			return Desc[Format];
		}

		inline image2D::size_type sizeBlock
		(
			format const & Format
		)
		{
			return getFormatInfo(Format).BlockSize;
		}

		inline image2D::size_type sizeBitPerPixel
		(
			format const & Format
		)
		{
			return getFormatInfo(Format).BBP;
		}

		inline image2D::size_type sizeComponent
		(
			format const & Format
		)
		{
			return getFormatInfo(Format).Component;
		}

		inline image2D::size_type sizeLinear
		(
			image2D const & Image
		)
		{
			image2D::dimensions_type Dimension = Image.dimensions();
			Dimension = glm::max(Dimension, image2D::dimensions_type(1));

			image2D::size_type BlockSize = sizeBlock(Image.format());
			image2D::size_type BPP = sizeBitPerPixel(Image.format());
			image2D::size_type BlockCount = 0;
			if((BlockSize << 3) == BPP)
				BlockCount = Dimension.x * Dimension.y;
			else
				BlockCount = ((Dimension.x + 3) >> 2) * ((Dimension.y + 3) >> 2);			

			return BlockCount * BlockSize;
		}
	}//namespace detail

	inline image2D::image2D() :
		Data(0),
		Dimensions(0),
		Format(FORMAT_NULL)
	{}

	inline image2D::image2D
	(
		image2D const & Image
	) :
		Data(Image.Data),
		Dimensions(Image.Dimensions),
		Format(Image.Format)
	{}

	inline image2D::image2D   
	(
		dimensions_type const & Dimensions,
		format_type const & Format
	) :
		Data((glm::compMul(Dimensions) * detail::sizeBitPerPixel(Format)) >> 3),
		Dimensions(Dimensions),
		Format(Format)
	{}

	inline image2D::image2D
	(
		dimensions_type const & Dimensions,
		format_type const & Format,
		std::vector<value_type> const & Data
	) :
		Data(Data),
		Dimensions(Dimensions),
		Format(Format)
	{}

	inline image2D::~image2D()
	{}

	template <typename genType>
	inline void image2D::setPixel
	(
		dimensions_type const & TexelCoord,
		genType const & TexelData
	)
	{
		size_type Index = this->dimensions().x * sizeof(genType) * TexelCoord.y + sizeof(genType) * TexelCoord.x;
		memcpy(this->data() + Index, &TexelData[0], sizeof(genType));
	}

	inline image2D::size_type image2D::value_size() const
	{
		return detail::sizeBitPerPixel(this->format());
	}

	inline image2D::size_type image2D::capacity() const
	{
		return detail::sizeLinear(*this);
	}

	inline image2D::dimensions_type image2D::dimensions() const
	{
		return this->Dimensions;
	}

	inline image2D::size_type image2D::components() const
	{
		return detail::sizeComponent(this->format());
	}

	inline image2D::format_type image2D::format() const
	{
		return this->Format;
	}

	inline image2D::value_type * image2D::data()
	{
		return &this->Data[0];
	}

	inline image2D::value_type const * const image2D::data() const
	{
		return &this->Data[0];
	}
}//namespace gli
