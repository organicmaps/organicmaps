#pragma once

#include "internal/opengl.hpp"
#include "color.hpp"
#include "managed_texture.hpp"
#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"
#include "../std/vector.hpp"
#include "../3party/boost/boost/gil/gil_all.hpp"
#include "../3party/boost/boost/mpl/vector_c.hpp"
#include "../platform/platform.hpp"
#include "../coding/lodepng_io.hpp"
#include "../std/iostream.hpp"

namespace gil = boost::gil;
namespace mpl = boost::mpl;


namespace yg
{
  namespace gl
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

      static const int gl_pixel_data_type = GL_UNSIGNED_BYTE;

      typedef Downsample<8, 8> color_converter;
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

      static const int gl_pixel_data_type = GL_UNSIGNED_SHORT_4_4_4_4;

      typedef Downsample<8, 4> color_converter;
    };

    template <typename Traits, bool IsBacked>
    class Texture{};

    inline m2::PointU const GetDimensions(string const & fileName)
    {
      gil::point2<ptrdiff_t> size = gil::lodepng_read_dimensions(GetPlatform().ResourcesDir() + fileName);
      return m2::PointU(size.x, size.y);
    }

    template <typename Traits>
    class Texture<Traits, false> : public BaseTexture
    {
    private:

      void upload(void * data)
      {
        makeCurrent();

        OGLCHECK(glTexImage2D(
          GL_TEXTURE_2D,
          0,
          GL_RGBA,
          width(),
          height(),
          0,
          GL_RGBA,
          Traits::gl_pixel_data_type,
          data));
      }

    public:

      Texture(string const & fileName) : BaseTexture(GetDimensions(fileName))
      {
        typename Traits::image_t image(width(), height());
        gil::lodepng_read_and_convert_image((GetPlatform().ResourcesDir() + fileName).c_str(), image, typename Traits::color_converter());
        upload(&gil::view(image)(0, 0));
      }

      Texture(unsigned width, unsigned height)
        : BaseTexture(width, height)
      {
        upload(0);
      }

      void dump(char const *){}
    };

    template <typename Traits>
    class Texture<Traits, true> : public ManagedTexture
    {
    public:

      typedef typename Traits::pixel_t pixel_t;
      typedef typename Traits::const_pixel_t const_pixel_t;
      typedef typename Traits::view_t view_t;
      typedef typename Traits::const_view_t const_view_t;
      typedef typename Traits::image_t image_t;

      static const int maxChannelVal = Traits::maxChannelVal;
      static const int gl_pixel_data_type = Traits::gl_pixel_data_type;

    private:

      /// system memory copy of texture data
      image_t m_image;
      /// system memory buffer for the purpose of correct working of glTexSubImage2D.
      /// in OpenGL ES 1.1 there are no way to specify GL_UNPACK_ROW_LENGTH so
      /// the data supplied to glTexSubImage2D should be continous.
      vector<pixel_t> m_auxData;

      void upload();
      void updateDirty(m2::RectU const & r);

    public:

      /// Create the texture loading it from file
      Texture(string const & fileName);
      /// Create the texture with the predefined dimensions
      Texture(size_t width, size_t height);
      Texture(m2::RectU const & r);

      /// You can call this anytime, regardless the locking status of the texture.
      const_view_t const_view() const;
      /// You can call this on locked texture only. All your changess to this view's data will be
      /// uploaded to the video memory on unlock()
      view_t view();

      void fill(yg::Color const & c);

      /// dump the texture into the file.
      void dump(char const * fileName);
      /// videomem -> sysmem texture image transfer.
      /// @warning could be very slow. use with caution.
      void readback();

      void randomize();
    };

    typedef Texture<RGBA8Traits, true> RGBA8Texture;
    typedef Texture<RGBA4Traits, true> RGBA4Texture;

    typedef Texture<RGBA8Traits, false> RawRGBA8Texture;
    typedef Texture<RGBA4Traits, false> RawRGBA4Texture;

    template <typename Traits>
    Texture<Traits, true>::Texture(const m2::RectU &r)
      : ManagedTexture(r.SizeX(), r.SizeY()), m_image(r.SizeX(), r.SizeY())
    {
      upload();
      m_auxData.resize(width * height);
    }

    template <typename Traits>
    Texture<Traits, true>::Texture(size_t width, size_t height)
      : ManagedTexture(width, height), m_image(width, height)
    {
      upload();
      m_auxData.resize(width * height);
    }

    template <typename Traits>
    Texture<Traits, true>::Texture(string const & fileName) : ManagedTexture(GetDimensions(fileName))
    {
      lock();
      gil::lodepng_read_and_convert_image(GetPlatform().ReadPathForFile(fileName).c_str(), m_image, typename Traits::color_converter());
      unlock();
      upload();
      m_image.recreate(0, 0);
    }

    template <typename Traits>
    typename Texture<Traits, true>::view_t Texture<Traits, true>::view()
    {
      ASSERT(m_isLocked, ("non const access to unlocked texture!"));
      return gil::view(m_image);
    }

    template <typename Traits>
    typename Texture<Traits, true>::const_view_t Texture<Traits, true>::const_view() const
    {
      return gil::const_view(m_image);
    }

    template <typename Traits>
    void Texture<Traits, true>::upload()
    {
      makeCurrent();

      OGLCHECK(glTexImage2D(
         GL_TEXTURE_2D,
         0,
         GL_RGBA,
         width(),
         height(),
         0,
         GL_RGBA,
         Traits::gl_pixel_data_type,
         &gil::const_view(m_image)(0, 0)));
    }


    template <typename Traits>
    void Texture<Traits, true>::updateDirty(m2::RectU const & r)
    {
      makeCurrent();

//      m_auxData.resize(r.SizeX() * r.SizeY());

      view_t auxView = gil::interleaved_view(
          r.SizeX(), r.SizeY(),
          (pixel_t*)&m_auxData[0],
          r.SizeX() * sizeof(pixel_t));

      gil::copy_pixels(
          gil::subimage_view(gil::const_view(m_image),
                             r.minX(), r.minY(),
                             r.SizeX(), r.SizeY()),
          auxView);

      OGLCHECK(glTexSubImage2D(
          GL_TEXTURE_2D,
          0,
          r.minX(),
          r.minY(),
          r.SizeX(),
          r.SizeY(),
          GL_RGBA,
          Traits::gl_pixel_data_type,
          &m_auxData[0]
      ));
    }

    template <typename Traits>
    void Texture<Traits, true>::randomize()
    {
      makeCurrent();
      lock();
      view_t v = view();

      for (size_t y = 0; y < height(); ++y)
        for (size_t x = 0; x < width(); ++x)
          v(x, y) = pixel_t(rand() % maxChannelVal, rand() % maxChannelVal, rand() % maxChannelVal, maxChannelVal);

      addDirtyRect(m2::RectU(0, 0, width(), height()));

      unlock();
    }

    template <typename Traits>
    void Texture<Traits, true>::readback()
    {
      makeCurrent();
#ifndef OMIM_GL_ES
      OGLCHECK(glGetTexImage(
          GL_TEXTURE_2D,
          0,
          GL_RGBA,
          Traits::gl_pixel_data_type,
          &gil::view(m_image)(0, 0)));
#else
          ASSERT(false, ("no glGetTexImage function in OpenGL ES"));
#endif
    }

    template <typename Traits>
    void Texture<Traits, true>::dump(char const * fileName)
    {
      readback();
      std::string const fullPath = GetPlatform().WritablePathForFile(fileName);
#ifndef OMIM_GL_ES
      boost::gil::lodepng_write_view(fullPath, gil::const_view(m_image));
#endif
    }

    template <typename Traits>
    void Texture<Traits, true>::fill(yg::Color const & c)
    {
      makeCurrent();
      lock();
      view_t v = view();

      pixel_t val((c.r / 255.0f) * maxChannelVal,
                  (c.g / 255.0f) * maxChannelVal,
                  (c.b / 255.0f) * maxChannelVal,
                  (c.a / 255.0f) * maxChannelVal);

      for (size_t y = 0; y < height(); ++y)
        for (size_t x = 0; x < width(); ++x)
          v(x, y) = val;

      addDirtyRect(m2::RectU(0, 0, width(), height()));
      unlock();
    }
  }
}
