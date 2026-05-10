#pragma once

#include "base/buffer_vector.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace languages
{
/// @note These functions are heavy enough to call them often. Be careful.

/// @return List of original system languages in the form "en-US|ru-RU|es|zh-Hant".
std::string GetPreferred();

/// @return Original language code for the current user in the form "en-US", "zh-Hant".
std::string GetCurrentOrig();

/// @return Current language in out Twine translations compatible format, e.g. "en", "pt" or "zh-Hant".
std::string GetCurrentTwine();
std::string GetCurrentMapTwine();

/// @return Normalized language code for the current user in the form "en", "zh".
/// Returned languages are normalized to our supported languages in the core, see
/// string_utf8_multilang.cpp and should not be used with any sub-locales like zh-Hans/zh-Hant. Some
/// langs like Danish (da) are not supported in the core too, but used as a locale.
std::string Normalize(std::string_view lang);
std::string GetCurrentNorm();
std::string GetCurrentMapLanguage();

buffer_vector<std::string, 4> const & GetSystemPreferred();

/// Locale-driven CJK font variant resolver. Owns the variant enum, locale/SFNT-family-name
/// /filename → variant mappings, and the cross-variant fallback chain (e.g. HK → TC → SC).
class CJKResolver
{
public:
  enum class Variant : uint8_t
  {
    JP,
    KR,
    SC,
    TC,
    HK,
  };

  /// Case-insensitive, primary-subtag aware. SC for non-CJK tags so callers always have a usable
  /// variant.
  static Variant FromLanguageTag(std::string tag);
  static std::optional<Variant> FromSfntFamilyName(std::string_view family) noexcept;
  static std::optional<Variant> FromFontFileName(std::string_view fileName) noexcept;

  /// True if the filename looks like a Pan-CJK font collection (e.g. NotoSansCJK-Regular.ttc),
  /// for which the consumer should pick a face index via the resolver instead of loading face 0.
  static bool IsCJKContainerFileName(std::string_view fileName) noexcept;

  /// Best-to-worst fallback chain (first = userVariant). Last-resort entries (e.g. KR for an HK
  /// user) keep some CJK rendering rather than .notdef boxes — a wrong-region glyph beats no glyph.
  static std::array<Variant, 5> FallbackChain(Variant userVariant) noexcept;

  CJKResolver();

  Variant User() const noexcept { return m_user; }
  std::array<Variant, 5> FallbackChain() const noexcept { return FallbackChain(m_user); }

private:
  Variant m_user;
};

std::string DebugPrint(CJKResolver::Variant v);
}  // namespace languages
