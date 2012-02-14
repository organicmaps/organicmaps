#pragma once

#include "../coding/multilang_utf8_string.hpp"

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

    SearchParams() : m_mode(All),
      m_inputLanguageCode(StringUtf8Multilang::UNSUPPORTED_LANGUAGE_CODE),
      m_validPos(false) {}

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

    /// @param[in] language can be "fr", "en-US", "ru_RU" etc.
    inline void SetInputLanguage(string const & language)
    {
      // @TODO take into an account zh_pinyin, ko_rm and ja_rm
      size_t delimPos = language.find("-");
      if (delimPos == string::npos)
        delimPos = language.find("_");
      m_inputLanguageCode = StringUtf8Multilang::GetLangIndex(delimPos == string::npos
                                        ? language: language.substr(0, delimPos));
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
    /// Can be -1 (@see StringUtf8Multilang::UNSUPPORTED_LANGUAGE_CODE) if input
    /// language is not known
    int8_t m_inputLanguageCode;

    double m_lat, m_lon;
    bool m_validPos;
  };
}
