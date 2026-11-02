#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace platform
{
/// Sources of localized text strings loaded from JSON resources.
enum class TextSource
{
  TtsSound = 0,  //!< Maneuvers text to speech strings.
  Countries,     //!< Countries names strings.
};

class GetTextById;
using TGetTextByIdPtr = std::unique_ptr<GetTextById>;
using TTranslations = std::vector<std::pair<std::string, std::string>>;

/// Looks up text strings loaded from JSON in a specified locale.
class GetTextById
{
public:
  /// @return the localized string for textId, or an empty string if textId is not found.
  std::string operator()(std::string const & textId) const;
  std::string GetLocale() const { return m_locale; }
  TTranslations GetAllSortedTranslations() const;

  static TGetTextByIdPtr Create(std::string const & jsonBuffer, std::string const & localeName);

private:
  GetTextById(std::string const & jsonBuffer, std::string const & localeName);

  /// \note IsValid is used only in factories and shall be private.
  bool IsValid() const { return !m_localeTexts.empty(); }

  std::string m_locale;
  std::unordered_map<std::string, std::string> m_localeTexts;
};

/// Factories to create GetTextById instances.
/// If TGetTextByIdPtr is created by GetTextByIdFactory or ForTestingGetTextByIdFactory
/// there are only two possibities:
/// * a factory returns a valid instance
/// * a factory returns nullptr
TGetTextByIdPtr GetTextByIdFactory(TextSource textSource, std::string const & localeName);
TGetTextByIdPtr ForTestingGetTextByIdFactory(std::string const & jsonBuffer, std::string const & localeName);

/// \brief Reads the JSON text resource for textSource and localeName into jsonBuffer.
/// @return true if the resource was read successfully, false otherwise.
bool GetJsonBuffer(platform::TextSource textSource, std::string const & localeName, std::string & jsonBuffer);
}  // namespace platform
