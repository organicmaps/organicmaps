#include "qt/build_style/build_skins.h"

#include "qt/build_style/build_common.h"

#include "tools/skin_generator/generator.hpp"

#include "platform/platform.hpp"

#include <stdexcept>
#include <string>

#include <QtCore/QDir>

namespace
{
// Same default as skin_generator_tool's --maxSize.
uint32_t constexpr kMaxTextureSize = 4096;
}  // namespace

namespace build_style
{
void BuildSkins(QString const & styleDir, QString const & outputDir, QString const & theme)
{
  QString const symbolsDir = JoinPathQt({styleDir, "symbols"});
  if (!QDir(symbolsDir).exists())
    throw std::runtime_error("Symbols directory does not exist: " + symbolsDir.toStdString());

  for (auto const & dpi : kSkinDpis)
  {
    QString const outputSkinDir = JoinPathQt({outputDir, "symbols", dpi.m_name, theme});
    if (!QDir().mkpath(outputSkinDir))
      throw std::runtime_error("Cannot create output skin directory: " + outputSkinDir.toStdString());

    tools::BuildSkin(symbolsDir, dpi.m_size, kMaxTextureSize, outputSkinDir);
  }
}

void ApplySkins(QString const & outputDir, QString const & theme)
{
  // symbols/<dpi>/<theme>/ in the writable dir shadows the bundled atlases ("wrf" scope); in a
  // dev checkout this overwrites data/symbols/ in place. The atlases are pixel-identical to the
  // ones tools/unix/generate_symbols.sh produces but not byte-identical, because that script also
  // runs optipng on them — see docs/STYLES.md before committing symbol changes.
  QString const writableDir = GetPlatform().WritableDir().c_str();

  for (auto const & dpi : kSkinDpis)
  {
    QString const outputSkinDir = JoinPathQt({outputDir, "symbols", dpi.m_name, theme});
    QString const writableSkinDir = JoinPathQt({writableDir, "symbols", dpi.m_name, theme});

    if (!QFileInfo::exists(writableSkinDir) && !QDir().mkpath(writableSkinDir))
      throw std::runtime_error("Cannot create skin directory: " + writableSkinDir.toStdString());

    if (!CopyQtFile(JoinPathQt({outputSkinDir, "symbols.png"}), JoinPathQt({writableSkinDir, "symbols.png"})) ||
        !CopyQtFile(JoinPathQt({outputSkinDir, "symbols.xml"}), JoinPathQt({writableSkinDir, "symbols.xml"})))
    {
      throw std::runtime_error("Cannot copy skins files");
    }
  }
}
}  // namespace build_style
