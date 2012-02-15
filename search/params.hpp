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

    SearchParams();

    void SetNearMeMode(bool b);
    void SetPosition(double lat, double lon);
    /// @param[in] language can be "fr", "en-US", "ru_RU" etc.
    void SetInputLanguage(string const & language);

    bool IsNearMeMode() const;
    bool IsLanguageValid() const;

  public:
    SearchCallbackT m_callback;

    string m_query;
    ModeT m_mode;
    /// Can be -1 (@see StringUtf8Multilang::UNSUPPORTED_LANGUAGE_CODE), in the
    /// case of input language is not known
    int8_t m_inputLanguageCode;

    double m_lat, m_lon;
    bool m_validPos;
  };
} // namespace search
