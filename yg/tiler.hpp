#pragma once

#include "../geometry/screenbase.hpp"
#include "../geometry/rect2d.hpp"
#include "../std/queue.hpp"
#include "../std/functional.hpp"

namespace yg
{
  class Tiler
  {
  public:

    struct RectInfo
    {
      int m_drawScale;

      int m_tileScale;
      int m_x;
      int m_y;

      m2::RectD m_rect;
      double m_distance;
      double m_coverage;

      RectInfo();
      RectInfo(int drawScale, int tileScale, int x, int y, m2::RectD const & globRect, m2::PointD const & centerPt);

      void init(m2::RectD const & globRect, m2::PointD const & centerPt);

      /// pack scale, x, y into 64 bit word to use it as a hash-map key
      uint64_t toUInt64Cell() const;
      /// unpack 64bit integer and compute other parameters
      void fromUInt64Cell(uint64_t v, m2::RectD const & globRect, m2::PointD const & centerPt);
    };

  private:

    ScreenBase m_screen;
    int m_drawScale;
    int m_tileScale;
    size_t m_seqNum;

    priority_queue<RectInfo, vector<RectInfo>, greater<RectInfo> > m_coverage;

  public:

    Tiler();

    /// seed tiler with new screenBase.
    /// if there are an existing tile sequence it is
    /// reorganized to reflect screen changes.
    void seed(ScreenBase const & screenBase, m2::PointD const & centerPt, int tileSize, int scaleEtalonSize);

    /// check whether the sequence has next tile
    bool hasTile();
    /// pop tile from the sequence and return it
    RectInfo const nextTile();

    size_t seqNum() const;
  };

  bool operator <(Tiler::RectInfo const & l, Tiler::RectInfo const & r);
  bool operator >(Tiler::RectInfo const & l, Tiler::RectInfo const & r);
}
