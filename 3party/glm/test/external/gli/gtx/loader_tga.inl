///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Image Copyright (c) 2008 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2010-09-08
// Updated : 2010-09-27
// Licence : This source is under MIT License
// File    : gli/gtx/loader_tga.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace gli{
namespace gtx{
namespace loader_tga
{
	inline texture2D loadTGA
	(
		std::string const & Filename
	)
	{
		std::ifstream FileIn(Filename.c_str(), std::ios::in | std::ios::binary);
		if(!FileIn)
			return texture2D();

		unsigned char IdentificationFieldSize;
		unsigned char ColorMapType;
		unsigned char ImageType;
		unsigned short ColorMapOrigin;
		unsigned short ColorMapLength;
		unsigned char ColorMapEntrySize;
		unsigned short OriginX;
		unsigned short OriginY;
		unsigned short Width;
		unsigned short Height;
		unsigned char TexelSize;
		unsigned char Descriptor;

		FileIn.read((char*)&IdentificationFieldSize, sizeof(IdentificationFieldSize));
		FileIn.read((char*)&ColorMapType, sizeof(ColorMapType));
		FileIn.read((char*)&ImageType, sizeof(ImageType));
		FileIn.read((char*)&ColorMapOrigin, sizeof(ColorMapOrigin));
		FileIn.read((char*)&ColorMapLength, sizeof(ColorMapLength));
		FileIn.read((char*)&ColorMapEntrySize, sizeof(ColorMapEntrySize));
		FileIn.read((char*)&OriginX, sizeof(OriginX));
		FileIn.read((char*)&OriginY, sizeof(OriginY));
		FileIn.read((char*)&Width, sizeof(Width));
		FileIn.read((char*)&Height, sizeof(Height));
		FileIn.read((char*)&TexelSize, sizeof(TexelSize));
		FileIn.read((char*)&Descriptor, sizeof(Descriptor));

		gli::format Format = gli::FORMAT_NULL;
		if(TexelSize == 24)
			Format = gli::RGB8U;
		else if(TexelSize == 32)
			Format = gli::RGBA8U;
		else
			assert(0);

		image2D Mipmap(texture2D::dimensions_type(Width, Height), Format);

		if (FileIn.fail() || FileIn.bad())
		{
			assert(0);
			return texture2D();
		}

		switch(ImageType)
		{
		default:
			assert(0);
			return texture2D();

		case 2:
			FileIn.seekg(18 + ColorMapLength, std::ios::beg);

			char* IdentificationField = new char[IdentificationFieldSize + 1];
			FileIn.read(IdentificationField, IdentificationFieldSize);
			IdentificationField[IdentificationFieldSize] = '\0';
			delete[] IdentificationField;

			std::size_t DataSize = Width * Height * (TexelSize >> 3);
			FileIn.read((char*)Mipmap.data(), std::streamsize(DataSize));

			if(FileIn.fail() || FileIn.bad())
				return texture2D();
			break;
		}

		FileIn.close();

		texture2D Image(1);
		Image[0] = Mipmap;

		// TGA images are saved in BGR or BGRA format.
		if(TexelSize == 24)
			Image.swizzle<glm::u8vec3>(gli::B, gli::G, gli::R, gli::A);
		if(TexelSize == 32)
			Image.swizzle<glm::u8vec4>(gli::B, gli::G, gli::R, gli::A);

		return Image;
	}

	inline void saveTGA
	(
		gli::texture2D const & ImageIn, 
		std::string const & Filename
	)
	{
		std::ofstream FileOut(Filename.c_str(), std::ios::out | std::ios::binary);
		if (!FileOut)
			return;

		gli::texture2D Image = duplicate(ImageIn);

		unsigned char IdentificationFieldSize = 1;
		unsigned char ColorMapType = 0;
		unsigned char ImageType = 2;
		unsigned short ColorMapOrigin = 0;
		unsigned short ColorMapLength = 0;
		unsigned char ColorMapEntrySize = 0;
		unsigned short OriginX = 0;
		unsigned short OriginY = 0;
		unsigned short Width = Image[0].dimensions().x;
		unsigned short Height = Image[0].dimensions().y;
		unsigned char TexelSize = (unsigned char)(Image[0].value_size());
		unsigned char Descriptor = 0;

		if(TexelSize == 24)
			Image.swizzle<glm::u8vec3>(gli::B, gli::G, gli::R, gli::A);
		if(TexelSize == 32)
			Image.swizzle<glm::u8vec4>(gli::B, gli::G, gli::R, gli::A);

		FileOut.write((char*)&IdentificationFieldSize, sizeof(IdentificationFieldSize));
		FileOut.write((char*)&ColorMapType, sizeof(ColorMapType));
		FileOut.write((char*)&ImageType, sizeof(ImageType));
		FileOut.write((char*)&ColorMapOrigin, sizeof(ColorMapOrigin));
		FileOut.write((char*)&ColorMapLength, sizeof(ColorMapLength));
		FileOut.write((char*)&ColorMapEntrySize, sizeof(ColorMapEntrySize));
		FileOut.write((char*)&OriginX, sizeof(OriginX));
		FileOut.write((char*)&OriginY, sizeof(OriginY));
		FileOut.write((char*)&Width, sizeof(Width));
		FileOut.write((char*)&Height, sizeof(Height));
		FileOut.write((char*)&TexelSize, sizeof(TexelSize));
		FileOut.write((char*)&Descriptor, sizeof(Descriptor));

		if (FileOut.fail () || FileOut.bad ())
			return;

		FileOut.seekp(18 + ColorMapLength, std::ios::beg);
		char* IdentificationField = new char[IdentificationFieldSize + 1];
		FileOut.write(IdentificationField, std::streamsize(IdentificationFieldSize));
		delete[] IdentificationField;
		FileOut.write((char*)Image[0].data(), std::streamsize(Image[0].capacity()));
		if(FileOut.fail() || FileOut.bad())
			return;

		FileOut.close ();
	}
}//namespace loader_tga
}//namespace gtx
}//namespace gli
