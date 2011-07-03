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
      int m_scale;
      int m_x;
      int m_y;

      m2::RectD m_rect;
      double m_distance;
      double m_coverage;

      RectInfo();
      RectInfo(int scale, int x, int y, m2::RectD const & globRect);

      void init(m2::RectD const & globRect);

      /// pack scale, x, y into 64 bit word to use it as a hash-map key
      uint64_t toUInt64Cell() const;
      /// unpack 64bit integer and compute other parameters
      void fromUInt64Cell(uint64_t v, m2::RectD const & globRect);
    };

  private:

    ScreenBase m_screen;
    int m_scale;
    size_t m_seqNum;

    priority_queue<RectInfo, vector<RectInfo>, greater<RectInfo> > m_coverage;

  public:

    Tiler();

    /// seed tiler with new screenBase.
    /// if there are an existing tile sequence it is
    /// reorganized to reflect screen changes.
    void seed(ScreenBase const & screenBase, int scaleEtalonSize);

    /// check whether the sequence has next tile
    bool hasTile();
    /// pop tile from the sequence and return it
    RectInfo const nextTile();

    size_t seqNum() const;
  };

  bool operator <(Tiler::RectInfo const & l, Tiler::RectInfo const & r);
  bool operator >(Tiler::RectInfo const & l, Tiler::RectInfo const & r);
}
