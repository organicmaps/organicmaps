#pragma once

#include "../geometry/screenbase.hpp"
#include "../geometry/rect2d.hpp"
#include "../std/vector.hpp"
#include "../std/functional.hpp"

class Tiler
{
public:

  struct RectInfo
  {
    int m_drawScale; //< used to select data from model (0, 17)
    int m_tileScale; //< bounds / (2 ^ m_tileScale) is a size of the grid for a tile,
                       //< could be larger than maximum model scale
    int m_x;
    int m_y;

    m2::RectD m_rect;

    RectInfo();
    RectInfo(int drawScale, int tileScale, int x, int y);

    void initRect();
  };

private:

  ScreenBase m_screen;
  m2::PointD m_centerPt;
  int m_drawScale;
  int m_tileScale;

  int getDrawScale(ScreenBase const & s, int ts, double k) const;
  int getTileScale(ScreenBase const & s, int ts) const;

public:

  Tiler();

  /// seed tiler with new screenBase.
  void seed(ScreenBase const & screenBase, m2::PointD const & centerPt);

  void tiles(vector<RectInfo> & tiles, int depth);
  bool isLeaf(RectInfo const & ri) const;

  int drawScale() const;
  int tileScale() const;
};

struct LessByScaleAndDistance
{
  m2::PointD m_pt;
  LessByScaleAndDistance(m2::PointD const & pt);
  bool operator()(Tiler::RectInfo const & l, Tiler::RectInfo const & r);
};

bool operator<(Tiler::RectInfo const & l, Tiler::RectInfo const & r);
bool operator==(Tiler::RectInfo const & l, Tiler::RectInfo const & r);
