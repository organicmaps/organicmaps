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
      AROUND_POSITION = 1,
      IN_VIEWPORT = 2,
      SEARCH_WORLD = 4,
      SEARCH_ADDRESS = 8,
      ALL = AROUND_POSITION | IN_VIEWPORT | SEARCH_WORLD | SEARCH_ADDRESS
    };

    inline void SetSearchMode(int mode) { m_searchMode = mode; }
    inline bool NeedSearch(SearchModeT mode) const { return ((m_searchMode & mode) != 0); }
    inline bool IsSortByViewport() const { return m_searchMode == IN_VIEWPORT; }
    //@}

    void SetPosition(double lat, double lon);
    bool IsValidPosition() const { return m_validPos; }

    /// @param[in] language can be "fr", "en-US", "ru_RU" etc.
    void SetInputLanguage(string const & language);
    bool IsLanguageValid() const;

    bool IsEqualCommon(SearchParams const & rhs) const;

    void Clear() { m_query.clear(); }

  public:
    SearchCallbackT m_callback;

    string m_query;
    /// Can be -1 (@see StringUtf8Multilang::UNSUPPORTED_LANGUAGE_CODE),
    /// in the case when input language is unknown.
    int8_t m_inputLanguageCode;

    double m_lat, m_lon;

    friend string DebugPrint(SearchParams const & params);

  private:
    int m_searchMode;
    bool m_forceSearch, m_validPos;
  };

} // namespace search
