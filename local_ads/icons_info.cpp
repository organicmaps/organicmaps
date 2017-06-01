#include "local_ads/icons_info.hpp"

#include "coding/reader.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <functional>

namespace
{
char const kDelimiter = '_';

void ParseIconsFile(std::string const & iconsFile,
                    std::function<void(std::string const &)> const & handler)
{
  ASSERT(handler != nullptr, ());
  try
  {
    std::string fileData;
    ReaderPtr<Reader>(GetPlatform().GetReader(iconsFile)).ReadAsString(fileData);
    strings::Tokenize(fileData, "\n", [&handler](std::string const & str) {
      if (!str.empty())
        handler(str);
    });
  }
  catch (Reader::Exception const & e)
  {
    LOG(LWARNING, ("Error reading file ", iconsFile, " : ", e.what()));
  }
}
}  // namespace

namespace local_ads
{
IconsInfo & IconsInfo::Instance()
{
  static IconsInfo iconsInfo;
  return iconsInfo;
}

void IconsInfo::SetSourceFile(std::string const & fileName)
{
  std::map<uint16_t, std::string> icons;
  ParseIconsFile(fileName, [&icons](std::string const & icon) {
    auto const pos = icon.find(kDelimiter);
    if (pos == std::string::npos)
      return;
    uint32_t index;
    if (!strings::to_uint(icon.substr(0, pos), index))
      index = 0;
    icons[static_cast<uint16_t>(index)] = icon;
  });

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_fileName = fileName;
    std::swap(m_icons, icons);
  }
}

std::string IconsInfo::GetIcon(uint16_t index) const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  auto const it = m_icons.find(index);
  if (it == m_icons.end())
    return {};
  return it->second;
}
}  // namespace local_ads
