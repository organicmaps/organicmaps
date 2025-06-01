#include "html_processor.hpp"

#include <string>
#include <string_view>

std::string RemovePTagsWithNonMatchedLanguages(std::string_view html, std::string_view lang)
{
  std::string_view static constexpr kPLangStart = "<p lang=\"";
  std::string_view static constexpr kPLangEnd = "</p>";

  std::string result(html);  // Copy input into mutable string
  std::size_t pos = 0;

  while (pos != std::string::npos && pos < result.size())
  {
    // Find opening <p lang="...">
    std::size_t startTagStart = result.find(kPLangStart, pos);
    if (startTagStart == std::string::npos)
      break;

    std::size_t startTagEnd = result.find('>', startTagStart);
    if (startTagEnd == std::string::npos)
      break;

    // Extract the lang value
    std::size_t langStart = startTagStart + kPLangStart.size();
    std::size_t langEnd = result.find('"', langStart);
    if (langEnd == std::string::npos)
      break;

    if (lang != result.substr(langStart, langEnd - langStart))
    {
      // Language doesn't match - find closing </p>
      std::size_t endTagStart = result.find(kPLangEnd, startTagEnd);
      if (endTagStart == std::string::npos)
        break;

      std::size_t endTagEnd = endTagStart + kPLangEnd.size();

      // Remove entire block
      result.erase(startTagStart, endTagEnd - startTagStart);

      // Restart from same position after removal
      pos = startTagStart;
    }
    else
    {
      // Move past this tag
      pos = startTagEnd + 1;
    }
  }

  return result;
}
