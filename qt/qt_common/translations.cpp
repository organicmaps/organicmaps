#include "qt/qt_common/translations.hpp"

#include "platform/preferred_languages.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <QtCore/QLocale>
#include <QtCore/QStringList>
#include <QtGui/QGuiApplication>

namespace qt
{
namespace
{
// The catalog language as QTranslator::language() reports it from Qt 5.15 on, e.g. zh-Hant for zh-hant.
QString CatalogLanguage(QString const & resourceLocale)
{
  auto parts = resourceLocale.split('-');
  for (int i = 1; i < parts.size(); ++i)
    parts[i] = parts[i].size() == 4 ? parts[i].left(1).toUpper() + parts[i].mid(1) : parts[i].toUpper();
  return parts.join('-');
}
}  // namespace

std::unique_ptr<QTranslator> InstallTranslator(QGuiApplication & app, std::span<std::string const> preferred)
{
  QStringList candidates;
  for (auto const & language : preferred)
  {
    // Prefer an exact regional catalog, then let the core parser select the language and Chinese script.
    auto original = QString::fromStdString(language);
    original.replace('_', '-');
    candidates << original.section('.', 0, 0).toLower()
               << QString::fromStdString(languages::GetTwine(language)).toLower();
  }
  candidates << "en";
  candidates.removeDuplicates();

  auto translator = std::make_unique<QTranslator>();
  QString path;
  for (auto const & language : candidates)
  {
    path = QString(":/translations/organicmaps_%1.qm").arg(language);
    if (translator->load(path))
    {
      translator->setObjectName(CatalogLanguage(language));
      break;
    }
    path.clear();
  }
  CHECK(!path.isEmpty(), ("Missing embedded Qt translations, including English"));

  auto * typesTranslator = new QTranslator(translator.get());
  CHECK(
      typesTranslator->load(QString(":/translations/organicmaps_types_%1.qm").arg(translator->objectName().toLower())),
      ("Missing embedded feature type translations"));
  CHECK(app.installTranslator(typesTranslator), ());

  // QObject ownership keeps the standard control translations alive with the app catalog.
  auto * qtTranslator = new QTranslator(translator.get());
  // Older Qt versions report failure when installing a valid empty English catalog.
  if (qtTranslator->load(QLocale(translator->objectName()), "qtbase", "_", ":/translations") &&
      !qtTranslator->isEmpty())
    CHECK(app.installTranslator(qtTranslator), ());

  CHECK(app.installTranslator(translator.get()), ());
  app.setLayoutDirection(QLocale(translator->objectName()).textDirection());
  LOG(LINFO, ("Loaded Qt translation", path.toStdString()));
  return translator;
}

QLocale GetDataSizeLocale(QTranslator const & translator, QLocale const & regionalLocale)
{
  QLocale const catalogLocale(translator.objectName());
  if (regionalLocale.language() == catalogLocale.language() && regionalLocale.script() == catalogLocale.script())
    return regionalLocale;
  return catalogLocale;
}

QString Tr(char const * id, int n)
{
  return qtTrId(id, n);
}
}  // namespace qt
