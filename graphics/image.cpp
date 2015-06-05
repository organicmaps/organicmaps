#include "graphics/image.hpp"

#include "graphics/opengl/data_traits.hpp"

#include "indexer/map_style_reader.hpp"

#include "coding/lodepng_io.hpp"

namespace gil = boost::gil;

namespace graphics
{
  m2::PointU const GetDimensions(string const & resName, EDensity density)
  {
    ReaderPtr<Reader> reader = GetStyleReader().GetResourceReader(resName, density);
    gil::point2<ptrdiff_t> size = gil::lodepng_read_dimensions(reader);
    return m2::PointU(size.x, size.y);
  }

  Image::Info::Info()
    : Resource::Info(Resource::EImage),
      m_size(0, 0)
  {}

  Image::Info::Info(char const * name, EDensity density)
    : Resource::Info(Resource::EImage),
      m_resourceName(name),
      m_size(0, 0)
  {
    try
    {
      m_size = GetDimensions(name, density);
      m_data.resize(m_size.x * m_size.y * sizeof(DATA_TRAITS::pixel_t));

      DATA_TRAITS::view_t v = gil::interleaved_view(
                  m_size.x, m_size.y,
                  (DATA_TRAITS::pixel_t*)&m_data[0],
                  m_size.x * sizeof(DATA_TRAITS::pixel_t));

      ReaderPtr<Reader> reader = GetStyleReader().GetResourceReader(name, density);

      gil::lodepng_read_and_convert_view(reader, v, DATA_TRAITS::color_converter());
    }
    catch (RootException const &)
    {
    }
  }

  unsigned Image::Info::width() const
  {
    return m_size.x;
  }

  unsigned Image::Info::height() const
  {
    return m_size.y;
  }

  unsigned char const * Image::Info::data() const
  {
    return &m_data[0];
  }

  bool Image::Info::lessThan(Resource::Info const * r) const
  {
    if (m_category != r->m_category)
      return m_category < r->m_category;

    Image::Info const * ri = static_cast<Image::Info const *>(r);

    if (m_resourceName != ri->m_resourceName)
      return m_resourceName < ri->m_resourceName;

    return false;
  }

  Resource::Info const & Image::Info::cacheKey() const
  {
    return *this;
  }

  m2::PointU const Image::Info::resourceSize() const
  {
    return m2::PointU(m_size.x + 4, m_size.y + 4);
  }

  Resource * Image::Info::createResource(m2::RectU const & texRect,
                                         uint8_t pipelineID) const
  {
    return new Image(texRect,
                     pipelineID,
                     *this);
  }

  Image::Image(m2::RectU const & texRect,
               int pipelineID,
               Info const & info)
    : Resource(EImage, texRect, pipelineID),
      m_info(info)
  {
  }

  void Image::render(void * dst)
  {
    DATA_TRAITS::view_t dstView = gil::interleaved_view(
                m_info.m_size.x + 4, m_info.m_size.y + 4,
          (DATA_TRAITS::pixel_t*)dst,
          (m_info.m_size.x + 4) * sizeof(DATA_TRAITS::pixel_t));

    if ((m_info.m_size.x != 0) && (m_info.m_size.y != 0))
    {
      DATA_TRAITS::view_t srcView = gil::interleaved_view(
            m_info.m_size.x, m_info.m_size.y,
            (DATA_TRAITS::pixel_t*)&m_info.m_data[0],
            m_info.m_size.x * sizeof(DATA_TRAITS::pixel_t));

      gil::copy_pixels(
            srcView,
            gil::subimage_view(dstView, 2, 2, m_info.m_size.x, m_info.m_size.y));
    }

    DATA_TRAITS::pixel_t pxBorder = DATA_TRAITS::createPixel(Color(0, 0, 0, 0));

    for (unsigned y = 0; y < 2; ++y)
      for (unsigned x = 0; x < m_info.m_size.x + 4; ++x)
        dstView(x, y) = pxBorder;

    for (unsigned y = 2; y < m_info.m_size.y + 2; ++y)
    {
      for (unsigned x = 0; x < 2; ++x)
        dstView(x, y) = pxBorder;
      for (unsigned x = m_info.m_size.x + 2; x < m_info.m_size.x + 4; ++x)
        dstView(x, y) = pxBorder;
    }

    for (unsigned y = m_info.m_size.y + 2; y < m_info.m_size.y + 4; ++y)
      for (unsigned x = 0; x < m_info.m_size.x + 4; ++x)
        dstView(x, y) = pxBorder;
  }

  Resource::Info const * Image::info() const
  {
    return &m_info;
  }
}
