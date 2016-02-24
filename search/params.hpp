#pragma once

#include "search/mode.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "std/function.hpp"
#include "std/string.hpp"


namespace search
{
  class Results;

  using TOnStarted = function<void()>;
  using TOnResults = function<void (Results const &)>;

  class SearchParams
  {
  public:
    SearchParams();

    /// @name Force run search without comparing with previous search params.
    //@{
    void SetForceSearch(bool b) { m_forceSearch = b; }
    bool IsForceSearch() const { return m_forceSearch; }
    //@}

    inline void SetMode(Mode mode) { m_mode = mode; }
    inline Mode GetMode() const { return m_mode; }

    void SetPosition(double lat, double lon);
    bool IsValidPosition() const { return m_validPos; }
    bool IsSearchAroundPosition() const
    {
      return (m_searchRadiusM > 0 && IsValidPosition());
    }

    void SetSearchRadiusMeters(double radiusM) { m_searchRadiusM = radiusM; }
    bool GetSearchRect(m2::RectD & rect) const;

    /// @param[in] locale can be "fr", "en-US", "ru_RU" etc.
    void SetInputLocale(string const & locale) { m_inputLocale = locale; }

    bool IsEqualCommon(SearchParams const & rhs) const;

    void Clear() { m_query.clear(); }

  public:
    TOnStarted m_onStarted;
    TOnResults m_onResults;

    string m_query;
    string m_inputLocale;

    double m_lat, m_lon;

    friend string DebugPrint(SearchParams const & params);

  private:
    double m_searchRadiusM;
    Mode m_mode;
    bool m_forceSearch, m_validPos;
  };
} // namespace search
