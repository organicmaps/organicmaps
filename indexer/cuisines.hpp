#pragma once

#include "platform/get_text_by_id.hpp"
#include "platform/preferred_languages.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"

namespace osm
{
using TAllCuisines = vector<pair<string, string>>;

class Cuisines
{
  Cuisines() = default;

public:
  static Cuisines & Instance();
  /// @param[out] outCuisines contains list of parsed cuisines (not localized).
  void Parse(string const & osmRawCuisinesTagValue, vector<string> & outCuisines);
  /// @param[in] lang should be in our twine strings.txt/cuisines.txt format.
  /// @param[out] outCuisines contains list of parsed cuisines (localized).
  void ParseAndLocalize(string const & osmRawCuisinesTagValue, vector<string> & outCuisines,
                        string const & lang = languages::GetCurrentTwine());
  /// @param[in] lang should be in our twine strings.txt/cuisines.txt format.
  /// @returns translated cuisine (can be empty, if we can't translate key).
  string Translate(string const & singleOsmCuisine,
                   string const & lang = languages::GetCurrentTwine());
  /// @returns list of osm cuisines in cuisines.txt (not localized).
  TAllCuisines AllSupportedCuisines();

private:
  platform::TGetTextByIdPtr m_translations;
};
}  // namespace osm
