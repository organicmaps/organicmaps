/*
    Copyright 2005-2007 Adobe Systems Incorporated

    Use, modification and distribution are subject to the Boost Software License,
    Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).

    See http://stlab.adobe.com/gil for most recent version including documentation.
*/
/*************************************************************************************************/

#ifndef GIL_LODEPNG_IO_PRIVATE_H
#define GIL_LODEPNG_IO_PRIVATE_H

/// \file
/// \brief  Internal support for reading and writing PNG files
/// \author Hailin Jin and Lubomir Bourdev \n
///         Adobe Systems Incorporated
/// \date   2005-2007 \n Last updated August 14, 2007

#include <algorithm>
#include <vector>
#include <boost/static_assert.hpp>
#include <boost/gil/gil_all.hpp>
#include <boost/gil/extension/io/io_error.hpp>
#include "lodepng.hpp"

namespace boost { namespace gil {

namespace detail {

static const std::size_t LODEPNG_BYTES_TO_CHECK = 4;

static const int LODEPNG_COLOR_MASK_PALETTE = 1;
static const int LODEPNG_COLOR_MASK_COLOR = 2;
static const int LODEPNG_COLOR_MASK_ALPHA = 4;
static const int LODEPNG_COLOR_TYPE_GRAY = 0;
static const int LODEPNG_COLOR_TYPE_PALETTE = (LODEPNG_COLOR_MASK_COLOR | LODEPNG_COLOR_MASK_PALETTE);
static const int LODEPNG_COLOR_TYPE_RGB = (LODEPNG_COLOR_MASK_COLOR);
static const int LODEPNG_COLOR_TYPE_RGB_ALPHA = (LODEPNG_COLOR_MASK_COLOR | LODEPNG_COLOR_MASK_ALPHA);
static const int LODEPNG_COLOR_TYPE_GRAY_ALPHA = (LODEPNG_COLOR_MASK_ALPHA);
static const int LODEPNG_COLOR_TYPE_RGBA = LODEPNG_COLOR_TYPE_RGB_ALPHA;
static const int LODEPNG_COLOR_TYPE_GA = LODEPNG_COLOR_TYPE_GRAY_ALPHA;


// lbourdev: These can be greatly simplified, for example:
template <typename Cs> struct lodepng_color_type {BOOST_STATIC_CONSTANT(int,color_type=0);};
template<> struct lodepng_color_type<gray_t> { BOOST_STATIC_CONSTANT(int,color_type=LODEPNG_COLOR_TYPE_GRAY); };
template<> struct lodepng_color_type<rgb_t>  { BOOST_STATIC_CONSTANT(int,color_type=LODEPNG_COLOR_TYPE_RGB); };
template<> struct lodepng_color_type<rgba_t> { BOOST_STATIC_CONSTANT(int,color_type=LODEPNG_COLOR_TYPE_RGBA); };

template <typename Channel,typename ColorSpace> struct lodepng_is_supported {BOOST_STATIC_CONSTANT(bool,value=false);};
template <> struct lodepng_is_supported<bits8,gray_t>  {BOOST_STATIC_CONSTANT(bool,value=true);};
template <> struct lodepng_is_supported<bits8,rgb_t>   {BOOST_STATIC_CONSTANT(bool,value=true);};
template <> struct lodepng_is_supported<bits8,rgba_t>  {BOOST_STATIC_CONSTANT(bool,value=true);};
template <> struct lodepng_is_supported<bits16,gray_t> {BOOST_STATIC_CONSTANT(bool,value=true);};
template <> struct lodepng_is_supported<bits16,rgb_t>  {BOOST_STATIC_CONSTANT(bool,value=true);};
template <> struct lodepng_is_supported<bits16,rgba_t> {BOOST_STATIC_CONSTANT(bool,value=true);};

template <typename Channel> struct lodepng_bit_depth {BOOST_STATIC_CONSTANT(int,bit_depth=sizeof(Channel)*8);};

template <typename Channel,typename ColorSpace>
struct lodepng_read_support_private {
    BOOST_STATIC_CONSTANT(bool,is_supported=false);
    BOOST_STATIC_CONSTANT(int,bit_depth=0);
    BOOST_STATIC_CONSTANT(int,color_type=0);
};
template <>
struct lodepng_read_support_private<bits8,gray_t> {
    BOOST_STATIC_CONSTANT(bool,is_supported=true);
    BOOST_STATIC_CONSTANT(int,bit_depth=8);
    BOOST_STATIC_CONSTANT(int,color_type=LODEPNG_COLOR_TYPE_GRAY);
};
template <>
struct lodepng_read_support_private<bits8,rgb_t> {
    BOOST_STATIC_CONSTANT(bool,is_supported=true);
    BOOST_STATIC_CONSTANT(int,bit_depth=8);
    BOOST_STATIC_CONSTANT(int,color_type=LODEPNG_COLOR_TYPE_RGB);
};
template <>
struct lodepng_read_support_private<bits8,rgba_t> {
    BOOST_STATIC_CONSTANT(bool,is_supported=true);
    BOOST_STATIC_CONSTANT(int,bit_depth=8);
    BOOST_STATIC_CONSTANT(int,color_type=LODEPNG_COLOR_TYPE_RGBA);
};
template <>
struct lodepng_read_support_private<bits16,gray_t> {
    BOOST_STATIC_CONSTANT(bool,is_supported=true);
    BOOST_STATIC_CONSTANT(int,bit_depth=16);
    BOOST_STATIC_CONSTANT(int,color_type=LODEPNG_COLOR_TYPE_GRAY);
};
template <>
struct lodepng_read_support_private<bits16,rgb_t> {
    BOOST_STATIC_CONSTANT(bool,is_supported=true);
    BOOST_STATIC_CONSTANT(int,bit_depth=16);
    BOOST_STATIC_CONSTANT(int,color_type=LODEPNG_COLOR_TYPE_RGB);
};
template <>
struct lodepng_read_support_private<bits16,rgba_t> {
    BOOST_STATIC_CONSTANT(bool,is_supported=true);
    BOOST_STATIC_CONSTANT(int,bit_depth=16);
    BOOST_STATIC_CONSTANT(int,color_type=LODEPNG_COLOR_TYPE_RGBA);
};

template <typename Channel,typename ColorSpace>
struct lodepng_write_support_private {
    BOOST_STATIC_CONSTANT(bool,is_supported=false);
    BOOST_STATIC_CONSTANT(int,bit_depth=0);
    BOOST_STATIC_CONSTANT(int,color_type=0);
};
template <>
struct lodepng_write_support_private<bits8,gray_t> {
    BOOST_STATIC_CONSTANT(bool,is_supported=true);
    BOOST_STATIC_CONSTANT(int,bit_depth=8);
    BOOST_STATIC_CONSTANT(int,color_type=LODEPNG_COLOR_TYPE_GRAY);
};
template <>
struct lodepng_write_support_private<bits8,rgb_t> {
    BOOST_STATIC_CONSTANT(bool,is_supported=true);
    BOOST_STATIC_CONSTANT(int,bit_depth=8);
    BOOST_STATIC_CONSTANT(int,color_type=LODEPNG_COLOR_TYPE_RGB);
};
template <>
struct lodepng_write_support_private<bits8,rgba_t> {
    BOOST_STATIC_CONSTANT(bool,is_supported=true);
    BOOST_STATIC_CONSTANT(int,bit_depth=8);
    BOOST_STATIC_CONSTANT(int,color_type=LODEPNG_COLOR_TYPE_RGBA);
};
template <>
struct lodepng_write_support_private<bits16,gray_t> {
    BOOST_STATIC_CONSTANT(bool,is_supported=true);
    BOOST_STATIC_CONSTANT(int,bit_depth=16);
    BOOST_STATIC_CONSTANT(int,color_type=LODEPNG_COLOR_TYPE_GRAY);
};
template <>
struct lodepng_write_support_private<bits16,rgb_t> {
    BOOST_STATIC_CONSTANT(bool,is_supported=true);
    BOOST_STATIC_CONSTANT(int,bit_depth=16);
    BOOST_STATIC_CONSTANT(int,color_type=LODEPNG_COLOR_TYPE_RGB);
};
template <>
struct lodepng_write_support_private<bits16,rgba_t> {
    BOOST_STATIC_CONSTANT(bool,is_supported=true);
    BOOST_STATIC_CONSTANT(int,bit_depth=16);
    BOOST_STATIC_CONSTANT(int,color_type=LODEPNG_COLOR_TYPE_RGBA);
};

class lodepng_reader {
protected:

    LodePNG::Decoder m_decoder;
    ReaderPtr<Reader> & m_reader;

    int sig_cmp(unsigned char * sig, size_t start, size_t num_to_check)
    {
       unsigned char png_sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
       if (num_to_check > 8)
          num_to_check = 8;
       else if (num_to_check < 1)
          return (-1);

       if (start > 7)
          return (-1);

       if (start + num_to_check > 8)
          num_to_check = 8 - start;

       for (size_t i = 0; i < num_to_check; ++i)
       if (sig[i] != png_sig[i])
         return -1;

       return 0;
    }

    void init()
    {
      // 29 bytes are required to read: 8-bytes PNG magic, 4-bytes
      // chunk length, 4-bytes chunk type, 4-bytes width, 4-bytes
      // height, 1-byte bit depth, 1-byte color type, 1-byte
      // compression type, 1-byte filter type and 1-byte interlace
      // type.
      //
      // 4 more bytes are needed if CRC32 sum should be checked.
      size_t const kMinSize = 29;
      size_t const kMaxSize = kMinSize + 4;

      unsigned char buf[kMaxSize];
      size_t const size = m_decoder.settings.ignoreCrc ? kMinSize : kMaxSize;
      m_reader.Read(0, buf, size);
      // io_error_if(fread(buf, 1, 30, get()) != 30,
      //             "lodepng_check_validity: fail to read file");
      m_decoder.inspect(buf, size);
    }

public:

    lodepng_reader(ReaderPtr<Reader> & reader) : m_reader(reader) { init(); }

    point2<std::ptrdiff_t> get_dimensions() {
        return point2<std::ptrdiff_t>(m_decoder.getWidth(), m_decoder.getHeight());
    }

    template <typename View>
    void apply(const View& view)
    {
        io_error_if((view.width() != m_decoder.getWidth() || view.height() != m_decoder.getHeight()),
                    "lodepng_read_view: input view size does not match PNG file size");

        int bitDepth = lodepng_read_support_private<typename channel_type<View>::type,
                                                    typename color_space_type<View>::type>::bit_depth;
        if (bitDepth != m_decoder.getBpp() / m_decoder.getChannels())
           io_error("lodepng_read_view: input view type is incompatible with the image type(bitDepth mismatch)");

        int colorType = lodepng_read_support_private<typename channel_type<View>::type,
                                                     typename color_space_type<View>::type>::color_type;
        if (colorType != m_decoder.getInfoPng().color.colorType)
            io_error("lodepng_read_view: input view type is incompatible with the image type(colorType mismatch)");

        std::vector<unsigned char> inputData;
        LodePNG::loadFile(inputData, m_reader);

        std::vector<unsigned char> decodedData;
        m_decoder.decode(decodedData, inputData);

        if (m_decoder.hasError())
        {
          std::stringstream out;
          out << "lodepng_read_view: " << m_decoder.getError();
          io_error(out.str().c_str());
        }

        gil::rgba8_view_t srcView = gil::interleaved_view(
            view.width(),
            view.height(),
            gil::rgba8_ptr_t(&decodedData[0]),
            4 * view.width());

        gil::copy_pixels(srcView, view);
    }

    template <typename Image>
    void read_image(Image& im) {
        im.recreate(get_dimensions());
        apply(view(im));
    }
};

// This code will be simplified...
template <typename CC>
class lodepng_reader_color_convert : public lodepng_reader {
private:
    CC _cc;
public:
    lodepng_reader_color_convert(ReaderPtr<Reader> & reader, CC cc_in) : lodepng_reader(reader),_cc(cc_in) {}
    lodepng_reader_color_convert(ReaderPtr<Reader> & reader) : lodepng_reader(reader) {}
    template <typename View>
    void apply(const View& view)
    {
      io_error_if((static_cast<unsigned>(view.width()) != m_decoder.getWidth()
                   || static_cast<unsigned>(view.height()) != m_decoder.getHeight()),
                  "lodepng_read_view: input view size does not match PNG file size");

      std::vector<unsigned char> inputData;
      LodePNG::loadFile(inputData, m_reader);

      std::vector<unsigned char> decodedData;
      m_decoder.decode(decodedData, inputData);

      if (m_decoder.hasError())
      {
        std::stringstream out;
        out << "lodepng_read_view: " << m_decoder.getError();
        io_error(out.str().c_str());
      }

      int bitDepth = m_decoder.getBpp() / m_decoder.getChannels();
      int colorType = m_decoder.getInfoPng().color.colorType;

      switch (colorType)
      {
      case LODEPNG_COLOR_TYPE_GRAY:
        switch (bitDepth)
        {
        case 8:
          {
            gil::gray8_view_t srcView = gil::interleaved_view(
              m_decoder.getWidth(),
              m_decoder.getHeight(),
              gil::gray8_ptr_t(&decodedData[0]),
              sizeof(gil::gray8_pixel_t) * view.width()
            );

            gil::copy_and_convert_pixels(srcView, view, _cc);
            break;
          }
        case 16:
          {
            gil::gray8_view_t srcView = gil::interleaved_view(
              m_decoder.getWidth(),
              m_decoder.getHeight(),
              gil::gray8_ptr_t(&decodedData[0]),
              sizeof(gil::gray8_pixel_t) * view.width()
            );

            gil::copy_and_convert_pixels(srcView, view, _cc);
            break;
          }
        default:
            io_error("png_reader_color_convert::apply(): unknown combination of color type and bit depth");
        }
        break;
      case LODEPNG_COLOR_TYPE_RGB:
        switch (bitDepth)
        {
        case 8:
          {
            gil::rgb8_view_t srcView = gil::interleaved_view(
              m_decoder.getWidth(),
              m_decoder.getHeight(),
              gil::rgb8_ptr_t(&decodedData[0]),
              sizeof(gil::rgb8_pixel_t) * view.width()
            );
            gil::copy_and_convert_pixels(srcView, view, _cc);
            break;
          }
        case 16:
          {
            gil::rgb16_view_t srcView = gil::interleaved_view(
                m_decoder.getWidth(),
                m_decoder.getHeight(),
                gil::rgb16_ptr_t(&decodedData[0]),
                sizeof(gil::rgba16_pixel_t) * view.width()
                );
            gil::copy_and_convert_pixels(srcView, view, _cc);
            break;
          }
        default:
          io_error("png_reader_color_convert::apply(): unknown combination of color type and bit depth");
        }
        break;
      case LODEPNG_COLOR_TYPE_RGBA:
        switch (bitDepth)
        {
        case 8:
          {
            gil::rgba8_view_t srcView = gil::interleaved_view(
                m_decoder.getWidth(),
                m_decoder.getHeight(),
                gil::rgba8_ptr_t(&decodedData[0]),
                sizeof(gil::rgba8_pixel_t) * m_decoder.getWidth()
                );
            gil::copy_and_convert_pixels(srcView, view, _cc);
            break;
          }
        case 16:
          {
            gil::rgba16_view_t srcView = gil::interleaved_view(
                m_decoder.getWidth(),
                m_decoder.getHeight(),
                gil::rgba16_ptr_t(&decodedData[0]),
                sizeof(gil::rgba16_pixel_t) * m_decoder.getWidth()
                );
            gil::copy_and_convert_pixels(srcView, view, _cc);
            break;
          }
        default:
          io_error("png_reader_color_convert::apply(): unknown combination of color type and bit depth");
        }
        break;
      default:
        io_error("png_reader_color_convert::apply(): unknown color type");
      }
    }

    template <typename Image>
    void read_image(Image& im) {
        im.recreate(get_dimensions());
        apply(view(im));
    }
};


class lodepng_writer /*: public file_mgr */{
protected:

    LodePNG::Encoder m_encoder;
    WriterPtr<Writer> & m_writer;

    void init() {}

public:
    lodepng_writer(WriterPtr<Writer> & writer) : /*file_mgr(filename, "wb"), m_fileName(filename)*/ m_writer(writer) { init(); }

    template <typename View>
    void apply(const View& view)
    {
      typedef lodepng_write_support_private<
          typename channel_type<View>::type,
          typename color_space_type<View>::type
      > write_support_t;

      /// Create temporary image with base color_space_type.

      LodePNG_InfoRaw infoRaw = m_encoder.getInfoRaw();
      LodePNG_InfoPng infoPng = m_encoder.getInfoPng();

      infoRaw.color.bitDepth = write_support_t::bit_depth;
      infoRaw.color.colorType = write_support_t::color_type;

      m_encoder.setInfoRaw(infoRaw);

      infoPng.compressionMethod = 0;
      infoPng.filterMethod = 0;
      infoPng.interlaceMethod = 0;
      infoPng.height = view.height();
      infoPng.width = view.width();
      infoPng.color.bitDepth = write_support_t::bit_depth;
      infoPng.color.colorType = write_support_t::color_type;

      m_encoder.setInfoPng(infoPng);

      std::vector<unsigned char> buffer;

      /// in original code there was some tricks with byte indiannes.
      m_encoder.encode(
          buffer,
          (unsigned char *)&view(0, 0),
          view.width(),
          view.height()
      );

      LodePNG::saveFile(buffer, m_writer);
   }
};

} // namespace detail
} }  // namespace boost::gil

#endif
