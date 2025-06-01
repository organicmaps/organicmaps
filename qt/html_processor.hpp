#pragma once

#include <string>
#include <string_view>

/// Function takes contents of a "copyright.html" file and removes all blocks 
/// from it which "lang" attribute doesn't match the "lang" parameter of the function.
std::string RemovePTagsWithNonMatchedLanguages(std::string_view html, std::string_view lang);
