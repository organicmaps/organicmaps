#pragma once

#include "graphics/opengl/opengl.hpp"
#include "graphics/color.hpp"
#include <boost/gil/gil_all.hpp>
#include <boost/mpl/vector_c.hpp>

namespace gil = boost::gil;
namespace mpl = boost::mpl;

namespace graphics
{
  template <unsigned Denom>
  struct DownsampleImpl
  {
    template <typename Ch1, typename Ch2>
    void operator()(Ch1 const & ch1, Ch2 & ch2) const
    {
      ch2 = ch1 / Denom;
    }
  };

  template <unsigned FromBig, unsigned ToSmall>
  struct Downsample
  {
    static const int Denom = 1 << (FromBig - ToSmall);

    template <typename SrcP, typename DstP>
    void operator()(SrcP const & src, DstP & dst) const
    {
      static_for_each(src, dst, DownsampleImpl<Denom>());
    }
  };

  struct RGBA8Traits
  {
    typedef gil::rgba8_pixel_t pixel_t;
    typedef gil::rgba8c_pixel_t const_pixel_t;
    typedef gil::rgba8_view_t view_t;
    typedef gil::rgba8c_view_t const_view_t;
    typedef gil::rgba8_image_t image_t;

    static const int maxChannelVal = 255;
    static const int channelScaleFactor = 1;

    static const int gl_pixel_data_type = GL_UNSIGNED_BYTE;
    static const int gl_pixel_format_type = GL_RGBA;

    typedef Downsample<8, 8> color_converter;

    static pixel_t const createPixel(graphics::Color const & c)
    {
      return pixel_t((unsigned char)(c.r / 255.0f) * maxChannelVal,
                     (unsigned char)(c.g / 255.0f) * maxChannelVal,
                     (unsigned char)(c.b / 255.0f) * maxChannelVal,
                     (unsigned char)(c.a / 255.0f) * maxChannelVal);
    }
  };

  struct RGB565Traits
  {
    typedef gil::packed_pixel_type<
        unsigned short,
        mpl::vector3_c<unsigned, 5, 6, 5>,
        gil::bgr_layout_t
    >::type pixel_t;

    typedef gil::memory_based_step_iterator<pixel_t*> iterator_t;
    typedef gil::memory_based_2d_locator<iterator_t> locator_t;
    typedef gil::image_view<locator_t> view_t;

    typedef pixel_t const const_pixel_t;

    typedef gil::memory_based_step_iterator<pixel_t const *> const_iterator_t;
    typedef gil::memory_based_2d_locator<const_iterator_t> const_locator_t;
    typedef gil::image_view<const_locator_t> const_view_t;

    typedef gil::image<pixel_t, false> image_t;

    static const int maxChannelVal = 32;
    static const int channelScaleFactor = 8;

    static const int gl_pixel_data_type = GL_UNSIGNED_SHORT_5_6_5;
    static const int gl_pixel_format_type = GL_RGB;

    typedef Downsample<8, 5> color_converter;

    static pixel_t const createPixel(graphics::Color const & c)
    {
      return pixel_t((int)(c.r / 255.0f) * maxChannelVal,
                     (int)(c.g / 255.0f) * maxChannelVal, //< fix this channel
                     (int)(c.b / 255.0f) * maxChannelVal);
    }
  };

  struct RGBA4Traits
  {
    typedef gil::packed_pixel_type<
        unsigned short,
        mpl::vector4_c<unsigned, 4, 4, 4, 4>,
        gil::abgr_layout_t
    >::type pixel_t;

    typedef gil::memory_based_step_iterator<pixel_t*> iterator_t;
    typedef gil::memory_based_2d_locator<iterator_t> locator_t;
    typedef gil::image_view<locator_t> view_t;

    typedef pixel_t const const_pixel_t;

    typedef gil::memory_based_step_iterator<pixel_t const *> const_iterator_t;
    typedef gil::memory_based_2d_locator<const_iterator_t> const_locator_t;
    typedef gil::image_view<const_locator_t> const_view_t;

    typedef gil::image<pixel_t, false> image_t;

    static const int maxChannelVal = 15;
    static const int channelScaleFactor = 16;

    static const int gl_pixel_data_type = GL_UNSIGNED_SHORT_4_4_4_4;
    static const int gl_pixel_format_type = GL_RGBA;

    typedef Downsample<8, 4> color_converter;

    static pixel_t const createPixel(graphics::Color const & c)
    {
      return pixel_t((int)(c.r / 255.0f) * maxChannelVal,
                     (int)(c.g / 255.0f) * maxChannelVal,
                     (int)(c.b / 255.0f) * maxChannelVal,
                     (int)(c.a / 255.0f) * maxChannelVal);
    }
  };
}

#ifdef OMIM_GL_ES
  #define DATA_TRAITS graphics::RGBA4Traits
  #define RT_TRAITS graphics::RGBA4Traits
#else
  #define DATA_TRAITS graphics::RGBA8Traits
  #define RT_TRAITS graphics::RGBA8Traits
#endif
