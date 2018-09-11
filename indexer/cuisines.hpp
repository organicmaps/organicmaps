#pragma once

#include "platform/get_text_by_id.hpp"
#include "platform/preferred_languages.hpp"

#include <string>
#include <utility>
#include <vector>

namespace osm
{
using AllCuisines = std::vector<std::pair<std::string, std::string>>;

class Cuisines
{
  Cuisines() = default;

public:
  static Cuisines & Instance();
  /// @param[out] outCuisines contains list of parsed cuisines (not localized).
  void Parse(std::string const & osmRawCuisinesTagValue, std::vector<std::string> & outCuisines);
  /// @param[in] lang should be in our twine strings.txt/cuisines.txt format.
  /// @param[out] outCuisines contains list of parsed cuisines (localized).
  void ParseAndLocalize(std::string const & osmRawCuisinesTagValue, std::vector<std::string> & outCuisines,
                        std::string const & lang = languages::GetCurrentTwine());
  /// @param[in] lang should be in our twine strings.txt/cuisines.txt format.
  /// @returns translated cuisine (can be empty, if we can't translate key).
  std::string Translate(std::string const & singleOsmCuisine,
                        std::string const & lang = languages::GetCurrentTwine());
  /// @returns list of osm cuisines in cuisines.txt.
  AllCuisines AllSupportedCuisines(std::string const & lang = languages::GetCurrentTwine());

private:
  platform::TGetTextByIdPtr m_translations;
};
}  // namespace osm
