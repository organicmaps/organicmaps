#include "drape/glyph_manager.hpp"
#include "drape/harfbuzz_shaping.hpp"

#include "platform/platform.hpp"

#include "base/string_utils.hpp"

#include <fstream>
#include <iostream>
#include <numeric>  // std::accumulate

void ItemizeLine(std::string & str, dp::GlyphManager & glyphManager)
{
  strings::Trim(str);
  if (str.empty())
    return;

  auto const segments = harfbuzz_shaping::GetTextSegments(str);
  for (auto const & run : segments.m_segments)
  {
    std::u16string_view sv{segments.m_text.data() + run.m_start, static_cast<size_t>(run.m_length)};
    std::cout << DebugPrint(sv) << '|';
  }
  std::cout << '\n';
}

int main(int argc, char ** argv)
{
  if (argc < 2)
  {
    std::cout << "Debugging tool to experiment with text segmentation\n";
    std::cout << "Usage: " << argv[0] << " [text file with utf8 strings or any arbitrary text string]\n";
    return -1;
  }

  dp::GlyphManager::Params params;
  params.m_uniBlocks = "unicode_blocks.txt";
  params.m_whitelist = "fonts_whitelist.txt";
  params.m_blacklist = "fonts_blacklist.txt";
  GetPlatform().GetFontNames(params.m_fonts);

  dp::GlyphManager glyphManager(params);

  if (Platform::IsFileExistsByFullPath(argv[1]))
  {
    std::ifstream file(argv[1]);
    std::string line;
    while (file.good())
    {
      std::getline(file, line);
      ItemizeLine(line, glyphManager);
    }
  }
  else
  {
    // Get all args as one string.
    std::vector<std::string> const args(argv + 1, argv + argc);
    auto line = std::accumulate(args.begin(), args.end(), std::string{});
    ItemizeLine(line, glyphManager);
  }
  return 0;
}
