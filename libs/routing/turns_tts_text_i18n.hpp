#pragma once

#include "base/string_utils.hpp"

#include <memory>
#include <string>

namespace routing
{
namespace turns
{
namespace sound
{

/**
 * @brief Modifies a string's last character to harmonize its vowel with a -ra/-re suffix
 *
 * @param std::string & myString - string to harmonize and modify
 */
void HungarianBaseWordTransform(std::string & hungarianString);

/**
 * @brief Decides if a string ends in all uppercase or numbers (no lowercase before a space)
 *
 * @param std::string const & myString - an unknown string like "Main Street" or "Highway 50"
 * @note Used for Hungarian so we don't need to worry about Unicode numerics, only ASCII
 * @return true - only uppercase and numbers before ending space; false - lowercase ending word
 */
bool EndsInAcronymOrNum(strings::UniString const & myUniStr);

/**
 * @brief Decides if an uppercase/numeric string has a "front" or "back" ending.
 *
 * If the last two characters in an acronym or number (i.e. we won't say ABC or 123 as if they were
 * words, we will spell it out like ay bee see or one hundred twenty three) then in Hungarian we
 * start by looking at the last two characters. If the last two characters are 10, 40, 50, 70, 90
 * then we have a "-re" ending because of how it's pronounced. If they're 20, 30, 60, 80 then
 * they'll have a "-ra" ending.
 * A phrase ending in "-hundred" is a special case, so if the last three letters are "100" then that
 * has a "-ra" ending.
 * If none of the above are true, then we can simply look at the last character in the string for
 * the appropriate suffix. If the last character is one of AÁHIÍKOÓUŰ0368 then it gets a "-re"
 * ending. All other cases will get a "-ra" ending however we can't simply stop there because if
 * there is some unknown character like whitespace or punctuation we have to keep looking further
 * backwards into the string until we find a match or we run off the end of the word (" ").
 *
 * @arg std::string const & myString - numbers or acronyms to be spoken in Hungarian
 * @return 1 = front = -re, 2 = back = -ra
 */
uint8_t CategorizeHungarianAcronymsAndNumbers(std::string const & hungarianString);

/**
 * @brief Decides if a string (possibly Hungarian) has a "front" or "back" ending.
 *
 * Much like the Acronym/Number algorithm above, we start from the back of the word and
 * keep trying to match a front or back vowel until we find one. Indeterminate vowels are
 * "back" by default but only if we find nothing else. And if we truly find nothing, it
 * may be an acronym after all. (The word "acerbic" has a different ending sound than ABC.)
 *
 * @arg std::string const & myString - words, numbers or acronyms to be spoken in Hungarian
 * @return 1 = front = -re, 2 = back = -ra
 */
uint8_t CategorizeHungarianLastWordVowels(std::string const & hungarianString);

}  // namespace sound
}  // namespace turns
}  // namespace routing
