#include "base/logging.hpp"
#include "base/string_utils.hpp"
#include "unicode/uchar.h"

#include <regex>
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

  bool allUppercaseNum = true;
  strings::UniString lowerStr = strings::MakeLowerCase(myUniStr);
  for (int i = myUniStr.size() - 1; i > 0; i--)
  {
    // if we've reached a space, we're done here
    if (myUniStr[i] == ' ')
      break;
    // we've found a char that is already lowercase and not a number,
    // therefore the string is not exclusively uppercase/numeric
    else if (myUniStr[i] == lowerStr[i] && !u_isdigit(myUniStr[i]))
    {
      allUppercaseNum = false;
      break;
    }
  }
  return allUppercaseNum;
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

  bool foundIndeterminate = false;
  strings::MakeLowerCaseInplace(myUniStr);  // this isn't an acronym, so match based on lowercase

  std::u32string_view constexpr front{U"eéöőüű"};
  std::u32string_view constexpr back{U"aáoóuú"};
  std::u32string_view constexpr indeterminate{U"ií"};

  // find last vowel in last word
  for (size_t i = myUniStr.size() - 1; i > 0; i--)
  {
    if (front.find(myUniStr[i]) != std::string::npos)
      return 1;
    if (back.find(myUniStr[i]) != std::string::npos)
      return 2;
    if (indeterminate.find(myUniStr[i]) != std::string::npos)
      foundIndeterminate = true;
    // if we've hit a space with only indeterminates, it's back
    if (myUniStr[i] == U' ' && foundIndeterminate == true)
      return 2;
    // if we've hit a space with no vowels at all, check for numbers and acronyms
    if (myUniStr[i] == U' ' && foundIndeterminate == false)
      return CategorizeHungarianAcronymsAndNumbers(hungarianString);
  }
  // if we got here, are we even reading Hungarian words?
  LOG(LWARNING, ("Hungarian word not found:", hungarianString));
  return 2;  // default
}

}  // namespace routing::turns::sound
