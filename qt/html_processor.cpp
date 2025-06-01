#include "html_processor.hpp"

#include <string>
#include <string_view>

void RemovePTagsWithNonMatchedLanguages(std::string_view lang, std::string & html)
{
  std::string_view constexpr kPLangStart = "<p lang=\"";
  std::string_view constexpr kPLangEnd = "</p>";

  std::string::size_type pos = 0;

  while (pos != std::string::npos && pos < html.size())
  {
    // Find opening <p lang="...">
    auto const startTagStart = html.find(kPLangStart, pos);
    if (startTagStart == std::string::npos)
      break;

    auto const startTagEnd = html.find('>', startTagStart);
    if (startTagEnd == std::string::npos)
      break;

    // Extract the lang value
    auto const langStart = startTagStart + kPLangStart.size();
    auto const langEnd = html.find('"', langStart);
    if (langEnd == std::string::npos)
      break;

    if (0 == html.compare(langStart, langEnd - langStart, lang))
    {
      // Move past this tag
      pos = startTagEnd + 1;
      continue;
    }

    // Language doesn't match - find closing </p>
    auto const endTagStart = html.find(kPLangEnd, startTagEnd);
    if (endTagStart == std::string::npos)
      break;

    auto const endTagEnd = endTagStart + kPLangEnd.size();

    // Remove entire block
    html.erase(startTagStart, endTagEnd - startTagStart);

    // Restart from same position after removal
    pos = startTagStart;
  }
}
