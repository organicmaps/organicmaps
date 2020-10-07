#pragma once

#include "geometry/latlon.hpp"

#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLineEdit>

#include <optional>
#include <string>

class Framework;

namespace qt
{
class RoutingSettings : public QDialog
{
public:
  static bool IsCacheEnabled();
  static bool TurnsEnabled();

  static std::optional<ms::LatLon> GetCoords(bool start);
  static void LoadSettings(Framework & framework);
  static void ResetSettings();

  RoutingSettings(QWidget * parent, Framework & framework);

  void ShowModal();

private:
  static std::string const kShowTurnsSettings;
  static std::string const kUseCachedRoutingSettings;
  static std::string const kStartCoordsCachedSettings;
  static std::string const kFinishCoordsCachedSettings;
  static std::string const kRouterTypeCachedSettings;

  static std::optional<ms::LatLon> GetCoordsFromString(std::string const & input);

  void AddLineEdit(std::string const & title, QLineEdit * lineEdit);
  void AddCheckBox();
  void AddButtonBox();
  void AddListWidgetWithRoutingTypes();

  void ShowMessage(std::string const & message);
  bool ValidateAndSaveCoordsFromInput();
  void SaveSettings();
  void LoadSettings();

  Framework & m_framework;
  QDialog m_dialog;
  QFormLayout m_form;

  QLineEdit * m_startInput;
  QLineEdit * m_finishInput;

  QComboBox * m_routerType;
  QCheckBox * m_showTurnsCheckbox;
  QCheckBox * m_alwaysCheckbox;
};
}  // namespace qt
