#pragma once

#include "../std/string.hpp"

#include "../geometry/point2d.hpp"

namespace graphics
{
  /// get dimensions of PNG image specified by it's resourceName.
  m2::PointU const GetDimensions(string const & resourceName);

  class ImageInfo
  {
  private:
    vector<unsigned char> m_data;
    m2::PointU m_size;
    string m_resourceName;
  public:
    /// create ImageInfo from PNG resource
    ImageInfo(char const * resourceName);

    unsigned width() const;
    unsigned height() const;

    unsigned char const * data() const;

    friend bool operator < (ImageInfo const & l, ImageInfo const & r);
  };

  bool operator < (ImageInfo const & l, ImageInfo const & r);
}
