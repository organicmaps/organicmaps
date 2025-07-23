#pragma once

#include <string>
#include <string_view>

/// Function takes contents of a "copyright.html" file via an "html" parameter and removes
/// all blocks from it which "lang" attribute doesn't match the "lang" parameter.
void RemovePTagsWithNonMatchedLanguages(std::string_view lang, std::string & html);
