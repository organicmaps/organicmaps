#include "qt/build_style/build_skins.h"

#include "qt/build_style/build_common.h"

#include "tools/skin_generator/generator.hpp"

#include "platform/platform.hpp"

#include "base/string_utils.hpp"

#include <exception>
#include <fstream>
#include <string>
#include <unordered_map>

#include <QtCore/QDir>

namespace
{
// Same default as skin_generator_tool's --maxSize.
uint32_t constexpr kMaxTextureSize = 4096;

// Symbol sizes per DPI bucket: kSkinDpis defaults, overridable via resolutions.txt.
std::unordered_map<std::string, int> GetSkinSizes(QString const & file)
{
  std::unordered_map<std::string, int> skinSizes;

  for (auto const & dpi : build_style::kSkinDpis)
    skinSizes.emplace(dpi.m_name, dpi.m_size);

  try
  {
    std::ifstream ifs(file.toStdString());

    std::string line;
    while (std::getline(ifs, line))
    {
      size_t const pos = line.find('=');
      if (pos == std::string::npos)
        continue;

      std::string name(line.begin(), line.begin() + pos);
      std::string valueTxt(line.begin() + pos + 1, line.end());

      strings::Trim(name);
      strings::Trim(valueTxt);
      int const value = std::stoi(valueTxt);

      if (value <= 0)
        continue;

      skinSizes[name] = value;
    }
  }
  catch (std::exception const &)
  {
    // Reset to defaults.
    for (auto const & dpi : build_style::kSkinDpis)
      skinSizes[dpi.m_name] = dpi.m_size;
  }

  return skinSizes;
}
}  // namespace

namespace build_style
{
void BuildSkins(QString const & styleDir, QString const & outputDir, QString const & theme)
{
  QString const symbolsDir = JoinPathQt({styleDir, "symbols"});
  if (!QDir(symbolsDir).exists())
    throw std::runtime_error("Symbols directory does not exist: " + symbolsDir.toStdString());

  auto const resolution2size = GetSkinSizes(JoinPathQt({styleDir, "resolutions.txt"}));

  for (auto const & dpi : kSkinDpis)
  {
    QString const outputSkinDir = JoinPathQt({outputDir, "symbols", dpi.m_name, theme});
    if (!QDir().mkpath(outputSkinDir))
      throw std::runtime_error("Cannot create output skin directory: " + outputSkinDir.toStdString());

    // Pre-rendered per-DPI overrides (historically <styleDir>/<dpi>/) take
    // precedence over rendering the SVG sources.
    tools::BuildSkin(symbolsDir, styleDir + dpi.m_name, resolution2size.at(dpi.m_name), kMaxTextureSize, outputSkinDir);
  }
}

void ApplySkins(QString const & outputDir, QString const & theme)
{
  // symbols/<dpi>/<theme>/ in the writable dir shadows the bundled atlases
  // ("wrf" scope); in a dev checkout this overwrites data/symbols/ in place,
  // exactly like tools/unix/generate_symbols.sh.
  QString const writableDir = GetPlatform().WritableDir().c_str();

  for (auto const & dpi : kSkinDpis)
  {
    QString const outputSkinDir = JoinPathQt({outputDir, "symbols", dpi.m_name, theme});
    QString const writableSkinDir = JoinPathQt({writableDir, "symbols", dpi.m_name, theme});

    if (!QFileInfo::exists(writableSkinDir) && !QDir().mkpath(writableSkinDir))
      throw std::runtime_error("Cannot create skin directory: " + writableSkinDir.toStdString());

    if (!CopyFile(JoinPathQt({outputSkinDir, "symbols.png"}), JoinPathQt({writableSkinDir, "symbols.png"})) ||
        !CopyFile(JoinPathQt({outputSkinDir, "symbols.sdf"}), JoinPathQt({writableSkinDir, "symbols.sdf"})))
    {
      throw std::runtime_error("Cannot copy skins files");
    }
  }
}
}  // namespace build_style
