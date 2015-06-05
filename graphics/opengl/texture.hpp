#pragma once

#include "graphics/opengl/managed_texture.hpp"
#include "graphics/opengl/data_traits.hpp"

#include "graphics/image.hpp"

#include "indexer/map_style_reader.hpp"

#include "coding/lodepng_io.hpp"

namespace gil = boost::gil;
namespace mpl = boost::mpl;

namespace graphics
{
  namespace gl
  {

    template <typename Traits, bool IsBacked>
    class Texture{};


    template <typename Traits>
    class Texture<Traits, false> : public BaseTexture
    {
    public:

      typedef Traits traits_t;
      typedef typename Traits::pixel_t pixel_t;
      typedef typename Traits::const_pixel_t const_pixel_t;
      typedef typename Traits::view_t view_t;
      typedef typename Traits::const_view_t const_view_t;
      typedef typename Traits::image_t image_t;

      static const int maxChannelVal = Traits::maxChannelVal;
      static const int channelScaleFactor = Traits::channelScaleFactor;
      static const int gl_pixel_data_type = Traits::gl_pixel_data_type;
      static const int gl_pixel_format_type = Traits::gl_pixel_format_type;

    private:

      void upload(void * data)
      {
        makeCurrent();

        OGLCHECK(glTexImage2D(
          GL_TEXTURE_2D,
          0,
          Traits::gl_pixel_format_type,
          width(),
          height(),
          0,
          Traits::gl_pixel_format_type,
          Traits::gl_pixel_data_type,
          data));

        /// In multithreaded resource usage scenarios the suggested way to see
        /// resource update made in one thread to the another thread is
        /// to call the glFlush in thread, which modifies resource and then rebind
        /// resource in another threads that is using this resource, if any.
        OGLCHECK(glFlushFn());

      }

    public:

      Texture(string const & fileName, EDensity density) : BaseTexture(GetDimensions(fileName, density))
      {
        typename Traits::image_t image(width(), height());
        ReaderPtr<Reader> reader = GetStyleReader().GetResourceReader(fileName, density);
        gil::lodepng_read_and_convert_image(reader, image, typename Traits::color_converter());
        upload(&gil::view(image)(0, 0));
      }

      Texture(unsigned width, unsigned height)
        : BaseTexture(width, height)
      {
        upload(0);
      }

      void fill(graphics::Color const & c)
      {
        makeCurrent();

        typename Traits::image_t image(width(), height());

        typename Traits::pixel_t val = Traits::createPixel(c);

        typename Traits::view_t v = gil::view(image);

        for (size_t y = 0; y < height(); ++y)
          for (size_t x = 0; x < width(); ++x)
            v(x, y) = val;

        upload(&v(0, 0));
      }

      void dump(char const * fileName)
      {
        makeCurrent();
//        std::string const fullPath = GetPlatform().WritablePathForFile(fileName);

        typename Traits::image_t image(width(), height());

#ifndef OMIM_GL_ES
      OGLCHECK(glGetTexImage(
          GL_TEXTURE_2D,
          0,
          Traits::gl_pixel_format_type,
          Traits::gl_pixel_data_type,
          &gil::view(image)(0, 0)));
//      boost::gil::lodepng_write_view(fullPath.c_str(), gil::view(image));
#endif


      }
    };

    template <typename Traits>
    class Texture<Traits, true> : public ManagedTexture
    {
    public:

      typedef Traits traits_t;
      typedef typename Traits::pixel_t pixel_t;
      typedef typename Traits::const_pixel_t const_pixel_t;
      typedef typename Traits::view_t view_t;
      typedef typename Traits::const_view_t const_view_t;
      typedef typename Traits::image_t image_t;

      static const int maxChannelVal = Traits::maxChannelVal;
      static const int channelScaleFactor = Traits::channelScaleFactor;
      static const int gl_pixel_data_type = Traits::gl_pixel_data_type;
      static const int gl_pixel_format_type = Traits::gl_pixel_format_type;

    private:

      void updateDirty(m2::RectU const & r);

    public:

      void upload(void * data);
      void upload(void * data, m2::RectU const & r);

      /// Create the texture loading it from file
      Texture(string const & fileName, EDensity density);
      /// Create the texture with the predefined dimensions
      Texture(size_t width, size_t height);
      Texture(m2::RectU const & r);

      /// You can call this anytime, regardless the locking status of the texture.
      /// const_view_t const_view() const;
      /// You can call this on locked texture only. All your changess to this view's data will be
      /// uploaded to the video memory on unlock()
      view_t view(size_t width, size_t height);

      void fill(graphics::Color const & c);

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

    typedef Texture<RGB565Traits, false> RawRGB565Texture;

    template <typename Traits>
    Texture<Traits, true>::Texture(const m2::RectU &r)
      : ManagedTexture(r.SizeX(), r.SizeY(), sizeof(pixel_t))
    {
      upload(0);
    }

    template <typename Traits>
    Texture<Traits, true>::Texture(size_t width, size_t height)
      : ManagedTexture(width, height, sizeof(pixel_t))
    {
      upload(0);
    }

    template <typename Traits>
    Texture<Traits, true>::Texture(string const & fileName, EDensity density)
      : ManagedTexture(GetDimensions(fileName, density), sizeof(pixel_t))
    {
      lock();
      view_t v = view(width(), height());
      ReaderPtr<Reader> reader = GetStyleReader().GetResourceReader(fileName, density);
      gil::lodepng_read_and_convert_view(reader, v, typename Traits::color_converter());
      upload(&v(0, 0));
      unlock();
    }

    template <typename Traits>
    typename Texture<Traits, true>::view_t Texture<Traits, true>::view(size_t w, size_t h)
    {
      ASSERT(m_isLocked, ("non const access to unlocked texture!"));
      return gil::interleaved_view(
          w,
          h,
          (pixel_t*)auxData(),
          w * sizeof(pixel_t));
    }

/*    template <typename Traits>
    typename Texture<Traits, true>::const_view_t Texture<Traits, true>::const_view() const
    {
      return gil::const_view(m_image);
    }
 */

    template <typename Traits>
    void Texture<Traits, true>::upload(void * data)
    {
      makeCurrent();

      OGLCHECK(glTexImage2D(
         GL_TEXTURE_2D,
         0,
         gl_pixel_format_type,
         width(),
         height(),
         0,
         gl_pixel_format_type,
         gl_pixel_data_type,
         data));

      /// In multithreaded resource usage scenarios the suggested way to see
      /// resource update made in one thread to the another thread is
      /// to call the glFlush in thread, which modifies resource and then rebind
      /// resource in another threads that is using this resource, if any.
      OGLCHECK(glFlushFn());
    }

    template <typename Traits>
    void Texture<Traits, true>::upload(void * data, m2::RectU const & r)
    {
      makeCurrent();
      /// Uploading texture data
      OGLCHECK(glTexSubImage2D(
         GL_TEXTURE_2D,
         0,
         r.minX(),
         r.minY(),
         r.SizeX(),
         r.SizeY(),
         gl_pixel_format_type,
         gl_pixel_data_type,
         data));
    }

    template <typename Traits>
    void Texture<Traits, true>::randomize()
    {
      makeCurrent();
      lock();
      view_t v = view(width(), height());

      for (size_t y = 0; y < height(); ++y)
        for (size_t x = 0; x < width(); ++x)
          v(x, y) = pixel_t(rand() % maxChannelVal, rand() % maxChannelVal, rand() % maxChannelVal, maxChannelVal);

      upload(&v(0, 0), m2::RectU(0, 0, width(), height()));
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
          gl_pixel_format_type,
          Traits::gl_pixel_data_type,
          &view(width(), height())(0, 0)));
#else
          ASSERT(false, ("no glGetTexImage function in OpenGL ES"));
#endif
    }

    template <typename Traits>
    void Texture<Traits, true>::dump(char const * fileName)
    {
      lock();
      readback();
//      std::string const fullPath = GetPlatform().WritablePathForFile(fileName);
#ifndef OMIM_GL_ES
//      boost::gil::lodepng_write_view(fullPath.c_str(), view(width(), height()));
#endif
      unlock();
    }

    template <typename Traits>
    void Texture<Traits, true>::fill(graphics::Color const & /*c*/)
    {
/*      makeCurrent();
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
 */
    }
  }
}
