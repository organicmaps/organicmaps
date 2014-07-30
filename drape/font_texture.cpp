#include "font_texture.hpp"
#include "pointers.hpp"
#include "utils/stb_image.h"

#include "../platform/platform.hpp"
#include "../coding/reader.hpp"

#include "../base/logging.hpp"
#include "../base/string_utils.hpp"
#include "../base/stl_add.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/map.hpp"
#include "../std/bind.hpp"

#include <boost/gil/algorithm.hpp>
#include <boost/gil/typedefs.hpp>

using boost::gil::gray8c_pixel_t;
using boost::gil::gray8_pixel_t;
using boost::gil::gray8c_view_t;
using boost::gil::gray8_view_t;
using boost::gil::interleaved_view;
using boost::gil::subimage_view;
using boost::gil::copy_pixels;

typedef gray8_view_t view_t;
typedef gray8c_view_t const_view_t;
typedef gray8_pixel_t pixel_t;
typedef gray8c_pixel_t const_pixel_t;

namespace dp
{

FontTexture::GlyphInfo::GlyphInfo(m2::RectF const & texRect, float xOffset,
                                  float yOffset, float advance)
  : ResourceInfo(texRect)
  , m_xOffset(xOffset)
  , m_yOffset(yOffset)
  , m_advance(advance)
{
}

void FontTexture::GlyphInfo::GetMetrics(float & xOffset, float & yOffset, float & advance) const
{
  xOffset = m_xOffset;
  yOffset = m_yOffset;
  advance = m_advance;
}

////////////////////////////////////////////////////////////////////////

Texture::ResourceInfo const * FontTexture::FindResource(Texture::Key const & key) const
{
  if (key.GetType() != Texture::Glyph)
    return NULL;

  int unicodePoint = static_cast<GlyphKey const &>(key).GetUnicodePoint();
  glyph_map_t::const_iterator it = m_glyphs.find(unicodePoint);
  if (it == m_glyphs.end())
    return NULL;

  return &it->second;
}

void FontTexture::Add(int unicodePoint, GlyphInfo const & glyphInfo)
{
  m_glyphs.insert(make_pair(unicodePoint, glyphInfo));
}

namespace
{
  class Grid
  {
    typedef pair<m2::RectU, FontTexture *> region_t;
    typedef vector<region_t> regions_t;

  public:
    void CutTexture(vector<uint8_t> const & image, uint32_t width, uint32_t height)
    {
      uint32_t maxTextureSize = Texture::GetMaxTextureSize();

      if (width <= maxTextureSize && width == height)
        SingleTexture(image, width, height);
      else
      {
        const_view_t srcView = interleaved_view(width, height, (const_pixel_t *)&image[0], width);
        uint32_t baseSize = maxTextureSize;
        vector<m2::RectU> notProcessed;
        notProcessed.push_back(m2::RectU(0, 0, width, height));
        while (!notProcessed.empty())
        {
          vector<m2::RectU> outRects;
          for (size_t i = 0; i < notProcessed.size(); ++i)
            CutTextureBySize(srcView, notProcessed[i], baseSize, outRects);

          baseSize >>= 1;
          swap(notProcessed, outRects);
        }
      }
    }

    void ParseMetrics(string const & fileData)
    {
      vector<string> lines;
      strings::Tokenize(fileData, "\n", MakeBackInsertFunctor(lines));
      for (size_t i = 0; i < lines.size(); ++i)
      {
        vector<string> metrics;
        strings::Tokenize(lines[i], "\t", MakeBackInsertFunctor(metrics));
        ASSERT(metrics.size() == 8, ());

        int32_t unicodePoint;
        int32_t x, y, w, h;
        double xoff, yoff, advance;

        strings::to_int(metrics[0], unicodePoint);
        strings::to_int(metrics[1], x);
        strings::to_int(metrics[2], y);
        strings::to_int(metrics[3], w);
        strings::to_int(metrics[4], h);
        strings::to_double(metrics[5], xoff);
        strings::to_double(metrics[6], yoff);
        strings::to_double(metrics[7], advance);

        m2::PointU centerPoint(x + w / 2, y + h / 2);
        region_t region = GetTexture(centerPoint);
        FontTexture * texture = region.second;
        m2::RectU rect = region.first;
        m2::RectF texRect(texture->GetS(x - rect.minX()), texture->GetT(y - rect.minY()),
                          texture->GetS(x + w - rect.minX()), texture->GetT(y + h - rect.minY()));
        texture->Add(unicodePoint, FontTexture::GlyphInfo(texRect, xoff, yoff, advance));
      }
    }

    void GetTextures(vector<TransferPointer<Texture> > & textures)
    {
      textures.reserve(m_regions.size());
      for (size_t i = 0; i < m_regions.size(); ++i)
        textures.push_back(MovePointer<Texture>(m_regions[i].second));
    }

  private:
    void SingleTexture(vector<uint8_t> const & image, uint32_t width, uint32_t height)
    {
      FontTexture * texture = new FontTexture();
      texture->Create(width, height, Texture::ALPHA, MakeStackRefPointer((void *)&image[0]));
      m_regions.push_back(make_pair(m2::RectU(0, 0, width, height), texture));
    }

    void CutTextureBySize(const_view_t const & image, m2::RectU const & fullRect,
                          uint32_t cutSize, vector<m2::RectU> & notProcessedRects)
    {
      uint32_t fullTexInWidth = fullRect.SizeX() / cutSize;
      uint32_t fullTexInHeight = fullRect.SizeY() / cutSize;

      if (fullTexInWidth == 0 || fullTexInHeight == 0)
      {
        notProcessedRects.push_back(fullRect);
        return;
      }

      vector<uint8_t> regionImage(cutSize * cutSize, 0);
      for (uint32_t dy = 0; dy < fullTexInHeight; ++dy)
      {
        for (uint32_t dx = 0; dx < fullTexInWidth; ++dx)
        {
          uint32_t pxDx = dx * cutSize + fullRect.minX();
          uint32_t pxDy = dy * cutSize + fullRect.minY();
          const_view_t subView = subimage_view(image, pxDx, pxDy, cutSize, cutSize);

          view_t dstView = interleaved_view(cutSize, cutSize,
                                            (pixel_t *)&regionImage[0], cutSize);

          copy_pixels(subView, dstView);
          FontTexture * texture = new FontTexture();
          texture->Create(cutSize, cutSize, Texture::ALPHA,
                          MakeStackRefPointer<void>(&regionImage[0]));

          m_regions.push_back(make_pair(m2::RectU(pxDx, pxDy,
                                                  pxDx + cutSize,
                                                  pxDy + cutSize),
                                        texture));
        }
      }

      uint32_t downBorder = fullTexInHeight * cutSize;
      uint32_t rightBorder = fullTexInWidth * cutSize;
      if (rightBorder == fullRect.SizeX())
      {
        ASSERT(downBorder == fullRect.SizeY(), ());
        return;
      }

      notProcessedRects.push_back(m2::RectU(rightBorder, 0, fullRect.maxX(), downBorder));

      if (downBorder == fullRect.SizeY())
        return;

      notProcessedRects.push_back(m2::RectU(0, downBorder, rightBorder, fullRect.maxY()));
    }

  private:
    region_t GetTexture(m2::PointU const & px)
    {
      regions_t::const_iterator it = find_if(m_regions.begin(), m_regions.end(),
                                             bind(&m2::RectU::IsPointInside,
                                                  bind(&region_t::first, _1), px));

      ASSERT(it != m_regions.end(), ());
      return *it;
    }

  private:
    regions_t m_regions;
  };
}

void LoadFont(string const & resourcePrefix, vector<TransferPointer<Texture> > & textures)
{
  string metrics;
  int w, h, channelCount;
  vector<uint8_t> imageData;

  try
  {
    {
      ReaderPtr<ModelReader> reader = GetPlatform().GetReader(resourcePrefix + ".png");
      imageData.resize(reader.Size());
      reader.Read(0, &imageData[0], imageData.size());

      unsigned char * img = stbi_png_load_from_memory(&imageData[0], imageData.size(),
                                                      &w, &h, &channelCount, 0);
      CHECK(channelCount == 1, ("Incorrect font texture format"));

      imageData.resize(w * h);
      memcpy(&imageData[0], img, w * h);
      stbi_image_free(img);
    }

    {
      ReaderPtr<ModelReader> reader = GetPlatform().GetReader(resourcePrefix + ".fdf");
      reader.ReadAsString(metrics);
    }
  }
  catch (RootException & e)
  {
    LOG(LERROR, (e.what()));
  }

  Grid grid;
  grid.CutTexture(imageData, w, h);
  grid.ParseMetrics(metrics);
  grid.GetTextures(textures);
}

} // namespace dp
