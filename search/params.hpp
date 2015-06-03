#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "std/function.hpp"
#include "std/string.hpp"


namespace search
{
  class Results;
  typedef function<void (Results const &)> SearchCallbackT;

  class SearchParams
  {
  public:
    SearchParams();

    /// @name Force run search without comparing with previous search params.
    //@{
    void SetForceSearch(bool b) { m_forceSearch = b; }
    bool IsForceSearch() const { return m_forceSearch; }
    //@}

    /// @name Search modes.
    //@{
    enum SearchModeT
    {
      IN_VIEWPORT_ONLY = 1,
      SEARCH_WORLD = 2,
      SEARCH_ADDRESS = 4,
      ALL = SEARCH_WORLD | SEARCH_ADDRESS
    };

    inline void SetSearchMode(int mode) { m_searchMode = mode; }
    inline bool HasSearchMode(SearchModeT mode) const { return ((m_searchMode & mode) != 0); }
    //@}

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
    SearchCallbackT m_callback;

    string m_query;
    string m_inputLocale;

    double m_lat, m_lon;

    friend string DebugPrint(SearchParams const & params);

  private:
    double m_searchRadiusM;
    int m_searchMode;
    bool m_forceSearch, m_validPos;
  };

} // namespace search
