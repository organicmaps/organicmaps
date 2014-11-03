///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2011-04-21
// Updated : 2011-04-26
// Licence : This source is under MIT licence
// File    : test/gtc/noise.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#define GLM_FORCE_RADIANS
#include <glm/gtc/noise.hpp>
#include <gli/gli.hpp>
#include <gli/gtx/loader.hpp>
#include <iostream>

int test_simplex()
{
	std::size_t const Size = 256;

	{
		std::vector<glm::byte> ImageData(Size * Size * 3);
		
		for(std::size_t y = 0; y < Size; ++y)
		for(std::size_t x = 0; x < Size; ++x)
		{
			ImageData[(x + y * Size) * 3 + 0] = glm::byte(glm::simplex(glm::vec2(x / 64.f, y / 64.f)) * 128.f + 127.f);
			ImageData[(x + y * Size) * 3 + 1] = ImageData[(x + y * Size) * 3 + 0];
			ImageData[(x + y * Size) * 3 + 2] = ImageData[(x + y * Size) * 3 + 0];
		}

		gli::texture2D Texture(1);
		Texture[0] = gli::image2D(glm::uvec2(Size), gli::RGB8U);
		memcpy(Texture[0].data(), &ImageData[0], ImageData.size());
		gli::saveDDS9(Texture, "texture_simplex2d_256.dds");
	}

	{
		std::vector<glm::byte> ImageData(Size * Size * 3);
		
		for(std::size_t y = 0; y < Size; ++y)
		for(std::size_t x = 0; x < Size; ++x)
		{
			ImageData[(x + y * Size) * 3 + 0] = glm::byte(glm::simplex(glm::vec3(x / 64.f, y / 64.f, 0.5f)) * 128.f + 127.f);
			ImageData[(x + y * Size) * 3 + 1] = ImageData[(x + y * Size) * 3 + 0];
			ImageData[(x + y * Size) * 3 + 2] = ImageData[(x + y * Size) * 3 + 0];
		}

		gli::texture2D Texture(1);
		Texture[0] = gli::image2D(glm::uvec2(Size), gli::RGB8U);
		memcpy(Texture[0].data(), &ImageData[0], ImageData.size());
		gli::saveDDS9(Texture, "texture_simplex3d_256.dds");
	}
	
	{
		std::vector<glm::byte> ImageData(Size * Size * 3);
		
		for(std::size_t y = 0; y < Size; ++y)
		for(std::size_t x = 0; x < Size; ++x)
		{
			ImageData[(x + y * Size) * 3 + 0] = glm::byte(glm::simplex(glm::vec4(x / 64.f, y / 64.f, 0.5f, 0.5f)) * 128.f + 127.f);
			ImageData[(x + y * Size) * 3 + 1] = ImageData[(x + y * Size) * 3 + 0];
			ImageData[(x + y * Size) * 3 + 2] = ImageData[(x + y * Size) * 3 + 0];
		}

		gli::texture2D Texture(1);
		Texture[0] = gli::image2D(glm::uvec2(Size), gli::RGB8U);
		memcpy(Texture[0].data(), &ImageData[0], ImageData.size());
		gli::saveDDS9(Texture, "texture_simplex4d_256.dds");
	}

	return 0;
}

int test_perlin()
{
	std::size_t const Size = 256;

	{
		std::vector<glm::byte> ImageData(Size * Size * 3);
		
		for(std::size_t y = 0; y < Size; ++y)
		for(std::size_t x = 0; x < Size; ++x)
		{
			ImageData[(x + y * Size) * 3 + 0] = glm::byte(glm::perlin(glm::vec2(x / 64.f, y / 64.f)) * 128.f + 127.f);
			ImageData[(x + y * Size) * 3 + 1] = ImageData[(x + y * Size) * 3 + 0];
			ImageData[(x + y * Size) * 3 + 2] = ImageData[(x + y * Size) * 3 + 0];
		}

		gli::texture2D Texture(1);
		Texture[0] = gli::image2D(glm::uvec2(Size), gli::RGB8U);
		memcpy(Texture[0].data(), &ImageData[0], ImageData.size());
		gli::saveDDS9(Texture, "texture_perlin2d_256.dds");
	}

	{
		std::vector<glm::byte> ImageData(Size * Size * 3);
		
		for(std::size_t y = 0; y < Size; ++y)
		for(std::size_t x = 0; x < Size; ++x)
		{
			ImageData[(x + y * Size) * 3 + 0] = glm::byte(glm::perlin(glm::vec3(x / 64.f, y / 64.f, 0.5f)) * 128.f + 127.f);
			ImageData[(x + y * Size) * 3 + 1] = ImageData[(x + y * Size) * 3 + 0];
			ImageData[(x + y * Size) * 3 + 2] = ImageData[(x + y * Size) * 3 + 0];
		}

		gli::texture2D Texture(1);
		Texture[0] = gli::image2D(glm::uvec2(Size), gli::RGB8U);
		memcpy(Texture[0].data(), &ImageData[0], ImageData.size());
		gli::saveDDS9(Texture, "texture_perlin3d_256.dds");
	}
	
	{
		std::vector<glm::byte> ImageData(Size * Size * 3);
		
		for(std::size_t y = 0; y < Size; ++y)
		for(std::size_t x = 0; x < Size; ++x)
		{
			ImageData[(x + y * Size) * 3 + 0] = glm::byte(glm::perlin(glm::vec4(x / 64.f, y / 64.f, 0.5f, 0.5f)) * 128.f + 127.f);
			ImageData[(x + y * Size) * 3 + 1] = ImageData[(x + y * Size) * 3 + 0];
			ImageData[(x + y * Size) * 3 + 2] = ImageData[(x + y * Size) * 3 + 0];
		}

		gli::texture2D Texture(1);
		Texture[0] = gli::image2D(glm::uvec2(Size), gli::RGB8U);
		memcpy(Texture[0].data(), &ImageData[0], ImageData.size());
		gli::saveDDS9(Texture, "texture_perlin4d_256.dds");
	}

	return 0;
}

int test_perlin_pedioric()
{
	std::size_t const Size = 256;

	{
		std::vector<glm::byte> ImageData(Size * Size * 3);
		
		for(std::size_t y = 0; y < Size; ++y)
		for(std::size_t x = 0; x < Size; ++x)
		{
			ImageData[(x + y * Size) * 3 + 0] = glm::byte(glm::perlin(glm::vec2(x / 64.f, y / 64.f), glm::vec2(2.0f)) * 128.f + 127.f);
			ImageData[(x + y * Size) * 3 + 1] = ImageData[(x + y * Size) * 3 + 0];
			ImageData[(x + y * Size) * 3 + 2] = ImageData[(x + y * Size) * 3 + 0];
		}

		gli::texture2D Texture(1);
		Texture[0] = gli::image2D(glm::uvec2(Size), gli::RGB8U);
		memcpy(Texture[0].data(), &ImageData[0], ImageData.size());
		gli::saveDDS9(Texture, "texture_perlin_pedioric_2d_256.dds");
	}

	{
		std::vector<glm::byte> ImageData(Size * Size * 3);
		
		for(std::size_t y = 0; y < Size; ++y)
		for(std::size_t x = 0; x < Size; ++x)
		{
			ImageData[(x + y * Size) * 3 + 0] = glm::byte(glm::perlin(glm::vec3(x / 64.f, y / 64.f, 0.5f), glm::vec3(2.0f)) * 128.f + 127.f);
			ImageData[(x + y * Size) * 3 + 1] = ImageData[(x + y * Size) * 3 + 0];
			ImageData[(x + y * Size) * 3 + 2] = ImageData[(x + y * Size) * 3 + 0];
		}

		gli::texture2D Texture(1);
		Texture[0] = gli::image2D(glm::uvec2(Size), gli::RGB8U);
		memcpy(Texture[0].data(), &ImageData[0], ImageData.size());
		gli::saveDDS9(Texture, "texture_perlin_pedioric_3d_256.dds");
	}
	
	{
		std::vector<glm::byte> ImageData(Size * Size * 3);
		
		for(std::size_t y = 0; y < Size; ++y)
		for(std::size_t x = 0; x < Size; ++x)
		{
			ImageData[(x + y * Size) * 3 + 0] = glm::byte(glm::perlin(glm::vec4(x / 64.f, y / 64.f, 0.5f, 0.5f), glm::vec4(2.0f)) * 128.f + 127.f);
			ImageData[(x + y * Size) * 3 + 1] = ImageData[(x + y * Size) * 3 + 0];
			ImageData[(x + y * Size) * 3 + 2] = ImageData[(x + y * Size) * 3 + 0];
		}

		gli::texture2D Texture(1);
		Texture[0] = gli::image2D(glm::uvec2(Size), gli::RGB8U);
		memcpy(Texture[0].data(), &ImageData[0], ImageData.size());
		gli::saveDDS9(Texture, "texture_perlin_pedioric_4d_256.dds");
	}

	return 0;
}

int main()
{
	int Error = 0;

	Error += test_simplex();
	Error += test_perlin();
	Error += test_perlin_pedioric();

	return Error;
}
