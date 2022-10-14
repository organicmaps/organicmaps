#pragma once

#include "geometry/latlon.hpp"

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>

#include <optional>
#include <string>

class Framework;

namespace qt
{
class RoutingSettings : public QDialog
{
public:
  static void LoadSession(Framework & framework);

  static bool TurnsEnabled();
  static bool UseDebugGuideTrack();
  static std::optional<ms::LatLon> GetCoords(bool start);

  RoutingSettings(QWidget * parent, Framework & framework);
  void ShowModal();

private:
  static std::optional<ms::LatLon> GetCoordsFromString(std::string const & input);

  void ShowMessage(std::string const & message);
  bool ValidateAndSaveCoordsFromInput();
  bool SaveSettings();
  void LoadSettings();

  Framework & m_framework;

  QLineEdit * m_startInput;
  QLineEdit * m_finishInput;

  QComboBox * m_routerType;
  QComboBox * m_routerStrategy;
  QComboBox * m_avoidRoutingOptions;
  QCheckBox * m_showTurnsCheckbox;
  QCheckBox * m_useDebugGuideCheckbox;
  QCheckBox * m_saveSessionCheckbox;
};
}  // namespace qt
