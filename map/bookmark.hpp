#pragma once

#include "../geometry/point2d.hpp"

#include "../std/string.hpp"


class Bookmark
{
  m2::PointD m_org;
  string m_name;

public:
  Bookmark() {}
  Bookmark(m2::PointD const & org, string const & name)
    : m_org(org), m_name(name)
  {
  }

  m2::PointD GetOrg() const { return m_org; }
  string const & GetName() const { return m_name; }
  m2::RectD GetViewport() const { return m2::RectD(m_org, m_org); }
};
