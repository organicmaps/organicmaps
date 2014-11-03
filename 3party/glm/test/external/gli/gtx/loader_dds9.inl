///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Image Copyright (c) 2008 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2010-09-08
// Updated : 2010-09-27
// Licence : This source is under MIT License
// File    : gli/gtx/loader_dds9.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace gli{
namespace gtx{
namespace loader_dds9{
namespace detail
{
	// DDS Documentation
	/*
		http://msdn.microsoft.com/en-us/library/bb943991(VS.85).aspx#File_Layout1
		http://msdn.microsoft.com/en-us/library/bb943992.aspx
	*/

	#define GLI_MAKEFOURCC(ch0, ch1, ch2, ch3) \
	  (glm::uint32)( \
		(((glm::uint32)(glm::uint8)(ch3) << 24) & 0xFF000000) | \
		(((glm::uint32)(glm::uint8)(ch2) << 16) & 0x00FF0000) | \
		(((glm::uint32)(glm::uint8)(ch1) <<  8) & 0x0000FF00) | \
		 ((glm::uint32)(glm::uint8)(ch0)        & 0x000000FF) )

	//enum dds_format
	//{
	//	GLI_D3DFMT_R8G8B8               = 20,
	//	GLI_D3DFMT_A8R8G8B8             = 21,
	//	GLI_D3DFMT_X8R8G8B8             = 22,
	//	GLI_D3DFMT_A8                   = 28,
	//	GLI_D3DFMT_A2B10G10R10          = 31,
	//	GLI_D3DFMT_A8B8G8R8             = 32,
	//	GLI_D3DFMT_X8B8G8R8             = 33,
	//	GLI_D3DFMT_G16R16               = 34,
	//	GLI_D3DFMT_A2R10G10B10          = 35,
	//	GLI_D3DFMT_A16B16G16R16         = 36,

	//	GLI_D3DFMT_L8                   = 50,
	//	GLI_D3DFMT_A8L8                 = 51,

	//	GLI_D3DFMT_DXT1                 = GLI_MAKEFOURCC('D', 'X', 'T', '1'),
	//	GLI_D3DFMT_DXT2                 = GLI_MAKEFOURCC('D', 'X', 'T', '2'),
	//	GLI_D3DFMT_DXT3                 = GLI_MAKEFOURCC('D', 'X', 'T', '3'),
	//	GLI_D3DFMT_DXT4                 = GLI_MAKEFOURCC('D', 'X', 'T', '4'),
	//	GLI_D3DFMT_DXT5                 = GLI_MAKEFOURCC('D', 'X', 'T', '5'),
	//	GLI_D3DFMT_DX10                 = GLI_MAKEFOURCC('D', 'X', '1', '0'),

	//	GLI_D3DFMT_D32                  = 71,
	//	GLI_D3DFMT_D24S8                = 75,
	//	GLI_D3DFMT_D24X8                = 77,
	//	GLI_D3DFMT_D16                  = 80,
	//	GLI_D3DFMT_L16                  = 81,
	//	GLI_D3DFMT_D32F_LOCKABLE        = 82,
	//	GLI_D3DFMT_D24FS8               = 83,

	//	GLI_D3DFMT_R16F                 = 111,
	//	GLI_D3DFMT_G16R16F              = 112,
	//	GLI_D3DFMT_A16B16G16R16F        = 113,

	//	GLI_D3DFMT_R32F                 = 114,
	//	GLI_D3DFMT_G32R32F              = 115,
	//	GLI_D3DFMT_A32B32G32R32F        = 116
	//};

	enum ddsCubemapflag
	{
		GLI_DDSCAPS2_CUBEMAP				= 0x00000200,
		GLI_DDSCAPS2_CUBEMAP_POSITIVEX		= 0x00000400,
		GLI_DDSCAPS2_CUBEMAP_NEGATIVEX		= 0x00000800,
		GLI_DDSCAPS2_CUBEMAP_POSITIVEY		= 0x00001000,
		GLI_DDSCAPS2_CUBEMAP_NEGATIVEY		= 0x00002000,
		GLI_DDSCAPS2_CUBEMAP_POSITIVEZ		= 0x00004000,
		GLI_DDSCAPS2_CUBEMAP_NEGATIVEZ		= 0x00008000,
		GLI_DDSCAPS2_VOLUME					= 0x00200000
	};

	enum ddsSurfaceflag
	{
		GLI_DDSCAPS_COMPLEX				= 0x00000008,
		GLI_DDSCAPS_MIPMAP				= 0x00400000,
		GLI_DDSCAPS_TEXTURE				= 0x00001000
	};

	struct ddsPixelFormat
	{
		glm::uint32 size; // 32
		glm::uint32 flags;
		glm::uint32 fourCC;
		glm::uint32 bpp;
		glm::uint32 redMask;
		glm::uint32 greenMask;
		glm::uint32 blueMask;
		glm::uint32 alphaMask;
	};

	struct ddsHeader
	{
		glm::uint32 size;
		glm::uint32 flags;
		glm::uint32 height;
		glm::uint32 width;
		glm::uint32 pitch;
		glm::uint32 depth;
		glm::uint32 mipMapLevels;
		glm::uint32 reserved1[11];
		ddsPixelFormat format;
		glm::uint32 surfaceFlags;
		glm::uint32 cubemapFlags;
		glm::uint32 reserved2[3];
	};

	glm::uint32 const GLI_D3DFMT_R8G8B8  = 20;
	glm::uint32 const GLI_D3DFMT_A8R8G8B8 = 21;
	glm::uint32 const GLI_D3DFMT_X8R8G8B8 = 22;
	glm::uint32 const GLI_D3DFMT_R5G6B5 = 23;
	glm::uint32 const GLI_D3DFMT_X1R5G5B5 = 24;
	glm::uint32 const GLI_D3DFMT_A1R5G5B5 = 25;
	glm::uint32 const GLI_D3DFMT_A4R4G4B4 = 26;
	glm::uint32 const GLI_D3DFMT_X4R4G4B4 = 30;
	glm::uint32 const GLI_D3DFMT_A2B10G10R10 = 31;
	glm::uint32 const GLI_D3DFMT_A8B8G8R8 = 32;
	glm::uint32 const GLI_D3DFMT_X8B8G8R8 = 33;
	glm::uint32 const GLI_D3DFMT_G16R16 = 34;
	glm::uint32 const GLI_D3DFMT_A2R10G10B10 = 35;
	glm::uint32 const GLI_D3DFMT_A16B16G16R16 = 36;


	glm::uint32 const GLI_FOURCC_DXT1 = GLI_MAKEFOURCC('D', 'X', 'T', '1');
	glm::uint32 const GLI_FOURCC_DXT2 = GLI_MAKEFOURCC('D', 'X', 'T', '2');
	glm::uint32 const GLI_FOURCC_DXT3 = GLI_MAKEFOURCC('D', 'X', 'T', '3');
	glm::uint32 const GLI_FOURCC_DXT4 = GLI_MAKEFOURCC('D', 'X', 'T', '4');
	glm::uint32 const GLI_FOURCC_DXT5 = GLI_MAKEFOURCC('D', 'X', 'T', '5');
	glm::uint32 const GLI_FOURCC_ATI1 = GLI_MAKEFOURCC('A', 'T', 'I', '1');			// ATI1
	glm::uint32 const GLI_FOURCC_ATI2 = GLI_MAKEFOURCC('A', 'T', 'I', '2');			// ATI2 (AKA 3Dc)
	glm::uint32 const GLI_FOURCC_DX10 = GLI_MAKEFOURCC('D', 'X', '1', '0');
	glm::uint32 const GLI_FOURCC_BC4U = GLI_MAKEFOURCC('B', 'C', '4', 'U');
	glm::uint32 const GLI_FOURCC_BC4S = GLI_MAKEFOURCC('B', 'C', '4', 'S');
	glm::uint32 const GLI_FOURCC_BC5U = GLI_MAKEFOURCC('B', 'C', '5', 'U');
	glm::uint32 const GLI_FOURCC_BC5S = GLI_MAKEFOURCC('B', 'C', '5', 'S');
	glm::uint32 const GLI_FOURCC_BC6H = GLI_MAKEFOURCC('B', 'C', '6', 'H');
	glm::uint32 const GLI_FOURCC_BC7  = GLI_MAKEFOURCC('B', 'C', '7', 'U');

	glm::uint32 const GLI_FOURCC_R16F                          = 0x0000006f;         // 16-bit float Red
	glm::uint32 const GLI_FOURCC_G16R16F                       = 0x00000070;         // 16-bit float Red/Green
	glm::uint32 const GLI_FOURCC_A16B16G16R16F                 = 0x00000071;         // 16-bit float RGBA
	glm::uint32 const GLI_FOURCC_R32F                          = 0x00000072;         // 32-bit float Red
	glm::uint32 const GLI_FOURCC_G32R32F                       = 0x00000073;         // 32-bit float Red/Green
	glm::uint32 const GLI_FOURCC_A32B32G32R32F                 = 0x00000074;         // 32-bit float RGBA

	glm::uint32 const GLI_DDPF_ALPHAPIXELS							= 0x00000001; // The surface has alpha channel information in the pixel format.
	glm::uint32 const GLI_DDPF_ALPHA								= 0x00000002; // The pixel format contains alpha only information
	glm::uint32 const GLI_DDPF_FOURCC                               = 0x00000004; // The FourCC code is valid.
	glm::uint32 const GLI_DDPF_RGB									= 0x00000040; // The RGB data in the pixel format structure is valid.
	//glm::uint32 const GLI_DDPF_COMPRESSED							= 0x00000080; // The surface will accept pixel data in the format specified and compress it during the write.
	//glm::uint32 const GLI_DDPF_RGBTOYUV								= 0x00000100; // The surface will accept RGB data and translate it during the write to YUV data.
	glm::uint32 const GLI_DDPF_YUV                                  = 0x00000200; // Pixel format is YUV - YUV data in pixel format struct is valid.
	//glm::uint32 const GLI_DDPF_ZBUFFER                              = 0x00000400; // Pixel format is a z buffer only surface
	//glm::uint32 const GLI_DDPF_ZPIXELS                              = 0x00002000; // The surface contains Z information in the pixels
	//glm::uint32 const GLI_DDPF_STENCILBUFFER                        = 0x00004000; // The surface contains stencil information along with Z
	//glm::uint32 const GLI_DDPF_ALPHAPREMULT                         = 0x00008000; // Premultiplied alpha format -- the color components have been premultiplied by the alpha component.
	glm::uint32 const GLI_DDPF_LUMINANCE                            = 0x00020000; // Luminance data in the pixel format is valid.
	//glm::uint32 const GLI_DDPF_BUMPLUMINANCE                        = 0x00040000; // Use this flag for luminance-only or luminance+alpha surfaces, the bit depth is then ddpf.dwLuminanceBitCount.
	//glm::uint32 const GLI_DDPF_BUMPDUDV                             = 0x00080000; // Bump map dUdV data in the pixel format is valid.

	glm::uint32 const GLI_DDSD_CAPS				= 0x00000001;
	glm::uint32 const GLI_DDSD_HEIGHT			= 0x00000002;
	glm::uint32 const GLI_DDSD_WIDTH			= 0x00000004;
	glm::uint32 const GLI_DDSD_PITCH			= 0x00000008;
	glm::uint32 const GLI_DDSD_PIXELFORMAT		= 0x00001000;
	glm::uint32 const GLI_DDSD_MIPMAPCOUNT		= 0x00020000;
	glm::uint32 const GLI_DDSD_LINEARSIZE		= 0x00080000;
	glm::uint32 const GLI_DDSD_DEPTH			= 0x00800000;

	struct DDLoader
	{
		glm::uint32 BlockSize;
		glm::uint32 BPP;
		gli::format Format;
	};

	enum format_type
	{
		FORMAT_TYPE_NULL,
		FORMAT_RGBA,
		FORMAT_FOURCC
	};

	inline glm::uint32 getFormatFourCC(gli::texture2D const & Image)
	{
		switch(Image.format())
		{
		default:
			return 0;
		case DXT1:
			return GLI_FOURCC_DXT1;
		case DXT3:
			return GLI_FOURCC_DXT3;
		case DXT5:
			return GLI_FOURCC_DXT5;
		case ATI1N_UNORM:
		case ATI1N_SNORM:
		case ATI2N_UNORM:
		case ATI2N_SNORM:
		case BP_UF16:
		case BP_SF16:
		case BP:
			return GLI_FOURCC_DX10;
		case R16F:
			return GLI_FOURCC_R16F;
		case RG16F:
			return GLI_FOURCC_G16R16F;
		case RGBA16F:
			return GLI_FOURCC_A16B16G16R16F;
		case R32F:
			return GLI_FOURCC_R32F;
		case RG32F:
			return GLI_FOURCC_G32R32F;
		case RGBA32F:
			return GLI_FOURCC_A32B32G32R32F;
		}
	}

	inline glm::uint32 getFormatBlockSize(gli::texture2D const & Image)
	{
		switch(Image.format())
		{
		default:
			return 0;
		case DXT1:
			return 8;
		case DXT3:
			return 16;
		case DXT5:
			return 16;
		case ATI1N_UNORM:
		case ATI1N_SNORM:
			return 16;
		case ATI2N_UNORM:
		case ATI2N_SNORM:
			return 32;
		case BP_UF16:
		case BP_SF16:
			return 32;
		case BP:
			return 32;
		case R16F:
			return 2;
		case RG16F:
			return 4;
		case RGBA16F:
			return 8;
		case R32F:
			return 4;
		case RG32F:
			return 8;
		case RGBA32F:
			return 16;
		}
	}

	inline glm::uint32 getFormatFlags(gli::texture2D const & Image)
	{
		glm::uint32 Result = 0;

		switch(Image.format())
		{
		default: 
			break;
		case R8U:
		case RG8U:
		case RGB8U:
		case RGBA8U:
		case R16U:
		case RG16U:
		case RGB16U:
		case RGBA16U:
		case R32U:
		case RG32U:
		case RGB32U:
		case RGBA32U:
		case R8I:
		case RG8I:
		case RGB8I:
		case RGBA8I:
		case R16I:
		case RG16I:
		case RGB16I:
		case RGBA16I:
		case R32I:
		case RG32I:
		case RGB32I:
		case RGBA32I:
			Result |= GLI_DDPF_RGB;
			break;
		case R16F:
		case RG16F:
		case RGB16F:
		case RGBA16F:
		case R32F:
		case RG32F:
		case RGB32F:
		case RGBA32F:
		case RGBE8:
		case RGB9E5:
		case RG11B10F:
		case R5G6B5:
		case RGBA4:
		case RGB10A2:
		case D16:
		case D24X8:
		case D24S8:
		case D32F:
		case D32FS8X24:
		case DXT1:
		case DXT3:
		case DXT5:
		case ATI1N_UNORM:
		case ATI1N_SNORM:
		case ATI2N_UNORM:
		case ATI2N_SNORM:
		case BP_UF16:
		case BP_SF16:
		case BP:
			Result |= GLI_DDPF_FOURCC;
			break;
		};

		return Result;
	}

	inline glm::uint32 getFormatBPP(gli::texture2D const & Image)
	{
		switch(Image.format())
		{
		default:
			return 0;
		case R8U:
		case R8I:
			return 8;
		case RG8U:
		case RG8I:
			return 16;
		case RGB8U:
		case RGB8I:
			return 24;
		case RGBA8U:
		case RGBA8I:
			return 32;
		case DXT1:
			return 4;
		case DXT3:
			return 8;
		case DXT5:
			return 8;
		case ATI1N_UNORM:
		case ATI1N_SNORM:
			return 4;
		case ATI2N_UNORM:
		case ATI2N_SNORM:
			return 8;
		case BP_UF16:
		case BP_SF16:
			return 8;
		case BP:
			return 8;
		}
	}

	inline bool isCompressed(gli::texture2D const & Image)
	{
		switch(Image.format())
		{
		default:
			return false;
		case DXT1:
		case DXT3:
		case DXT5:
		case ATI1N_UNORM:
		case ATI1N_SNORM:
		case ATI2N_UNORM:
		case ATI2N_SNORM:
		case BP_UF16:
		case BP_SF16:
		case BP:
			return true;
		}
		return false;
	}

}//namespace detail

	inline texture2D loadDDS9
	(
		std::string const & Filename
	)
	{
		std::ifstream FileIn(Filename.c_str(), std::ios::in | std::ios::binary);
		if(FileIn.fail())
			return texture2D();

		detail::ddsHeader SurfaceDesc;
		char Magic[4]; 

		//* Read magic number and check if valid .dds file 
		FileIn.read((char*)&Magic, sizeof(Magic));

		assert(strncmp(Magic, "DDS ", 4) == 0);

		// Get the surface descriptor 
		FileIn.read((char*)&SurfaceDesc, sizeof(SurfaceDesc));

		std::size_t Width = SurfaceDesc.width;
		std::size_t Height = SurfaceDesc.height;

		//std::size_t Levels = glm::max(glm::highestBit(Width), glm::highestBit(Height));

		detail::DDLoader Loader;
		if(SurfaceDesc.format.flags & detail::GLI_DDPF_FOURCC)
		{
			switch(SurfaceDesc.format.fourCC)
			{
			case detail::GLI_FOURCC_DX10:
				assert(0);
				break;
			case detail::GLI_FOURCC_DXT1:
				Loader.BlockSize = 8;
				Loader.Format = DXT1;
				break;
			case detail::GLI_FOURCC_DXT3:
				Loader.BlockSize = 16;
				Loader.Format = DXT3;
				break;
			case detail::GLI_FOURCC_DXT5:
				Loader.BlockSize = 16;
				Loader.Format = DXT5;
				break;
			case detail::GLI_FOURCC_R16F:
				Loader.BlockSize = 2;
				Loader.Format = R16F;
				break;
			case detail::GLI_FOURCC_G16R16F:
				Loader.BlockSize = 4;
				Loader.Format = RG16F;
				break;
			case detail::GLI_FOURCC_A16B16G16R16F:
				Loader.BlockSize = 8;
				Loader.Format = RGBA16F;
				break;
			case detail::GLI_FOURCC_R32F:
				Loader.BlockSize = 4;
				Loader.Format = R32F;
				break;
			case detail::GLI_FOURCC_G32R32F:
				Loader.BlockSize = 8;
				Loader.Format = RG32F;
				break;
			case detail::GLI_FOURCC_A32B32G32R32F:
				Loader.BlockSize = 16;
				Loader.Format = RGBA32F;
				break;

			default:
				assert(0);
				return texture2D();
			}
		}
		else if(SurfaceDesc.format.flags & detail::GLI_DDPF_RGB)
		{
			switch(SurfaceDesc.format.bpp)
			{
			case 8:
				Loader.BlockSize = 2;
				Loader.Format = R8U;
				break;
			case 16:
				Loader.BlockSize = 2;
				Loader.Format = RG8U;
				break;
			case 24:
				Loader.BlockSize = 3;
				Loader.Format = RGB8U;
				break;
			case 32:
				Loader.BlockSize = 4;
				Loader.Format = RGBA8U;
				break;
			}
		}
		else
		{

		}

		gli::format Format = Loader.Format;

		std::streamoff Curr = FileIn.tellg();
		FileIn.seekg(0, std::ios_base::end);
		std::streamoff End = FileIn.tellg();
		FileIn.seekg(Curr, std::ios_base::beg);

		std::vector<glm::byte> Data(std::size_t(End - Curr), 0);
		std::size_t Offset = 0;

		FileIn.read((char*)&Data[0], std::streamsize(Data.size()));

		//image Image(glm::min(MipMapCount, Levels));//SurfaceDesc.mipMapLevels);
		std::size_t MipMapCount = (SurfaceDesc.flags & detail::GLI_DDSD_MIPMAPCOUNT) ? SurfaceDesc.mipMapLevels : 1;
		//if(Loader.Format == DXT1 || Loader.Format == DXT3 || Loader.Format == DXT5) 
		//	MipMapCount -= 2;
		texture2D Image(MipMapCount);
		for(std::size_t Level = 0; Level < Image.levels() && (Width || Height); ++Level)
		{
			Width = glm::max(std::size_t(Width), std::size_t(1));
			Height = glm::max(std::size_t(Height), std::size_t(1));

			std::size_t MipmapSize = 0;
			if(Loader.Format == DXT1 || Loader.Format == DXT3 || Loader.Format == DXT5)
				MipmapSize = ((Width + 3) >> 2) * ((Height + 3) >> 2) * Loader.BlockSize;
			else
				MipmapSize = Width * Height * Loader.BlockSize;
			std::vector<glm::byte> MipmapData(MipmapSize, 0);

			memcpy(&MipmapData[0], &Data[0] + Offset, MipmapSize);

			image2D::dimensions_type Dimensions(Width, Height);
			Image[Level] = image2D(Dimensions, Format, MipmapData);

			Offset += MipmapSize;
			Width >>= 1;
			Height >>= 1;
		}

		return Image;
	}

	inline textureCube loadTextureCubeDDS9
	(
		std::string const & Filename
	)
	{
		std::ifstream FileIn(Filename.c_str(), std::ios::in | std::ios::binary);
		if(FileIn.fail())
			return textureCube();

		detail::ddsHeader SurfaceDesc;
		char Magic[4]; 

		//* Read magic number and check if valid .dds file 
		FileIn.read((char*)&Magic, sizeof(Magic));

		assert(strncmp(Magic, "DDS ", 4) == 0);

		// Get the surface descriptor 
		FileIn.read((char*)&SurfaceDesc, sizeof(SurfaceDesc));

		std::size_t Width = SurfaceDesc.width;
		std::size_t Height = SurfaceDesc.height;

		//std::size_t Levels = glm::max(glm::highestBit(Width), glm::highestBit(Height));

		detail::DDLoader Loader;
		if(SurfaceDesc.format.flags & detail::GLI_DDPF_FOURCC)
		{
			switch(SurfaceDesc.format.fourCC)
			{
			case detail::GLI_FOURCC_DX10:
				assert(0);
				break;
			case detail::GLI_FOURCC_DXT1:
				Loader.BlockSize = 8;
				Loader.Format = DXT1;
				break;
			case detail::GLI_FOURCC_DXT3:
				Loader.BlockSize = 16;
				Loader.Format = DXT3;
				break;
			case detail::GLI_FOURCC_DXT5:
				Loader.BlockSize = 16;
				Loader.Format = DXT5;
				break;
			case detail::GLI_FOURCC_R16F:
				Loader.BlockSize = 2;
				Loader.Format = R16F;
				break;
			case detail::GLI_FOURCC_G16R16F:
				Loader.BlockSize = 4;
				Loader.Format = RG16F;
				break;
			case detail::GLI_FOURCC_A16B16G16R16F:
				Loader.BlockSize = 8;
				Loader.Format = RGBA16F;
				break;
			case detail::GLI_FOURCC_R32F:
				Loader.BlockSize = 4;
				Loader.Format = R32F;
				break;
			case detail::GLI_FOURCC_G32R32F:
				Loader.BlockSize = 8;
				Loader.Format = RG32F;
				break;
			case detail::GLI_FOURCC_A32B32G32R32F:
				Loader.BlockSize = 16;
				Loader.Format = RGBA32F;
				break;

			default:
				assert(0);
				return textureCube();
			}
		}
		else if(SurfaceDesc.format.flags & detail::GLI_DDPF_RGB)
		{
			switch(SurfaceDesc.format.bpp)
			{
			case 8:
				Loader.BlockSize = 2;
				Loader.Format = R8U;
				break;
			case 16:
				Loader.BlockSize = 2;
				Loader.Format = RG8U;
				break;
			case 24:
				Loader.BlockSize = 3;
				Loader.Format = RGB8U;
				break;
			case 32:
				Loader.BlockSize = 4;
				Loader.Format = RGBA8U;
				break;
			}
		}
		else
		{

		}

		gli::format Format = Loader.Format;

		std::streamoff Curr = FileIn.tellg();
		FileIn.seekg(0, std::ios_base::end);
		std::streamoff End = FileIn.tellg();
		FileIn.seekg(Curr, std::ios_base::beg);

		std::vector<glm::byte> Data(std::size_t(End - Curr), 0);
		std::size_t Offset = 0;

		FileIn.read((char*)&Data[0], std::streamsize(Data.size()));

		//image Image(glm::min(MipMapCount, Levels));//SurfaceDesc.mipMapLevels);
		std::size_t MipMapCount = (SurfaceDesc.flags & detail::GLI_DDSD_MIPMAPCOUNT) ? SurfaceDesc.mipMapLevels : 1;
		//if(Loader.Format == DXT1 || Loader.Format == DXT3 || Loader.Format == DXT5) 
		//	MipMapCount -= 2;
		textureCube Texture(MipMapCount);

		for(textureCube::size_type Face = 0; Face < FACE_MAX; ++Face)
		{
			Width = SurfaceDesc.width;
			Height = SurfaceDesc.height;

			for(textureCube::size_type Level = 0; Level < Texture.levels() && (Width || Height); ++Level)
			{
				Width = glm::max(std::size_t(Width), std::size_t(1));
				Height = glm::max(std::size_t(Height), std::size_t(1));

				std::size_t MipmapSize = 0;
				if(Loader.Format == DXT1 || Loader.Format == DXT3 || Loader.Format == DXT5)
					MipmapSize = ((Width + 3) >> 2) * ((Height + 3) >> 2) * Loader.BlockSize;
				else
					MipmapSize = Width * Height * Loader.BlockSize;
				std::vector<glm::byte> MipmapData(MipmapSize, 0);

				memcpy(&MipmapData[0], &Data[0] + Offset, MipmapSize);

				textureCube::dimensions_type Dimensions(Width, Height);
				Texture[textureCube::face_type(Face)][Level] = image2D(Dimensions, Format, MipmapData);

				Offset += MipmapSize;
				Width >>= 1;
				Height >>= 1;
			}
		}

		return Texture;
	}

	inline void saveDDS9
	(
		texture2D const & Texture, 
		std::string const & Filename
	)
	{
		std::ofstream FileOut(Filename.c_str(), std::ios::out | std::ios::binary);
		if (!FileOut)
			return;

		char const * Magic = "DDS ";
		FileOut.write((char*)Magic, sizeof(char) * 4);

		glm::uint32 Caps = detail::GLI_DDSD_CAPS | detail::GLI_DDSD_HEIGHT | detail::GLI_DDSD_WIDTH | detail::GLI_DDSD_PIXELFORMAT;

		detail::ddsHeader SurfaceDesc;
		SurfaceDesc.size = sizeof(detail::ddsHeader);
		SurfaceDesc.flags = Caps | (detail::isCompressed(Texture) ? detail::GLI_DDSD_LINEARSIZE : detail::GLI_DDSD_PITCH) | (Texture.levels() > 1 ? detail::GLI_DDSD_MIPMAPCOUNT : 0); //659463;
		SurfaceDesc.width = Texture[0].dimensions().x;
		SurfaceDesc.height = Texture[0].dimensions().y;
		SurfaceDesc.pitch = loader_dds9::detail::isCompressed(Texture) ? size(Texture, LINEAR_SIZE) : 32;
		SurfaceDesc.depth = 0;
		SurfaceDesc.mipMapLevels = glm::uint32(Texture.levels());
		SurfaceDesc.format.size = sizeof(detail::ddsPixelFormat);
		SurfaceDesc.format.flags = detail::getFormatFlags(Texture);
		SurfaceDesc.format.fourCC = detail::getFormatFourCC(Texture);
		SurfaceDesc.format.bpp = detail::getFormatBPP(Texture);
		SurfaceDesc.format.redMask = 0;
		SurfaceDesc.format.greenMask = 0;
		SurfaceDesc.format.blueMask = 0;
		SurfaceDesc.format.alphaMask = 0;
		SurfaceDesc.surfaceFlags = detail::GLI_DDSCAPS_TEXTURE | (Texture.levels() > 1 ? detail::GLI_DDSCAPS_MIPMAP : 0);
		SurfaceDesc.cubemapFlags = 0;

		FileOut.write((char*)&SurfaceDesc, sizeof(SurfaceDesc));

		for(texture2D::level_type Level = 0; Level < Texture.levels(); ++Level)
		{
			texture2D::size_type ImageSize = size(Texture[Level], gli::LINEAR_SIZE);
			FileOut.write((char*)(Texture[Level].data()), ImageSize);
		}

		if(FileOut.fail() || FileOut.bad())
			return;

		FileOut.close ();
	}

	inline void saveTextureCubeDDS9
	(
		textureCube const & Texture, 
		std::string const & Filename
	)
	{
		std::ofstream FileOut(Filename.c_str(), std::ios::out | std::ios::binary);
		if (!FileOut || Texture.empty())
			return;

		char const * Magic = "DDS ";
		FileOut.write((char*)Magic, sizeof(char) * 4);

		glm::uint32 Caps = detail::GLI_DDSD_CAPS | detail::GLI_DDSD_HEIGHT | detail::GLI_DDSD_WIDTH | detail::GLI_DDSD_PIXELFORMAT | detail::GLI_DDSCAPS_COMPLEX;

		detail::ddsHeader SurfaceDesc;
		SurfaceDesc.size = sizeof(detail::ddsHeader);
		SurfaceDesc.flags = Caps | (detail::isCompressed(Texture[POSITIVE_X]) ? detail::GLI_DDSD_LINEARSIZE : detail::GLI_DDSD_PITCH) | (Texture.levels() > 1 ? detail::GLI_DDSD_MIPMAPCOUNT : 0); //659463;
		SurfaceDesc.width = Texture[POSITIVE_X][0].dimensions().x;
		SurfaceDesc.height = Texture[POSITIVE_X][0].dimensions().y;
		SurfaceDesc.pitch = loader_dds9::detail::isCompressed(Texture[POSITIVE_X]) ? size(Texture[POSITIVE_X], LINEAR_SIZE) : 32;
		SurfaceDesc.depth = 0;
		SurfaceDesc.mipMapLevels = glm::uint32(Texture.levels());
		SurfaceDesc.format.size = sizeof(detail::ddsPixelFormat);
		SurfaceDesc.format.flags = detail::getFormatFlags(Texture[POSITIVE_X]);
		SurfaceDesc.format.fourCC = detail::getFormatFourCC(Texture[POSITIVE_X]);
		SurfaceDesc.format.bpp = detail::getFormatBPP(Texture[POSITIVE_X]);
		SurfaceDesc.format.redMask = 0;
		SurfaceDesc.format.greenMask = 0;
		SurfaceDesc.format.blueMask = 0;
		SurfaceDesc.format.alphaMask = 0;
		SurfaceDesc.surfaceFlags = detail::GLI_DDSCAPS_TEXTURE | (Texture.levels() > 1 ? detail::GLI_DDSCAPS_MIPMAP : 0);
		SurfaceDesc.cubemapFlags = 
			detail::GLI_DDSCAPS2_CUBEMAP | detail::GLI_DDSCAPS2_CUBEMAP_POSITIVEX | detail::GLI_DDSCAPS2_CUBEMAP_NEGATIVEX | detail::GLI_DDSCAPS2_CUBEMAP_POSITIVEY | detail::GLI_DDSCAPS2_CUBEMAP_NEGATIVEY | detail::GLI_DDSCAPS2_CUBEMAP_POSITIVEZ | detail::GLI_DDSCAPS2_CUBEMAP_NEGATIVEZ;

		FileOut.write((char*)&SurfaceDesc, sizeof(SurfaceDesc));

		for(textureCube::size_type Face = 0; Face < FACE_MAX; ++Face)
		for(texture2D::level_type Level = 0; Level < Texture.levels(); ++Level)
		{
			texture2D::size_type ImageSize = size(Texture[textureCube::face_type(Face)][Level], gli::LINEAR_SIZE);
			FileOut.write((char*)(Texture[textureCube::face_type(Face)][Level].data()), ImageSize);
		}

		if(FileOut.fail() || FileOut.bad())
			return;

		FileOut.close ();
	}

}//namespace loader_dds9
}//namespace gtx
}//namespace gli
