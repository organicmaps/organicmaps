#include "qt/qt_common/translations.hpp"

#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"

#include "base/logging.hpp"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QTranslator>

#include <algorithm>
#include <cctype>
#include <vector>

namespace qt
{
namespace
{
std::string NormalizeLocale(std::string locale)
{
  std::replace(locale.begin(), locale.end(), '_', '-');
  return locale;
}

std::string ToLowerAscii(std::string value)
{
  for (char & c : value)
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return value;
}

std::string GetBaseLocale(std::string locale)
{
  auto const pos = locale.find_first_of("-_ ");
  if (pos != std::string::npos)
    locale.resize(pos);
  return NormalizeLocale(std::move(locale));
}

std::string GetTwineLocale(std::string const & locale)
{
  auto const lower = ToLowerAscii(locale);
  if (lower.starts_with("zh"))
  {
    for (char const * s : {"hant", "tw", "hk", "mo"})
      if (lower.find(s) != std::string::npos)
        return "zh-Hant";
    return "zh-Hans";
  }

  return GetBaseLocale(locale);
}

void AddLocale(std::vector<std::string> & locales, std::string locale)
{
  locale = NormalizeLocale(std::move(locale));
  if (!locale.empty() && std::find(locales.begin(), locales.end(), locale) == locales.end())
    locales.emplace_back(std::move(locale));
}

std::vector<std::string> GetLocaleCandidates(std::string const & locale)
{
  std::vector<std::string> locales;
  AddLocale(locales, locale);
  AddLocale(locales, GetTwineLocale(locale));
  AddLocale(locales, languages::GetCurrentOrig());
  AddLocale(locales, languages::GetCurrentTwine());
  AddLocale(locales, "en");
  return locales;
}

QStringList GetTranslationDirs()
{
  QString const appDir = QCoreApplication::applicationDirPath();

  return {
      QDir(appDir).filePath("translations"),
      QDir(appDir).filePath("../Resources/translations"),
      QDir(QString::fromStdString(GetPlatform().ResourcesDir())).filePath("translations"),
  };
}
}  // namespace

std::unique_ptr<QTranslator> InstallTranslator(QCoreApplication & app, std::string const & locale)
{
  auto translator = std::make_unique<QTranslator>();
  for (auto const & language : GetLocaleCandidates(locale))
  {
    QString const fileName = QString("omim_%1.qm").arg(QString::fromStdString(language));
    for (QString const & dir : GetTranslationDirs())
    {
      QString const path = QDir(dir).filePath(fileName);
      if (translator->load(path))
      {
        app.installTranslator(translator.get());
        LOG(LINFO, ("Loaded Qt translation", path.toStdString()));
        return translator;
      }
    }
  }

  LOG(LWARNING, ("Failed to load Qt translation for locale", locale));
  return {};
}

QString Tr(char const * id, int n)
{
  return qtTrId(id, n);
}
}  // namespace qt
