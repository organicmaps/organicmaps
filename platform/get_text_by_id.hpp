#pragma once

#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/unordered_map.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace platform
{
// class GetTextById is ready to work with different sources of text strings.
// For the time being it's only strings for TTS.
enum class TextSource
{
  TtsSound = 0, //!< Maneuvers text to speech strings.
  Countries,    //!< Countries names strings.
  Cuisines      //!< OSM cuisines translations.
};

class GetTextById;
using TGetTextByIdPtr = unique_ptr<GetTextById>;
using TTranslations = vector<pair<string, string>>;

/// GetTextById represents text messages which are saved in textsDir
/// in a specified locale.
class GetTextById
{
public:
  /// @return a pair of a text string in a specified locale for textId and a boolean flag.
  /// If textId is found in m_localeTexts then the boolean flag is set to true.
  /// The boolean flag is set to false otherwise.
  string operator()(string const & textId) const;
  string GetLocale() const { return m_locale; }
  TTranslations GetAllSortedTranslations() const;

private:
  friend TGetTextByIdPtr GetTextByIdFactory(TextSource textSource, string const & localeName);
  friend TGetTextByIdPtr ForTestingGetTextByIdFactory(string const & jsonBuffer,
                                                      string const & localeName);
  friend TGetTextByIdPtr MakeGetTextById(string const & jsonBuffer, string const & localeName);

  GetTextById(string const & jsonBuffer, string const & localeName);
  void InitFromJson(string const & jsonBuffer);
  /// \note IsValid is used only in factories and shall be private.
  bool IsValid() const { return !m_localeTexts.empty(); }

  string m_locale;
  unordered_map<string, string> m_localeTexts;
};

/// Factories to create GetTextById instances.
/// If TGetTextByIdPtr is created by GetTextByIdFactory or ForTestingGetTextByIdFactory
/// there are only two possibities:
/// * a factory returns a valid instance
/// * a factory returns nullptr
TGetTextByIdPtr GetTextByIdFactory(TextSource textSource, string const & localeName);
TGetTextByIdPtr ForTestingGetTextByIdFactory(string const & jsonBuffer, string const & localeName);

/// \bried fills jsonBuffer with json file in twine format with strings in a language of localeName.
/// @return true if no error was happened and false otherwise.
bool GetJsonBuffer(platform::TextSource textSource, string const & localeName, string & jsonBuffer);
}  // namespace platform
