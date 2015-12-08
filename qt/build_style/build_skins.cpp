#include "build_skins.h"
#include "build_common.h"

#include "platform/platform.hpp"

#include "std/array.hpp"
#include "std/algorithm.hpp"
#include "std/exception.hpp"
#include "std/tuple.hpp"
#include "std/unordered_map.hpp"
#include "std/fstream.hpp"

#include <QDir>

namespace
{

enum SkinType
{
  SkinYota,
  SkinLDPI,
  SkinMDPI,
  SkinHDPI,
  SkinXHDPI,
  SkinXXHDPI,
  Skin6Plus,

  // SkinCount MUST BE last
  SkinCount
};

typedef tuple<const char*, int, bool> SkinInfo;
const SkinInfo g_skinInfo[SkinCount] =
{
  make_tuple("yota", 19, true),
  make_tuple("ldpi", 18, false),
  make_tuple("mdpi", 18, false),
  make_tuple("hdpi", 27, false),
  make_tuple("xhdpi", 36, false),
  make_tuple("xxhdpi", 54, false),
  make_tuple("6plus", 54, false),
};

const array<SkinType, SkinCount> g_skinTypes =
{{
  SkinYota,
  SkinLDPI,
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
  QString const resourceDir = GetPlatform().ResourcesDir().c_str();
  return resourceDir + "skin_generator.app/Contents/MacOS/skin_generator";
}

class RAII
{
public:
  RAII(function<void()> && f) : m_f(move(f)) {}
  ~RAII() { m_f(); }
private:
  function<void()> const m_f;
};

string trim(string && s)
{
  s.erase(remove_if(s.begin(), s.end(), &isspace), s.end());
  return s;
}

}  // namespace

namespace build_style
{

unordered_map<string, int> GetSkinSizes(QString const & file)
{
  unordered_map<string, int> skinSizes;

  for (SkinType s : g_skinTypes)
    skinSizes.insert(make_pair(SkinSuffix(s), SkinSize(s)));

  try
  {
    ifstream ifs(to_string(file));

    string line;
    while (std::getline(ifs, line))
    {
      size_t const pos = line.find('=');
      if (pos == string::npos)
        continue;

      string name(line.begin(), line.begin() + pos);
      string valueTxt(line.begin() + pos + 1, line.end());

      name = trim(move(name));
      int value = std::stoi(trim(move(valueTxt)));

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

void BuildSkinImpl(QString const & styleDir, QString const & suffix, int size, bool colorCorrection,
                 QString const & outputDir)
{
  QString const symbolsDir = styleDir + "symbols/";

  // Check symbols directory exists
  if (!QDir(symbolsDir).exists())
    throw runtime_error("Symbols directory does not exist");

  // Caller ensures that output directory is clear
  if (QDir(outputDir).exists())
    throw runtime_error("Output directory is not clear");

  // Create output skin directory
  if (!QDir().mkdir(outputDir))
    throw runtime_error("Cannot create output skin directory");

  // Create symbolic link for symbols/png
  QString const pngOriginDir = styleDir + suffix;
  QString const pngDir = styleDir + "symbols/png";
  QFile::remove(pngDir);
  if (!QFile::link(pngOriginDir, pngDir))
    throw runtime_error("Unable to create symbols/png link");
  RAII const cleaner([=]() { QFile::remove(pngDir); });

  // Prepare command line
  QStringList params;
  params << GetSkinGeneratorPath() <<
          "--symbolWidth" << to_string(size).c_str() <<
          "--symbolHeight" << to_string(size).c_str() <<
          "--symbolsDir" << symbolsDir <<
          "--skinName" << outputDir + "basic" <<
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
    throw runtime_error(to_string(msg));
  }

  // Check files were created
  if (QFile(outputDir + "symbols.png").size() == 0 ||
      QFile(outputDir + "symbols.sdf").size() == 0)
  {
    throw runtime_error("Skin files have not been created");
  }
}

void BuildSkins(QString const & styleDir, QString const & outputDir)
{
  QString const resolutionFilePath = styleDir + "resolutions.txt";

  unordered_map<string, int> const resolution2size = GetSkinSizes(resolutionFilePath);

  for (SkinType s : g_skinTypes)
  {
    QString const suffix = SkinSuffix(s);
    QString const outputSkinDir = outputDir + "resources-" + suffix + "/";
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
    QString const outputSkinDir = outputDir + "resources-" + suffix + "/";
    QString const resourceSkinDir = resourceDir + "resources-" + suffix + "/";

    if (!QDir(resourceSkinDir).exists() && !QDir().mkdir(resourceSkinDir))
      throw runtime_error("Cannot create resource skin directory");

    if (!CopyFile(outputSkinDir + "symbols.png", resourceSkinDir + "symbols.png") ||
        !CopyFile(outputSkinDir + "symbols.sdf", resourceSkinDir + "symbols.sdf"))
    {
      throw runtime_error("Cannot copy skins files");
    }
  }
}

}  // namespace build_style
