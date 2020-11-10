#include "qt/routing_settings_dialog.hpp"

#include "map/framework.hpp"

#include "routing/router.hpp"

#include "platform/settings.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/string_utils.hpp"

#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMessageBox>

namespace
{
char constexpr kDelim[] = " ,\t";
}  // namespace

namespace qt
{
std::string const RoutingSettings::kShowTurnsSettings = "show_turns_desktop";
std::string const RoutingSettings::kUseCachedRoutingSettings = "use_cached_settings_desktop";
std::string const RoutingSettings::kStartCoordsCachedSettings = "start_coords_desktop";
std::string const RoutingSettings::kFinishCoordsCachedSettings = "finish_coords_desktop";
std::string const RoutingSettings::kRouterTypeCachedSettings = "router_type_desktop";

// static
bool RoutingSettings::TurnsEnabled()
{
  bool enabled = false;
  if (settings::Get(kShowTurnsSettings, enabled))
    return enabled;

  return false;
}

// static
bool RoutingSettings::IsCacheEnabled()
{
  bool enable = false;
  if (settings::Get(kUseCachedRoutingSettings, enable))
    return enable;

  return false;
}

// static
void RoutingSettings::ResetSettings()
{
  settings::Set(kUseCachedRoutingSettings, false);
  settings::Set(kStartCoordsCachedSettings, std::string());
  settings::Set(kFinishCoordsCachedSettings, std::string());
  settings::Set(kRouterTypeCachedSettings, static_cast<uint32_t>(routing::RouterType::Vehicle));
}

// static
std::optional<ms::LatLon> RoutingSettings::GetCoordsFromString(std::string const & input)
{
  ms::LatLon coords{};
  strings::SimpleTokenizer iter(input, kDelim);
  if (!iter)
    return {};

  if (!strings::to_double(*iter, coords.m_lat))
    return {};

  ++iter;

  if (!strings::to_double(*iter, coords.m_lon))
    return {};

  return {coords};
}

// static
std::optional<ms::LatLon> RoutingSettings::GetCoords(bool start)
{
  std::string input;
  settings::TryGet(start ? kStartCoordsCachedSettings : kFinishCoordsCachedSettings, input);
  return GetCoordsFromString(input);
}

// static
void RoutingSettings::LoadSettings(Framework & framework)
{
  int routerType = 0;
  settings::TryGet(kRouterTypeCachedSettings, routerType);

  framework.GetRoutingManager().SetRouterImpl(static_cast<routing::RouterType>(routerType));
}

RoutingSettings::RoutingSettings(QWidget * parent, Framework & framework)
  : QDialog(parent)
  , m_framework(framework)
  , m_form(this)
  , m_startInput(new QLineEdit(this))
  , m_finishInput(new QLineEdit(this))
  , m_routerType(new QComboBox(this))
  , m_showTurnsCheckbox(new QCheckBox("", this))
  , m_alwaysCheckbox(new QCheckBox("", this))
{
  setWindowTitle("Routing settings");

  AddLineEdit("Start coords (lat, lon)", m_startInput);
  AddLineEdit("Finish coords (lat, lon)", m_finishInput);
  AddListWidgetWithRoutingTypes();
  AddCheckBox();
  AddButtonBox();

  LoadSettings();
}

void RoutingSettings::AddLineEdit(std::string const & title, QLineEdit * lineEdit)
{
  std::string defaultText;
  if (IsCacheEnabled())
  {
    bool const isStart = lineEdit == m_startInput;
    std::string const settingsName = isStart ? kStartCoordsCachedSettings.c_str()
                                             : kFinishCoordsCachedSettings.c_str();
    settings::TryGet(settingsName, defaultText);
  }

  lineEdit->setText(defaultText.c_str());

  QString const label = QString(title.c_str());
  m_form.addRow(label, lineEdit);
}

void RoutingSettings::AddCheckBox()
{
  m_form.addRow("Show turns:", m_showTurnsCheckbox);
  m_form.addRow("Save for next sessions:", m_alwaysCheckbox);
}

void RoutingSettings::AddButtonBox()
{
  auto * buttonBox =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);

  m_form.addRow(buttonBox);

  QObject::connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  QObject::connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void RoutingSettings::AddListWidgetWithRoutingTypes()
{
  using namespace routing;
  m_routerType->insertItem(static_cast<int>(RouterType::Vehicle), "car");
  m_routerType->insertItem(static_cast<int>(RouterType::Pedestrian), "pedestrian");
  m_routerType->insertItem(static_cast<int>(RouterType::Bicycle), "bicycle");
  m_routerType->insertItem(static_cast<int>(RouterType::Taxi), "taxi");
  m_routerType->insertItem(static_cast<int>(RouterType::Transit), "transit");

  m_form.addRow("Choose router:", m_routerType);
}

void RoutingSettings::ShowMessage(std::string const & message)
{
  QMessageBox msgBox;
  msgBox.setText(message.c_str());
  msgBox.exec();
}

bool RoutingSettings::ValidateAndSaveCoordsFromInput()
{
  std::string const & startText = m_startInput->text().toStdString();
  std::string const & finishText = m_finishInput->text().toStdString();

  if (!startText.empty() && !GetCoordsFromString(startText))
    return false;

  if (!finishText.empty() && !GetCoordsFromString(finishText))
    return false;

  settings::Set(kStartCoordsCachedSettings, startText);
  settings::Set(kFinishCoordsCachedSettings, finishText);

  return true;
}

void RoutingSettings::SaveSettings()
{
  settings::Set(kShowTurnsSettings, m_showTurnsCheckbox->checkState() == Qt::CheckState::Checked);
  settings::Set(kUseCachedRoutingSettings, true);
  ValidateAndSaveCoordsFromInput();
  settings::Set(kRouterTypeCachedSettings, m_routerType->currentIndex());
}

void RoutingSettings::LoadSettings()
{
  std::string startCoordsText;
  settings::TryGet(kStartCoordsCachedSettings, startCoordsText);
  m_startInput->setText(startCoordsText.c_str());

  std::string finishCoordsText;
  settings::TryGet(kFinishCoordsCachedSettings, finishCoordsText);
  m_finishInput->setText(finishCoordsText.c_str());

  int routerType = 0;
  settings::TryGet(kRouterTypeCachedSettings, routerType);
  m_routerType->setCurrentIndex(routerType);

  m_framework.GetRoutingManager().SetRouterImpl(static_cast<routing::RouterType>(routerType));

  bool showTurns = false;
  settings::TryGet(kShowTurnsSettings, showTurns);
  m_showTurnsCheckbox->setChecked(showTurns);

  bool setChecked = false;
  settings::TryGet(kUseCachedRoutingSettings, setChecked);
  m_alwaysCheckbox->setChecked(setChecked);
}

void RoutingSettings::ShowModal()
{
  if (exec() != QDialog::Accepted)
    return;

  if (!ValidateAndSaveCoordsFromInput())
  {
    ShowMessage("Bad start or finish coords input.");
    ShowModal();
    return;
  }

  int routerType = m_routerType->currentIndex();
  settings::Set(kRouterTypeCachedSettings, routerType);
  m_framework.GetRoutingManager().SetRouterImpl(
      static_cast<routing::RouterType>(routerType));

  if (m_alwaysCheckbox->isChecked())
    SaveSettings();
  else
    settings::Set(kUseCachedRoutingSettings, false);
}
}  // namespace qt
