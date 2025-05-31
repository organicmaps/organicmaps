#include "html_processor.hpp"

#include <string>

#include <QString>

QString ProcessCopyrightHtml(std::string const & html, std::string const & lang)
{
  QString result = html.c_str();

  int pos = 0;

  // Loop to find and remove <p lang="..."> tags
  while (true)
  {
    // Find opening <p lang="...">
    int startTagStart = result.indexOf("<p lang=\"", pos);
    if (startTagStart == -1)
      break;

    int startTagEnd = result.indexOf('>', startTagStart);
    if (startTagEnd == -1)
      break;

    // Extract the lang value
    int langStart = startTagStart + strlen("<p lang=\"");
    int langEnd = result.indexOf('"', langStart);
    if (langEnd == -1)
      break;

    std::string tagLang = result.mid(langStart, langEnd - langStart).toStdString();

    // If language doesn't match, find closing </p> and remove entire block
    if (tagLang != lang)
    {
      int endTagStart = result.indexOf("</p>", startTagEnd);
      if (endTagStart == -1)
        break;

      int endTagEnd = endTagStart + strlen("</p>");
      result.remove(startTagStart, endTagEnd - startTagStart);
      pos = startTagStart;  // Restart from same place after removal
    }
    else
    {
      pos = startTagEnd + 1;  // Continue searching
    }
  }

  return result;
}
