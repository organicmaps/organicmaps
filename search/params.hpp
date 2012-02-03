#pragma once

#include "../geometry/point2d.hpp"

#include "../std/function.hpp"
#include "../std/string.hpp"


namespace search
{
  class Results;
  typedef function<void (Results const &)> SearchCallbackT;

  class SearchParams
  {
  public:
    enum ModeT { All, NearMe };

    SearchParams() : m_mode(All), m_validPos(false) {}

    inline void SetNearMeMode(bool b)
    {
      m_mode = (b ? NearMe : All);
    }

    inline void SetPosition(double lat, double lon)
    {
      m_lat = lat;
      m_lon = lon;
      m_validPos = true;
    }

    inline bool IsNearMeMode() const
    {
      // this mode is valid only with correct My Position
      return (m_mode == NearMe && m_validPos);
    }

  public:
    SearchCallbackT m_callback;

    string m_query;
    ModeT m_mode;

    double m_lat, m_lon;
    bool m_validPos;
  };
}
