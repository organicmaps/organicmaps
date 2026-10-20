#pragma once

#include <QString>
#include <QVariantList>

namespace sailfish
{
// A HTML file of the data folder, like faq.html, as {id, html} sections for QML rich text. Only the elements in the
// app's language are kept, as the pages' scripts do in a browser. The sections start at the anchor targets.
QVariantList BundledHtmlSections(QString const & file);
}  // namespace sailfish
