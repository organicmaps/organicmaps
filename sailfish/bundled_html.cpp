#include "sailfish/bundled_html.hpp"

#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"

#include "base/assert.hpp"

#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QVariantMap>

#include <array>

namespace sailfish
{
namespace
{
// The choice of the pages' showLanguage() scripts.
QString ChooseLanguage(QSet<QString> const & available)
{
  std::array<char const *, 6> constexpr kCanReadRussian = {"ab", "be", "kk", "ky", "tg", "uz"};
  QString const lang = QString::fromStdString(languages::GetCurrentTwine());
  QString const base = lang.section('-', 0, 0);
  for (auto const * code : kCanReadRussian)
    if (base == QLatin1String(code) && available.contains(QStringLiteral("ru")))
      return QStringLiteral("ru");
  if (available.contains(lang))
    return lang;
  if (available.contains(base))
    return base;
  return QStringLiteral("en");
}

// Drops the elements in other languages: the pages hide every [lang] element but the chosen ones.
QString KeepLanguage(QString const & body)
{
  static QRegularExpression const kLang(QStringLiteral(" lang=\"([^\"]+)\""));
  QSet<QString> available;
  for (auto it = kLang.globalMatch(body); it.hasNext();)
    available.insert(it.next().captured(1));
  QString const lang = ChooseLanguage(available);

  static QRegularExpression const kTag(QStringLiteral("<(/?)([a-zA-Z0-9]+)([^>]*)>"));
  QString result;
  int copiedTo = 0;
  QString skippedTag;
  int depth = 0;
  for (auto it = kTag.globalMatch(body); it.hasNext();)
  {
    auto const tag = it.next();
    bool const closing = !tag.captured(1).isEmpty();
    QString const name = tag.captured(2).toLower();
    if (!skippedTag.isEmpty())
    {
      if (name == skippedTag)
        depth += closing ? -1 : 1;
      if (depth == 0)
      {
        skippedTag.clear();
        copiedTo = tag.capturedEnd();
      }
      continue;
    }
    if (closing)
      continue;
    auto const attributes = kLang.match(tag.captured(3));
    if (attributes.hasMatch() && attributes.captured(1) != lang)
    {
      result += body.midRef(copiedTo, tag.capturedStart() - copiedTo);
      skippedTag = name;
      depth = 1;
    }
  }
  if (skippedTag.isEmpty())
    result += body.midRef(copiedTo);
  return result;
}
}  // namespace

QVariantList BundledHtmlSections(QString const & file)
{
  QFile in(QString::fromStdString(GetPlatform().ResourcesDir()) + file);
  CHECK(in.open(QIODevice::ReadOnly), (file.toStdString()));
  QString const page = QString::fromUtf8(in.readAll());

  // The body without the head's style and script, which rich text can't use.
  static QRegularExpression const kBody(QStringLiteral("<body[^>]*>(.*)</body>"),
                                        QRegularExpression::DotMatchesEverythingOption);
  auto const body = kBody.match(page);
  CHECK(body.hasMatch(), (file.toStdString()));
  QString const html = KeepLanguage(body.captured(1));

  // Rich text can't scroll to an anchor, a list of sections can.
  static QRegularExpression const kAnchor(QStringLiteral("<[a-zA-Z0-9]+[^>]* id=\"([^\"]+)\""));
  QVariantList sections;
  QString id;
  int start = 0;
  for (auto it = kAnchor.globalMatch(html); it.hasNext();)
  {
    auto const anchor = it.next();
    sections.append(QVariantMap{{"id", id}, {"html", html.mid(start, anchor.capturedStart() - start)}});
    id = anchor.captured(1);
    start = anchor.capturedStart();
  }
  sections.append(QVariantMap{{"id", id}, {"html", html.mid(start)}});
  return sections;
}
}  // namespace sailfish
