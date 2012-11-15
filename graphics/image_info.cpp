#include "image_info.hpp"

#include "opengl/data_traits.hpp"

#include "../platform/platform.hpp"

#include "../coding/lodepng_io.hpp"

namespace gil = boost::gil;

namespace graphics
{
  m2::PointU const GetDimensions(string const & resourceName)
  {
    ReaderPtr<Reader> reader = GetPlatform().GetReader(resourceName);
    gil::point2<ptrdiff_t> size = gil::lodepng_read_dimensions(reader);
    return m2::PointU(size.x, size.y);
  }

  ImageInfo::ImageInfo(char const * resourceName)
  {
    m_size = GetDimensions(resourceName);
    m_data.resize(m_size.x * m_size.y * sizeof(DATA_TRAITS::pixel_t));

    DATA_TRAITS::view_t v = gil::interleaved_view(
                m_size.x, m_size.y,
                (DATA_TRAITS::pixel_t*)&m_data[0],
                m_size.x * sizeof(DATA_TRAITS::pixel_t));

    ReaderPtr<Reader> reader = GetPlatform().GetReader(resourceName);

    gil::lodepng_read_and_convert_view(reader, v, DATA_TRAITS::color_converter());
  }

  unsigned ImageInfo::width() const
  {
    return m_size.x;
  }

  unsigned ImageInfo::height() const
  {
    return m_size.y;
  }

  unsigned char const * ImageInfo::data() const
  {
    return &m_data[0];
  }

  bool operator<(ImageInfo const & l, ImageInfo const & r)
  {
    return l.m_resourceName < r.m_resourceName;
  }
}
