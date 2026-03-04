#include "base/logging.hpp"
#include "base/string_utils.hpp"
#include "unicode/uchar.h"

#include <string>

namespace routing::turns::sound
{

void HungarianBaseWordTransform(std::string & hungarianString)
{
  if (hungarianString.length() == 0)
    return;

  strings::UniString myUniStr = strings::MakeUniString(hungarianString);

  std::pair<char32_t, char32_t> constexpr kToReplace[] = {{U'e', U'é'}, {U'a', U'á'}, {U'ö', U'ő'}, {U'ü', U'ű'}};
  auto & lastChar = myUniStr.back();
  for (auto [base, harmonized] : kToReplace)
  {
    if (lastChar == base)
    {
      lastChar = harmonized;
      hungarianString = strings::ToUtf8(myUniStr);
      return;
    }
  }
}

bool EndsInAcronymOrNum(strings::UniString const & myUniStr)
{
  if (myUniStr.empty())
    return false;

  size_t capitalCharsCount = 0;
  for (size_t i = myUniStr.size(); i-- > 0;)
  {
    if (strings::IsASCIISpace(myUniStr[i]))
      break;
    if (!u_isupper(myUniStr[i]) && !u_isdigit(myUniStr[i]))
      return false;
    capitalCharsCount++;
  }
  return capitalCharsCount > 0;
}

uint8_t CategorizeHungarianAcronymsAndNumbers(std::string const & hungarianString)
{
  if (hungarianString.empty())
    return 2;

  std::array<std::string_view, 14> constexpr backNames = {
      "A",  // a
      "Á",  // á
      "H",  // há
      "I",  // i
      "Í",  // í
      "K",  // ká
      "O",  // o
      "Ó",  // ó
      "U",  // u
      "Ű",  // ú
      "0",  // nulla or zéró
      "3",  // három
      "6",  // hat
      "8",  // nyolc
  };

  std::array<std::string_view, 31> constexpr frontNames = {
      // all other letters besides H and K
      "B", "C", "D", "E", "É", "F", "G", "J", "L", "M", "N", "Ö", "Ő",
      "P", "Q", "R", "S", "T", "Ú", "Ü", "V", "W", "X", "Y", "Z",
      "1",  // egy
      "2",  // kettő
      "4",  // négy
      "5",  // öt
      "7",  // hét
      "9",  // kilenc
  };

  std::array<std::string_view, 5> constexpr specialCaseFront = {
      "10",  // tíz special case front
      "40",  // negyven front
      "50",  // ötven front
      "70",  // hetven front
      "90",  // kilencven front
  };

  std::array<std::string_view, 4> constexpr specialCaseBack = {
      "20",  // húsz back
      "30",  // harminc back
      "60",  // hatvan back
      "80",  // nyolcvan back
  };

  // "100", // száz back, handled below

  // Compare the end of our string with the views above. The order is chosen
  // in priority of what would most likely cause an ending vowel sound change.
  for (auto myCase : specialCaseFront)
    if (hungarianString.ends_with(myCase))
      return 1;
  for (auto myCase : specialCaseBack)
    if (hungarianString.ends_with(myCase))
      return 2;
  if (hungarianString.ends_with("100"))
    return 2;
  for (auto myCase : frontNames)
    if (hungarianString.ends_with(myCase))
      return 1;
  for (auto myCase : backNames)
    if (hungarianString.ends_with(myCase))
      return 2;
  if (hungarianString.ends_with(' '))
    return 2;

  LOG(LWARNING, ("Unable to find Hungarian front/back for", hungarianString));
  return 2;
}

uint8_t CategorizeHungarianLastWordVowels(std::string const & hungarianString)
{
  if (hungarianString.empty())
    return 2;

  strings::UniString myUniStr = strings::MakeUniString(hungarianString);

  // scan for acronyms and numbers first (i.e. characters spoken differently than words)
  // if the last word is an acronym/number like M5, check those instead
  if (EndsInAcronymOrNum(myUniStr))
    return CategorizeHungarianAcronymsAndNumbers(hungarianString);

  strings::MakeLowerCaseInplace(myUniStr);  // this isn't an acronym, so match based on lowercase

  std::u32string_view constexpr kFront{U"eéöőüű"};
  std::u32string_view constexpr kBack{U"aáoóuú"};
  std::u32string_view constexpr kIndeterminate{U"ií"};

  bool foundIndeterminate = false;
  for (size_t i = myUniStr.size(); i-- > 0;)
  {
    auto const ch = myUniStr[i];
    if (kFront.find(ch) != std::u32string_view::npos)
      return 1;
    if (kBack.find(ch) != std::u32string_view::npos)
      return 2;
    if (kIndeterminate.find(ch) != std::u32string_view::npos)
      foundIndeterminate = true;
    if (ch == U' ')
      return foundIndeterminate ? 2 : CategorizeHungarianAcronymsAndNumbers(hungarianString);
  }

  LOG(LWARNING, ("Hungarian word not found:", hungarianString));
  return 2;
}

}  // namespace routing::turns::sound
