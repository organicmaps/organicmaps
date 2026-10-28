#include "testing/testing.hpp"

#include "qt/place_page_dialog_common.hpp"

#include <QtCore/QUrl>
#include <QtGui/QGuiApplication>
#include <QtGui/QTextBlock>
#include <QtGui/QTextDocument>
#include <QtGui/QTextFragment>

namespace place_page_dialog_common_tests
{
void EnsureApp()
{
  static int argc = 0;
  static QGuiApplication app(argc, nullptr);
}

UNIT_TEST(PlacePage_LinkKeepsTargetAndEscapesDisplayedText)
{
  EnsureApp();
  QString const url = "https://example.org/route?a=1&b=2";
  QString const text = QString::fromUtf8("example.org/<b> & café");
  QTextDocument document;
  document.setHtml(place_page_dialog::MakeHtmlLink(url, text));
  TEST_EQUAL(document.toPlainText().toStdString(), text.toStdString(), ());
  auto const target = document.begin().begin().fragment().charFormat().anchorHref();
  TEST_EQUAL(target.toStdString(), url.toStdString(), ());
  TEST(!QUrl(target).isRelative(), (target.toStdString()));
}

UNIT_TEST(PlacePage_DescriptionKeepsCompleteUnicodeCharacters)
{
  using place_page_dialog::ShortDescription;
  TEST_EQUAL(ShortDescription("<p>First paragraph</p><p>Second</p>").toStdString(), "First paragraph", ());
  TEST(ShortDescription("").isEmpty(), ());
  auto const shortText = QString(200, QChar(u'é'));
  TEST_EQUAL(ShortDescription(shortText.toStdString()).toStdString(), shortText.toStdString(), ());

  auto const accented = ShortDescription(QString(300, QChar(u'é')).toStdString());
  TEST_EQUAL(accented.toStdString(), (QString(248, QChar(u'é')) + QStringLiteral("…")).toStdString(), ());
  TEST(!accented.contains(QChar::ReplacementCharacter), ());

  auto const supplementary = ShortDescription(std::string(496, 'x') + "😀" + std::string(20, 'y'));
  TEST_EQUAL(supplementary.toStdString(), std::string(496, 'x') + "…", ());
  TEST_LESS_OR_EQUAL(supplementary.toUtf8().size(), 500, ());
  TEST(!supplementary.contains(QChar::ReplacementCharacter), ());
}
}  // namespace place_page_dialog_common_tests
