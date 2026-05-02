#pragma once

#include <functional>
#include <string>

#include <QtWidgets/QDialog>

#include "drape/drape_global.hpp"

class Framework;

namespace qt
{
class PreferencesDialog : public QDialog
{
  typedef QDialog base_t;

  Q_OBJECT

public:
  using SetApiVersionFn = std::function<void(dp::ApiVersion)>;

  PreferencesDialog(QWidget * parent, Framework & framework, SetApiVersionFn setApiVersionFn);
};
}  // namespace qt

#ifdef BUILD_DESIGNER
extern std::string const kEnabledAutoRegenGeomIndex;
#endif
