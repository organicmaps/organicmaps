#include "build_skins.h"
#include "build_common.h"

#include "platform/platform.hpp"

#include <array>
#include <algorithm>
#include <exception>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <fstream>

#include <QtCore/QDir>

namespace
{
enum SkinType
{
  SkinMDPI,
  SkinHDPI,
  SkinXHDPI,
  SkinXXHDPI,
  Skin6Plus,

  // SkinCount MUST BE last
  SkinCount
};

using SkinInfo = std::tuple<const char*, int, bool>;
SkinInfo const g_skinInfo[SkinCount] =
{
  std::make_tuple("mdpi", 18, false),
  std::make_tuple("hdpi", 27, false),
  std::make_tuple("xhdpi", 36, false),
  std::make_tuple("xxhdpi", 54, false),
  std::make_tuple("6plus", 54, false),
};

std::array<SkinType, SkinCount> const g_skinTypes =
{{
  SkinMDPI,
  SkinHDPI,
  SkinXHDPI,
  SkinXXHDPI,
  Skin6Plus
}};

inline const char * SkinSuffix(SkinType s) { return std::get<0>(g_skinInfo[s]); }
inline int SkinSize(SkinType s) { return std::get<1>(g_skinInfo[s]); }
inline bool SkinCoorrectColor(SkinType s) { return std::get<2>(g_skinInfo[s]); }

QString GetSkinGeneratorPath()
{
  QString path = GetExternalPath("skin_generator_tool", "skin_generator_tool.app/Contents/MacOS", "");
  if (path.isEmpty())
    throw std::runtime_error("Can't find skin_generator_tool");
  ASSERT(QFileInfo::exists(path), (path.toStdString()));
  return path;
}

class RAII
{
public:
  RAII(std::function<void()> && f) : m_f(std::move(f)) {}
  ~RAII() { m_f(); }
private:
  function<void()> const m_f;
};

std::string trim(std::string && s)
{
  s.erase(std::remove_if(s.begin(), s.end(), &isspace), s.end());
  return s;
}
}  // namespace

namespace build_style
{
std::unordered_map<string, int> GetSkinSizes(QString const & file)
{
  std::unordered_map<string, int> skinSizes;

  for (SkinType s : g_skinTypes)
    skinSizes.insert(make_pair(SkinSuffix(s), SkinSize(s)));

  try
  {
    std::ifstream ifs(to_string(file));

    std::string line;
    while (std::getline(ifs, line))
    {
      size_t const pos = line.find('=');
      if (pos == std::string::npos)
        continue;

      std::string name(line.begin(), line.begin() + pos);
      std::string valueTxt(line.begin() + pos + 1, line.end());

      name = trim(std::move(name));
      int value = std::stoi(trim(std::move(valueTxt)));

      if (value <= 0)
        continue;

      skinSizes[name] = value;
    }
  }
  catch (std::exception const & e)
  {
    // reset
    for (SkinType s : g_skinTypes)
      skinSizes[SkinSuffix(s)] = SkinSize(s);
  }

  return skinSizes;
}

void BuildSkinImpl(QString const & styleDir, QString const & suffix,
                   int size, bool colorCorrection, QString const & outputDir)
{
  QString const symbolsDir = JoinFoldersToPath({styleDir, "symbols"});

  // Check symbols directory exists
  if (!QDir(symbolsDir).exists())
    throw std::runtime_error("Symbols directory does not exist");

  // Caller ensures that output directory is clear
  if (QDir(outputDir).exists())
    throw std::runtime_error("Output directory is not clear");

  // Create output skin directory
  if (!QDir().mkdir(outputDir))
    throw std::runtime_error("Cannot create output skin directory");

  // Create symbolic link for symbols/png
  QString const pngOriginDir = styleDir + suffix;
  QString const pngDir = JoinFoldersToPath({styleDir, "symbols", "png"});
  QFile::remove(pngDir);
  if (!QFile::link(pngOriginDir, pngDir))
    throw std::runtime_error("Unable to create symbols/png link");
  RAII const cleaner([=]() { QFile::remove(pngDir); });

  // Prepare command line
  QStringList params;
  params << GetSkinGeneratorPath() <<
          "--symbolWidth" << to_string(size).c_str() <<
          "--symbolHeight" << to_string(size).c_str() <<
          "--symbolsDir" << symbolsDir <<
          "--skinName" << JoinFoldersToPath({outputDir, "basic"}) <<
          "--skinSuffix=\"\"";
  if (colorCorrection)
    params << "--colorCorrection true";
  QString const cmd = params.join(' ');

  // Run the script
  auto res = ExecProcess(cmd);

  // If script returns non zero then it is error
  if (res.first != 0)
  {
    QString msg = QString("System error ") + to_string(res.first).c_str();
    if (!res.second.isEmpty())
      msg = msg + "\n" + res.second;
    throw std::runtime_error(to_string(msg));
  }

  // Check files were created
  if (QFile(JoinFoldersToPath({outputDir, "symbols.png"})).size() == 0 ||
      QFile(JoinFoldersToPath({outputDir, "symbols.sdf"})).size() == 0)
  {
    throw std::runtime_error("Skin files have not been created");
  }
}

void BuildSkins(QString const & styleDir, QString const & outputDir)
{
  QString const resolutionFilePath = JoinFoldersToPath({styleDir, "resolutions.txt"});

  auto const resolution2size = GetSkinSizes(resolutionFilePath);

  for (SkinType s : g_skinTypes)
  {
    QString const suffix = SkinSuffix(s);
    QString const outputSkinDir = JoinFoldersToPath({outputDir, "resources-" + suffix + "_design"});
    int const size = resolution2size.at(to_string(suffix)); // SkinSize(s);
    bool const colorCorrection = SkinCoorrectColor(s);

    BuildSkinImpl(styleDir, suffix, size, colorCorrection, outputSkinDir);
  }
}

void ApplySkins(QString const & outputDir)
{
  QString const resourceDir = GetPlatform().ResourcesDir().c_str();

  for (SkinType s : g_skinTypes)
  {
    QString const suffix = SkinSuffix(s);
    QString const outputSkinDir = JoinFoldersToPath({outputDir, "resources-" + suffix + "_design"});
    QString const resourceSkinDir = JoinFoldersToPath({resourceDir, "resources-" + suffix + "_design"});

    if (!QFileInfo::exists(resourceSkinDir) && !QDir().mkdir(resourceSkinDir))
      throw std::runtime_error("Cannot create resource skin directory: " + resourceSkinDir.toStdString());

    if (!CopyFile(JoinFoldersToPath({outputSkinDir, "symbols.png"}),
                  JoinFoldersToPath({resourceSkinDir, "symbols.png"})) ||
        !CopyFile(JoinFoldersToPath({outputSkinDir, "symbols.sdf"}),
                  JoinFoldersToPath({resourceSkinDir, "symbols.sdf"})))
    {
      throw std::runtime_error("Cannot copy skins files");
    }
  }
}
}  // namespace build_style
