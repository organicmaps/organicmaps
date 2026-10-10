#pragma once

#include <QDate>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>

namespace sailfish
{
class AppInfo : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString version READ version CONSTANT)
  Q_PROPERTY(QDate dataVersion READ dataVersion CONSTANT)
  Q_PROPERTY(QStringList bookmarkColors READ bookmarkColors CONSTANT)
  // organicmaps, or harbour-organicmaps in the Harbour package.
  Q_PROPERTY(QString name READ name CONSTANT)
  // The Harbour package has no voice instructions.
  Q_PROPERTY(bool voiceSupported READ voiceSupported CONSTANT)

public:
  using QObject::QObject;

  QString version() const;
  QDate dataVersion() const;
  QStringList bookmarkColors() const;
  QString name() const { return QStringLiteral(SAILFISH_APP_NAME); }
  bool voiceSupported() const;

  Q_INVOKABLE QString localized(QString const & key, QStringList const & args = {}) const;
  Q_INVOKABLE QString formatSize(qint64 bytes) const;
  // See BundledHtmlSections().
  Q_INVOKABLE QVariantList bundledHtml(QString const & file) const;
};
}  // namespace sailfish
