#include "qt/build_style/build_skins.h"

#include "qt/build_style/build_common.h"

#include "platform/platform.hpp"

#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"

#include <exception>
#include <fstream>
#include <string>
#include <unordered_map>

#include <QtCore/QDir>

namespace
{
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
void BuildSkinImpl(QString const & styleDir, QString const & suffix, int size, QString const & outputDir)
{
  QString const symbolsDir = JoinPathQt({styleDir, "symbols"});

  // Check symbols directory exists
  if (!QDir(symbolsDir).exists())
    throw std::runtime_error("Symbols directory does not exist");

  // Caller ensures that output directory is clear
  if (QDir(outputDir).exists())
    throw std::runtime_error("Output directory is not clear");

  // Create output skin directory (mkpath so the symbols/<dpi>/ parent is also created).
  if (!QDir().mkpath(outputDir))
    throw std::runtime_error("Cannot create output skin directory");

  // Create symbolic link for symbols/png
  QString const pngOriginDir = styleDir + suffix;
  QString const pngDir = JoinPathQt({styleDir, "symbols", "png"});
  QFile::remove(pngDir);
  if (!QFile::link(pngOriginDir, pngDir))
    throw std::runtime_error("Unable to create symbols/png link");
  SCOPE_GUARD(cleaner, [&pngDir]() { QFile::remove(pngDir); });

  QString const strSize = QString::number(size);
  // Run the script.
  (void)ExecProcess(GetExternalPath("skin_generator_tool", "skin_generator_tool.app/Contents/MacOS", ""),
                    {
                        "--symbolWidth",
                        strSize,
                        "--symbolHeight",
                        strSize,
                        "--symbolsDir",
                        symbolsDir,
                        "--skinName",
                        JoinPathQt({outputDir, "basic"}),
                        "--skinSuffix=",
                    });

  // Check if files were created.
  if (QFile(JoinPathQt({outputDir, "symbols.png"})).size() == 0 ||
      QFile(JoinPathQt({outputDir, "symbols.sdf"})).size() == 0)
  {
    throw std::runtime_error("Skin files have not been created");
  }
}

void BuildSkins(QString const & styleDir, QString const & outputDir, QString const & theme)
{
  auto const resolution2size = GetSkinSizes(JoinPathQt({styleDir, "resolutions.txt"}));

  // Sequential by design: every BuildSkinImpl call recreates the same
  // symbols/png symlink, so the DPI buckets must not run concurrently.
  for (auto const & dpi : kSkinDpis)
  {
    QString const outputSkinDir = JoinPathQt({outputDir, "symbols", dpi.m_name, theme});
    BuildSkinImpl(styleDir, dpi.m_name, resolution2size.at(dpi.m_name), outputSkinDir);
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
