///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Image Copyright (c) 2008 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2010-01-19
// Updated : 2010-01-19
// Licence : This source is under MIT License
// File    : gli/core/operator.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace gli
{
	namespace detail
	{
		template <typename T>
		void element
		(
			T & DataDst,
			T const & DataSrcA,
			T const & DataSrcB,
			std::binary_function<T, T, T> const & Func
		)
		{
			*DataDst = Func(DataSrcA, DataSrcB);
		}

		void op
		(
			texture2D::value_type * DataDst,
			texture2D::value_type const * const DataSrcA,
			texture2D::value_type const * const DataSrcB,
			format Format
		)
		{
			std::plus<>()
			switch(Format)
			{
			case R8U:
				*((glm::u8*)DataDst) = *((glm::u8*)DataSrcA) + *((glm::u8*)DataSrcB);
				break;
			case RG8U:
				*((glm::u8vec2*)DataDst) = *((glm::u8vec2*)DataSrcA) + *((glm::u8vec2*)DataSrcB);
				break;
			case RGB8U:
				*((glm::u8vec3*)DataDst) = *((glm::u8vec3*)DataSrcA) + *((glm::u8vec3*)DataSrcB);
				break;
			case RGBA8U:
				*((glm::u8vec4*)DataDst) = *((glm::u8vec4*)DataSrcA) + *((glm::u8vec4*)DataSrcB);
				break;

			case R16U:
				*((glm::u16*)DataDst) = *((glm::u16*)DataSrcA) + *((glm::u16*)DataSrcB);
				break;
			case RG16U:
				*((glm::u16vec2*)DataDst) = *((glm::u16vec2*)DataSrcA) + *((glm::u16vec2*)DataSrcB);
				break;
			case RGB16U:
				*((glm::u16vec3*)DataDst) = *((glm::u16vec3*)DataSrcA) + *((glm::u16vec3*)DataSrcB);
				break;
			case RGBA16U:
				*((glm::u16vec4*)DataDst) = *((glm::u16vec4*)DataSrcA) + *((glm::u16vec4*)DataSrcB);
				break;

			case R32U:
				*((glm::u32*)DataDst) = *((glm::u32*)DataSrcA) + *((glm::u32*)DataSrcB);
				break;
			case RG32U:
				*((glm::u32vec2*)DataDst) = *((glm::u32vec2*)DataSrcA) + *((glm::u32vec2*)DataSrcB);
				break;
			case RGB32U:
				*((glm::u32vec3*)DataDst) = *((glm::u32vec3*)DataSrcA) + *((glm::u32vec3*)DataSrcB);
				break;
			case RGBA32U:
				*((glm::u32vec4*)DataDst) = *((glm::u32vec4*)DataSrcA) + *((glm::u32vec4*)DataSrcB);
				break;

			case R8I:
				*((glm::i8*)DataDst) = *((glm::i8*)DataSrcA) + *((glm::i8*)DataSrcB);
				break;
			case RG8I:
				*((glm::i8vec2*)DataDst) = *((glm::i8vec2*)DataSrcA) + *((glm::i8vec2*)DataSrcB);
				break;
			case RGB8I:
				*((glm::i8vec3*)DataDst) = *((glm::i8vec3*)DataSrcA) + *((glm::i8vec3*)DataSrcB);
				break;
			case RGBA8I:
				*((glm::i8vec4*)DataDst) = *((glm::i8vec4*)DataSrcA) + *((glm::i8vec4*)DataSrcB);
				break;

			case R16I:
				*((glm::i16*)DataDst) = *((glm::i16*)DataSrcA) + *((glm::i16*)DataSrcB);
				break;
			case RG16I:
				*((glm::i16vec2*)DataDst) = *((glm::i16vec2*)DataSrcA) + *((glm::i16vec2*)DataSrcB);
				break;
			case RGB16I:
				*((glm::i16vec3*)DataDst) = *((glm::i16vec3*)DataSrcA) + *((glm::i16vec3*)DataSrcB);
				break;
			case RGBA16I:
				*((glm::i16vec4*)DataDst) = *((glm::i16vec4*)DataSrcA) + *((glm::i16vec4*)DataSrcB);
				break;

			case R32I:
				*((glm::i32*)DataDst) = *((glm::i32*)DataSrcA) + *((glm::i32*)DataSrcB);
				break;
			case RG32I:
				*((glm::i32vec2*)DataDst) = *((glm::i32vec2*)DataSrcA) + *((glm::i32vec2*)DataSrcB);
				break;
			case RGB32I:
				*((glm::i32vec3*)DataDst) = *((glm::i32vec3*)DataSrcA) + *((glm::i32vec3*)DataSrcB);
				break;
			case RGBA32I:
				*((glm::i32vec4*)DataDst) = *((glm::i32vec4*)DataSrcA) + *((glm::i32vec4*)DataSrcB);
				break;

			case R16F:
				*((glm::f16*)DataDst) = *((glm::f16*)DataSrcA) + *((glm::f16*)DataSrcB);
				break;
			case RG16F:
				*((glm::f16vec2*)DataDst) = *((glm::f16vec2*)DataSrcA) + *((glm::f16vec2*)DataSrcB);
				break;
			case RGB16F:
				*((glm::f16vec3*)DataDst) = *((glm::f16vec3*)DataSrcA) + *((glm::f16vec3*)DataSrcB);
				break;
			case RGBA16F:
				*((glm::f16vec4*)DataDst) = *((glm::f16vec4*)DataSrcA) + *((glm::f16vec4*)DataSrcB);
				break;

			case R32F:
				*((glm::f32*)DataDst) = *((glm::f32*)DataSrcA) + *((glm::f32*)DataSrcB);
				break;
			case RG32F:
				*((glm::f32vec2*)DataDst) = *((glm::f32vec2*)DataSrcA) + *((glm::f32vec2*)DataSrcB);
				break;
			case RGB32F:
				*((glm::f32vec3*)DataDst) = *((glm::f32vec3*)DataSrcA) + *((glm::f32vec3*)DataSrcB);
				break;
			case RGBA32F:
				*((glm::f32vec4*)DataDst) = *((glm::f32vec4*)DataSrcA) + *((glm::f32vec4*)DataSrcB);
				break;
			default:
				assert(0);
			}
		}

		void add
		(
			texture2D::image & Result,
			texture2D::image const & ImageA,
			texture2D::image const & ImageB,
		)
		{

		}

	}//namespace detail

	texture2D operator+
	(
		texture2D const & ImageA, 
		texture2D const & ImageB
	)
	{
		assert(ImageA.levels() == ImageB.levels());
		texture2D Result[ImageA.levels()];

		for(texture2D::level_type Level = 0; Level < Result.levels(); ++Level)
		{
			assert(ImageA.capacity() == ImageB.capacity());
			assert(ImageA.format() == ImageB.format());

			Result[Level] = texture2D::image(ImageA[Level].dimensions(), ImageA[Level].format());

			add(Result[Level], ImageA[Level], ImageB[Level]);

			texture2D::size_type ValueSize = Result.value_size();
			texture2D::size_type TexelCount = this->capacity() / ValueSize;
			for(texture2D::size_type Texel = 0; Texel < TexelCount; ++Texel)
			{
				texture2D::value_type * DataDst = Result[Level].data() + Texel * ValueSize;
				texture2D::value_type const * const DataSrcA = ImageA[Level].data() + Texel * ValueSize;
				texture2D::value_type const * const DataSrcB = ImageB[Level].data() + Texel * ValueSize;

				detail::op(DataDst, DataSrcA, DataSrcB, Result.format(), std::plus);
			}
		}

		return Result;
	}

	texture2D operator-
	(
		texture2D const & ImageA, 
		texture2D const & ImageB
	)
	{
		assert(ImageA.levels() == ImageB.levels());
		texture2D Result[ImageA.levels()];

		
		for(texture2D::level_type Level = 0; Level < ImageA.levels(); ++Level)
		{
			assert(ImageA.capacity() == ImageB.capacity());

			
		}

		return Result;
	}

}//namespace gli
