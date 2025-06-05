#pragma once

#include <string>

#include <QtWidgets/QDialog>

class Framework;

namespace qt
{
class PreferencesDialog : public QDialog
{
  typedef QDialog base_t;

  Q_OBJECT

public:
  PreferencesDialog(QWidget * parent, Framework & framework);
};
}  // namespace qt

#ifdef BUILD_DESIGNER
extern std::string const kEnabledAutoRegenGeomIndex;
#endif
