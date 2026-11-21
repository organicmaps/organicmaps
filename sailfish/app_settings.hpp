#pragma once

#include <QObject>

class Framework;

namespace sailfish
{
// Applies the map appearance.
class AppSettings : public QObject
{
  Q_OBJECT

public:
  explicit AppSettings(Framework & framework, QObject * parent = nullptr);

  Q_INVOKABLE void applyMapAppearance(bool dark);

private:
  Framework & m_framework;
};
}  // namespace sailfish
