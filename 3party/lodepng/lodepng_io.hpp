/*
    Copyright 2005-2007 Adobe Systems Incorporated

    Use, modification and distribution are subject to the Boost Software License,
    Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).

    See http://opensource.adobe.com/gil for most recent version including documentation.
*/

/*************************************************************************************************/

#ifndef GIL_LODEPNG_IO_H
#define GIL_LODEPNG_IO_H

/// \file
/// \brief  Support for reading and writing PNG files
///         using LodePNG
//
// We are currently providing the following functions:
// point2<std::ptrdiff_t>    lodepng_read_dimensions(const char*)
// template <typename View>  void lodepng_read_view(const char*,const View&)
// template <typename View>  void lodepng_read_image(const char*,image<View>&)
// template <typename View>  void lodepng_write_view(const char*,const View&)
// template <typename View>  struct lodepng_read_support;
// template <typename View>  struct lodepng_write_support;
//
/// \author Rachytski Siarhei
/// \date   2010 \n Last updated June 20, 2010

#include <stdio.h>
#include <string>
#include <boost/static_assert.hpp>
#include <boost/gil/gil_config.hpp>
#include <boost/gil/utilities.hpp>
#include <boost/gil/extension/io/io_error.hpp>
#include "lodepng_io_private.hpp"

namespace boost { namespace gil {

/// \ingroup LODEPNG_IO
/// \brief Returns the width and height of the PNG file at the specified location.
/// Throws std::ios_base::failure if the location does not correspond to a valid PNG file
inline point2<std::ptrdiff_t> lodepng_read_dimensions(ReaderPtr<Reader> & reader) {
    detail::lodepng_reader m(reader);
    return m.get_dimensions();
}

/// \ingroup LODEPNG_IO
/// \brief Returns the width and height of the PNG file at the specified location.
/// Throws std::ios_base::failure if the location does not correspond to a valid PNG file
//inline point2<std::ptrdiff_t> lodepng_read_dimensions(ReaderPtr<Reader> & reader) {
//    return lodepng_read_dimensions(reader);
//}

/// \ingroup LODEPNG_IO
/// \brief Determines whether the given view type is supported for reading
template <typename View>
struct lodepng_read_support {
    BOOST_STATIC_CONSTANT(bool,is_supported=
                          (detail::lodepng_read_support_private<typename channel_type<View>::type,
                                                            typename color_space_type<View>::type>::is_supported));
    BOOST_STATIC_CONSTANT(int,bit_depth=
                          (detail::lodepng_read_support_private<typename channel_type<View>::type,
                                                            typename color_space_type<View>::type>::bit_depth));
    BOOST_STATIC_CONSTANT(int,color_type=
                          (detail::lodepng_read_support_private<typename channel_type<View>::type,
                                                            typename color_space_type<View>::type>::color_type));
    BOOST_STATIC_CONSTANT(bool, value=is_supported);
};

/// \ingroup LODEPNG_IO
/// \brief Loads the image specified by the given png image file name into the given view.
/// Triggers a compile assert if the view color space and channel depth are not supported by the PNG library or by the I/O extension.
/// Throws std::ios_base::failure if the file is not a valid PNG file, or if its color space or channel depth are not
/// compatible with the ones specified by View, or if its dimensions don't match the ones of the view.
template <typename View>
inline void lodepng_read_view(ReaderPtr<Reader> & reader, const View& view) {
    BOOST_STATIC_ASSERT(lodepng_read_support<View>::is_supported);
    detail::lodepng_reader m(reader);
    m.apply(view);
}

/// \ingroup LODEPNG_IO
/// \brief Loads the image specified by the given png image file name into the given view.
template <typename View>
inline void lodepng_read_view(const std::string& filename,const View& view) {
    lodepng_read_view(filename.c_str(),view);
}

/// \ingroup LODEPNG_IO
/// \brief Allocates a new image whose dimensions are determined by the given png image file, and loads the pixels into it.
/// Triggers a compile assert if the image color space or channel depth are not supported by the PNG library or by the I/O extension.
/// Throws std::ios_base::failure if the file is not a valid PNG file, or if its color space or channel depth are not
/// compatible with the ones specified by Image
template <typename Image>
inline void lodepng_read_image(ReaderPtr<Reader> & reader, Image& im) {
    BOOST_STATIC_ASSERT(lodepng_read_support<typename Image::view_t>::is_supported);
    detail::lodepng_reader m(reader);
    m.read_image(im);
}

/// \ingroup LODEPNG_IO
/// \brief Allocates a new image whose dimensions are determined by the given png image file, and loads the pixels into it.
template <typename Image>
inline void lodepng_read_image(const std::string& filename,Image& im) {
    lodepng_read_image(filename.c_str(),im);
}

/// \ingroup LODEPNG_IO
/// \brief Loads the image specified by the given png image file name and color-converts it into the given view.
/// Throws std::ios_base::failure if the file is not a valid PNG file, or if its dimensions don't match the ones of the view.
template <typename View,typename CC>
inline void lodepng_read_and_convert_view(const char* filename,const View& view,CC cc) {
    detail::lodepng_reader_color_convert<CC> m(filename,cc);
    m.apply(view);
}

/// \ingroup LODEPNG_IO
/// \brief Loads the image specified by the given png image file name and color-converts it into the given view.
/// Throws std::ios_base::failure if the file is not a valid PNG file, or if its dimensions don't match the ones of the view.
template <typename View>
inline void lodepng_read_and_convert_view(ReaderPtr<Reader> & reader,const View& view) {
    detail::lodepng_reader_color_convert<default_color_converter> m(reader,default_color_converter());
    m.apply(view);
}

/// \ingroup LODEPNG_IO
/// \brief Loads the image specified by the given png image file name and color-converts it into the given view.
template <typename View,typename CC>
inline void lodepng_read_and_convert_view(ReaderPtr<Reader> & reader,const View& view,CC cc) {
    detail::lodepng_reader_color_convert<CC> m(reader, cc);
    m.apply(view);
}

/// \ingroup LODEPNG_IO
/// \brief Loads the image specified by the given png image file name and color-converts it into the given view.
//template <typename View>
//inline void lodepng_read_and_convert_view(ReaderPtr<Reader> & reader,const View& view) {
//    lodepng_read_and_convert_view(reader,view);
//}

/// \ingroup LODEPNG_IO
/// \brief Allocates a new image whose dimensions are determined by the given png image file, loads and color-converts the pixels into it.
/// Throws std::ios_base::failure if the file is not a valid PNG file
template <typename Image,typename CC>
inline void lodepng_read_and_convert_image(ReaderPtr<Reader> & reader,Image& im,CC cc) {
    detail::lodepng_reader_color_convert<CC> m(reader,cc);
    m.read_image(im);
}

/// \ingroup LODEPNG_IO
/// \brief Allocates a new image whose dimensions are determined by the given png image file, loads and color-converts the pixels into it.
/// Throws std::ios_base::failure if the file is not a valid PNG file
template <typename Image>
inline void lodepng_read_and_convert_image(ReaderPtr<Reader> & reader, Image& im) {
    detail::lodepng_reader_color_convert<default_color_converter> m(reader, default_color_converter());
    m.read_image(im);
}

/// \ingroup LODEPNG_IO
/// \brief Allocates a new image whose dimensions are determined by the given png image file, loads and color-converts the pixels into it.
template <typename Image,typename CC>
inline void lodepng_read_and_convert_image(const std::string& filename,Image& im,CC cc) {
    lodepng_read_and_convert_image(filename.c_str(),im,cc);
}

/// \ingroup LODEPNG_IO
/// \brief Allocates a new image whose dimensions are determined by the given png image file, loads and color-converts the pixels into it.
template <typename Image>
inline void lodepng_read_and_convert_image(const std::string& filename,Image& im) {
    lodepng_read_and_convert_image(filename.c_str(),im);
}

/// \ingroup LODEPNG_IO
/// \brief Determines whether the given view type is supported for writing
template <typename View>
struct lodepng_write_support {
    BOOST_STATIC_CONSTANT(bool,is_supported=
                          (detail::lodepng_write_support_private<typename channel_type<View>::type,
                                                             typename color_space_type<View>::type>::is_supported));
    BOOST_STATIC_CONSTANT(int,bit_depth=
                          (detail::lodepng_write_support_private<typename channel_type<View>::type,
                                                             typename color_space_type<View>::type>::bit_depth));
    BOOST_STATIC_CONSTANT(int,color_type=
                          (detail::lodepng_write_support_private<typename channel_type<View>::type,
                                                             typename color_space_type<View>::type>::color_type));
    BOOST_STATIC_CONSTANT(bool, value=is_supported);
};

/// \ingroup LODEPNG_IO
/// \brief Saves the view to a png file specified by the given png image file name.
/// Triggers a compile assert if the view color space and channel depth are not supported by the PNG library or by the I/O extension.
/// Throws std::ios_base::failure if it fails to create the file.
template <typename View>
inline void lodepng_write_view(WriterPtr<Writer> & writer,const View& view) {
    BOOST_STATIC_ASSERT(lodepng_write_support<View>::is_supported);
    detail::lodepng_writer m(writer);
    m.apply(view);
}

/// \ingroup LODEPNG_IO
/// \brief Saves the view to a png file specified by the given png image file name.
/*template <typename View>
inline void lodepng_write_view(const std::string& filename,const View& view) {
    lodepng_write_view(filename.c_str(),view);
}*/

} }  // namespace boost::gil

#endif
