#pragma once

#include "map/user_mark.hpp"
#include "map/user_mark_container.hpp"

#include "search/result.hpp"

#include "indexer/feature.hpp"

#include "geometry/point2d.hpp"

#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/utility.hpp"


class StyledPoint : public ICustomDrawable
{
public:
  StyledPoint(m2::PointD const & ptOrg, UserMarkContainer * container)
    : ICustomDrawable(ptOrg, container)
  {
  }

  StyledPoint(string const & style, m2::PointD const & ptOrg, UserMarkContainer * container)
    : ICustomDrawable(ptOrg, container)
    , m_style(style)
  {
  }

  virtual graphics::DisplayList * GetDisplayList(UserMarkDLCache * cache) const = 0;
  virtual double GetAnimScaleFactor() const = 0;
  virtual m2::PointD const & GetPixelOffset() const = 0;

  string const & GetStyle() const { return m_style; }
  void SetStyle(const string & style) { m_style = style; }

private:
  string m_style;  ///< Point style (name of icon).
};
