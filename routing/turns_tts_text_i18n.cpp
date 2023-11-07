#include "base/string_utils.hpp"
#include "base/logging.hpp"

#include <string>
#include <regex>

namespace routing
{
namespace turns
{
namespace sound
{

template <std::size_t N>
long FindInStrArray(const std::array<strings::UniChar, N>& haystack, strings::UniChar needle)
{
  auto begin = haystack.begin();
  for(auto i = begin; i != haystack.end(); ++i) {
    const strings::UniChar item = *i;
    if (item == needle)
      return (long)i;
  }
  return -1;
}

void HungarianBaseWordTransform(std::string & hungarianString) {
  std::pair<std::string_view, std::string_view> constexpr kToReplace[] = {{"e", "é"}, {"a", "á"}, {"ö", "ő"}, {"ü", "ű"}};
  for (auto [base, harmonized] : kToReplace)
  {
    if (strings::EndsWith(hungarianString, base))
    {
      hungarianString.replace(hungarianString.size() - base.size(), base.size(), harmonized);
      break;
    }
  }
}

bool EndsInAcronymOrNum(strings::UniString const & myUniStr)
{
  bool allUppercaseNum = true;
  strings::UniString lowerStr = strings::MakeLowerCase(myUniStr);
  for (int i = myUniStr.size() - 1; i > 0; i--) {

    // if we've reached a space, we're done here
    if (myUniStr[i] == ' ')
      break;
    // we've found a char that is already lowercase and not a number,
    // therefore the string is not exclusively uppercase/numeric
    else if (myUniStr[i] == lowerStr[i] && !std::isdigit(myUniStr[i])) {
      allUppercaseNum = false;
      break;
    }
  }
  return allUppercaseNum;
}

uint8_t CategorizeHungarianAcronymsAndNumbers(std::string const & hungarianString)
{
  constexpr std::array<std::string_view, 14> backNames = {
      "A", // a
      "Á", // á
      "H", // há
      "I", // i
      "Í", // í
      "K", // ká
      "O", // o
      "Ó", // ó
      "U", // u
      "Ű", // ú
      "0", // nulla or zéró
      "3", // három
      "6", // hat
      "8", // nyolc
  };

  constexpr std::array<std::string_view, 31> frontNames = {
      // all other letters besides H and K
      "B", "C", "D", "E", "É", "F", "G", "J", "L", "M", "N", "Ö", "Ő", "P", "Q", "R", "S", "T", "Ú", "Ü", "V", "W", "X", "Y", "Z",
      "1", // egy
      "2", // kettő
      "4", // négy
      "5", // öt
      "7", // hét
      "9", // kilenc
  };

  constexpr std::array<std::string_view, 5> specialCaseFront = {
      "10", // tíz special case front
      "40", // negyven front
      "50", // ötven front
      "70", // hetven front
      "90", // kilencven front
  };

  constexpr std::array<std::string_view, 4> specialCaseBack = {
      "20", // húsz back
      "30", // harminc back
      "60", // hatvan back
      "80", // nyolcvan back
  }; //TODO: consider regex

  //'100', // száz back


  for (size_t i = hungarianString.length(); i > 0; i--) {
    std::string hungarianSub = hungarianString.substr(0, i);
    for (auto myCase : specialCaseFront)
      if (strings::EndsWith(hungarianSub,myCase))
        return 1;
    for (auto myCase : specialCaseBack)
      if (strings::EndsWith(hungarianSub,myCase))
        return 2;
    if (strings::EndsWith(hungarianSub,"100"))
      return 2;

    for (auto myCase : frontNames)
      if (strings::EndsWith(hungarianSub,myCase))
        return 1;
    for (auto myCase : backNames)
      if (strings::EndsWith(hungarianSub,myCase))
        return 2;
    if (strings::EndsWith(hungarianSub," "))
      return 2;
  }

  LOG(LWARNING, ("Unable to find Hungarian front/back for", hungarianString));
  return 2;
}

uint8_t CategorizeHungarianLastWordVowels(std::string const & hungarianString)
{
  constexpr std::array<strings::UniChar, 6> front = {U'e',U'é',U'ö',U'ő',U'ü',U'ű'};
  constexpr std::array<strings::UniChar, 6> back = {U'a',U'á',U'o',U'ó',U'u',U'ú'};
  constexpr std::array<strings::UniChar, 2> indeterminate = {U'i',U'í'};

  strings::UniString myUniStr = strings::MakeUniString(strings::MakeLowerCase(hungarianString));

  // scan for acronyms and numbers first (i.e. characters spoken differently than words)
  // if the last word is an acronym/number like M5, check those instead
  if (EndsInAcronymOrNum(myUniStr)) {
    return CategorizeHungarianAcronymsAndNumbers(hungarianString);
  }

  bool foundIndeterminate = false;

  // find last vowel in last word
  for (size_t i=myUniStr.size()-1; i>0; i--) {
    LOG(LWARNING, ("i", i, (char)myUniStr[i], "Y"));
    if (FindInStrArray(front, myUniStr[i]) != -1){
      LOG(LWARNING, ("returning 1"));
      return 1;
    }
    if (FindInStrArray(back, myUniStr[i]) != -1) {
      LOG(LWARNING, ("returning 2"));
      return 2;
    }
    if (FindInStrArray(indeterminate, myUniStr[i]) != -1) {
      LOG(LWARNING, ("finding ind"));
      foundIndeterminate = true;
    }
    if (myUniStr[i] == U' ' && foundIndeterminate == true) { // if we've hit a space with only indeterminates, it's back
      LOG(LWARNING, ("returning 2 (space, ind)"));
      return 2;
    }
    if (myUniStr[i] == U' ' && foundIndeterminate == false) { // if we've hit a space with no vowels at all, check for numbers and acronyms
      LOG(LWARNING, ("categorizing acronym (space, ind)"));
      return CategorizeHungarianAcronymsAndNumbers(hungarianString);
    }
  }
  // if we got here, are we even reading Hungarian words?
  LOG(LWARNING, ("Hungarian word not found:", hungarianString));
  return 2; // default
}

}  // namespace sound
}  // namespace turns
}  // namespace routing
