#include "base/SRC_FIRST.hpp"
#include "testing/testing.hpp"
#include "3party/lodepng/lodepng_io.hpp"
#include "platform/platform.hpp"

#include <boost/gil/gil_all.hpp>
#include <boost/mpl/vector_c.hpp>
#include "std/iostream.hpp"
#include "std/iomanip.hpp"

#include "graphics/texture.hpp"

namespace gil = boost::gil;
namespace mpl = boost::mpl;

typedef yg::gl::RGBA4Texture::pixel_t pixel_t;
typedef yg::gl::RGBA4Texture::view_t view_t;
typedef yg::gl::RGBA4Texture::image_t image_t;
//typedef yg::gl::Texture::iterator_t iterator_t;
//typedef yg::gl::Texture::locator_t locator_t;

UNIT_TEST(ABGR4_GIL_COPY)
{
  image_t image(4, 4);
  view_t view = gil::view(image);

  gil::fill_pixels(view, pixel_t(0x0, 0x0, 0x0, 0x0));

  for (size_t y = 0; y < 4; ++y)
  {
    for (size_t x = 0; x < 4; ++x)
    {
      unsigned short val = *((unsigned short *)&view(x, y));
      cout << hex << val << " ";
    }
    cout << endl;
  }

  pixel_t cl(0xF, 0, 0xF, 0);

  view(1, 1) = cl;
  view(1, 2) = cl;
  view(2, 1) = cl;
  view(2, 2) = cl;

  for (size_t y = 0; y < 4; ++y)
  {
    for (size_t x = 0; x < 4; ++x)
    {
      unsigned short val = *((unsigned short *)&view(x, y));

      cout << hex << val << " ";
    }
    cout << endl;
  }
}

UNIT_TEST(FormatABGR4_Layout)
{
  pixel_t px[2] = {pixel_t(0xF, 0xE, 0xD, 0xC), pixel_t(0xF, 0xE, 0xD, 0xC)};

  uint32_t raw = *(uint32_t *)(px);

  cout << "packed 2 pixels " << hex << raw << std::endl;

  TEST_EQUAL(raw, 0xCDEFCDEF, ());
}

UNIT_TEST(FormatRGBA4444_Layout)
{
  pixel_t px(15, 14, 13, 12);
  uint16_t * pxp = (uint16_t*)&px;
  uint16_t val = *pxp;

  cout << "shifting 1 << 4 = " << (1 << 4) << endl;
  cout << "shifting 1 >> 4 = " << (1 >> 4) << endl;

  cout << "value : " << hex << val
       << ", sh1 : " << hex << ((val) & 0xF)
       << ", sh2 : " << hex << ((val >> 4) & 0xF)
       << ", sh3 : " << hex << ((val >> 8) & 0xF)
       << ", sh4 : " << hex << ((val >> 12) & 0xF) << endl;

  TEST_EQUAL(gil::get_color(px, gil::alpha_t()), 15, ());
  TEST_EQUAL(gil::get_color(px, gil::blue_t()), 14, ());
  TEST_EQUAL(gil::get_color(px, gil::green_t()), 13, ());
  TEST_EQUAL(gil::get_color(px, gil::red_t()), 12, ());
}

UNIT_TEST(FormatRGBA4444_SaveLoad)
{
  image_t image;

//  gil::lodepng_read_image((GetPlatform().ResourcesDir() + "basic.png").c_str(), image);
}
