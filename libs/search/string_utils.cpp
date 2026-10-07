#include "search/string_utils.hpp"

#include "indexer/search_string_utils.hpp"

#include "base/stl_helpers.hpp"

#include <algorithm>
#include <regex>

namespace search
{

std::string RemoveAddressDetails(std::string const & query)
{
  // Bound regex work for pasted text; unusually long queries use the normal tokenizer unchanged.
  if (query.size() > 4096)
    return query;
  static std::regex const unitPrefix(R"(^\s*(?:(?:apt|apartment|unit|suite)\.?\s*#?\s*|#\s*)[0-9]+[a-z]?[,\s]+)",
                                     std::regex::icase);
  static std::regex const street(
      R"(^\s*[0-9]+[a-z]?[ ,]+[^,\r\n]+?\b(?:street|st|avenue|ave|road|rd|boulevard|blvd|drive|dr|lane|ln|court|ct|crescent|cres|place|pl|way|terrace|ter)\b)",
      std::regex::icase);
  static std::regex const unitSuffix(
      R"((?:[,\s]*#\s*|[,\s]+(?:apt|apartment|unit|suite)\.?\s*#?\s*)[0-9]+[a-z]?(?:-[0-9a-z]+)?\b)",
      std::regex::icase);
  static std::regex const canadianPostcode(
      R"(\b[abceghj-nprstvxy][0-9][abceghj-nprstv-z][\s-]?[0-9][abceghj-nprstv-z][0-9]\b)", std::regex::icase);
  static std::regex const township(R"(\b(?:twp|twnshp)\b\.?)", std::regex::icase);
  static std::regex const borough(R"(\bboro\b\.?)", std::regex::icase);

  auto value = std::regex_replace(query, unitPrefix, "");
  // Australia Post uses unit/house order. Do not apply it to countries where the
  // slash forms part of the civic house identifier.
  static std::regex const australianUnit(R"(^\s*[0-9]+[a-z]?\s*/\s*([0-9]+[a-z]?)\s+)", std::regex::icase);
  static std::regex const australia(R"(\bAustralia\s*$)", std::regex::icase);
  if (std::regex_search(value, australia))
    value = std::regex_replace(value, australianUnit, "$1 ");
  std::smatch match;
  if (!std::regex_search(value, match, street))
    return query;

  auto const streetEnd = static_cast<size_t>(match.length());
  auto context = std::regex_replace(value.substr(streetEnd), unitSuffix, " ");
  // Expand administrative suffixes only after the street; never rewrite a street or ordinary POI query.
  context = std::regex_replace(context, township, "Township");
  context = std::regex_replace(context, borough, "Borough");
  // Only omit a postal code when locality context remains; postcode-only searches keep their meaning.
  auto const withoutPostcode = std::regex_replace(context, canadianPostcode, " ");
  if (std::any_of(withoutPostcode.begin(), withoutPostcode.end(),
                  [](unsigned char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }))
    context = withoutPostcode;
  return value.substr(0, streetEnd) + context;
}

QueryString MakeQueryString(std::string s)
{
  QueryString qs;
  qs.m_query = std::move(s);

  Delimiters delims;
  auto const uniString = NormalizeAndSimplifyString(qs.m_query);
  SplitUniString(uniString, base::MakeBackInsertFunctor(qs.m_tokens), delims);

  if (!qs.m_tokens.empty() && !delims(uniString.back()))
  {
    qs.m_prefix = qs.m_tokens.back();
    qs.m_tokens.pop_back();
  }

  return qs;
}

}  // namespace search
