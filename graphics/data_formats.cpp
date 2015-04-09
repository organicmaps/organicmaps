#include "graphics/data_formats.hpp"

namespace graphics
{
  struct DataFormatInfo
  {
    graphics::DataFormat m_fmt;
    unsigned m_size;
    char const * m_name;
    DataFormatInfo(graphics::DataFormat fmt, unsigned size, char const * name)
      : m_fmt(fmt), m_size(size), m_name(name)
    {}
  };

  DataFormatInfo s_info [] = {DataFormatInfo(Data4Bpp, 2, "Data4Bpp"),
                              DataFormatInfo(Data8Bpp, 4, "Data8Bpp"),
                              DataFormatInfo(Data565Bpp, 2, "Data565Bpp"),
                              DataFormatInfo((graphics::DataFormat)0, 0, "Unknown")};

  DataFormatInfo const findInfo(graphics::DataFormat const & fmt)
  {
    unsigned const cnt = sizeof(s_info) / sizeof(DataFormatInfo);
    for (unsigned i = 0; i < cnt; ++i)
      if (s_info[i].m_fmt == fmt)
        return s_info[i];

    return s_info[cnt - 1];
  }

  unsigned formatSize(DataFormat const & fmt)
  {
    return findInfo(fmt).m_size;
  }

  char const * formatName(DataFormat const & fmt)
  {
    return findInfo(fmt).m_name;
  }
}
