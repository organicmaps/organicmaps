#include "qt/routing_settings_dialog.hpp"

#include "map/framework.hpp"

#include "routing/router.hpp"

#include "platform/settings.hpp"

#include "base/string_utils.hpp"

#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QMessageBox>

namespace qt
{
namespace
{
char constexpr kDelim[] = " ,\t";
std::string const kShowTurnsSettings = "show_turns_desktop";
std::string const kUseDebugGuideSettings = "use_debug_guide_desktop";
std::string const kUseCachedRoutingSettings = "use_cached_settings_desktop";
std::string const kStartCoordsCachedSettings = "start_coords_desktop";
std::string const kFinishCoordsCachedSettings = "finish_coords_desktop";
std::string const kRouterTypeCachedSettings = "router_type_desktop";

int constexpr DefaultRouterIndex()
{
  // Car router by default.
  return static_cast<int>(routing::RouterType::Vehicle);
}
}  // namespace

// static
bool RoutingSettings::TurnsEnabled()
{
  bool enabled = false;
  if (settings::Get(kShowTurnsSettings, enabled))
    return enabled;

  return false;
}

// static
bool RoutingSettings::UseDebugGuideTrack()
{
  bool enabled = false;
  if (settings::Get(kUseDebugGuideSettings, enabled))
    return enabled;

  return false;
}

// static
std::optional<ms::LatLon> RoutingSettings::GetCoordsFromString(std::string const & input)
{
  ms::LatLon coords;
  strings::SimpleTokenizer iter(input, kDelim);
  if (iter && strings::to_double(*iter, coords.m_lat))
  {
    ++iter;
    if (iter && strings::to_double(*iter, coords.m_lon))
      return coords;
  }
  return {};
}

// static
std::optional<ms::LatLon> RoutingSettings::GetCoords(bool start)
{
  std::string input;
  settings::TryGet(start ? kStartCoordsCachedSettings : kFinishCoordsCachedSettings, input);
  return GetCoordsFromString(input);
}

// static
void RoutingSettings::LoadSession(Framework & framework)
{
  bool enable = false;
  settings::TryGet(kUseCachedRoutingSettings, enable);
  if (enable)
  {
    int routerType = DefaultRouterIndex();
    settings::TryGet(kRouterTypeCachedSettings, routerType);
    framework.GetRoutingManager().SetRouter(static_cast<routing::RouterType>(routerType));
  }
  else
  {
    settings::Delete(kUseCachedRoutingSettings);
    settings::Delete(kShowTurnsSettings);
    settings::Delete(kUseDebugGuideSettings);
    settings::Delete(kStartCoordsCachedSettings);
    settings::Delete(kFinishCoordsCachedSettings);
    settings::Delete(kRouterTypeCachedSettings);
  }
}

RoutingSettings::RoutingSettings(QWidget * parent, Framework & framework) : QDialog(parent), m_framework(framework)
{
  setWindowTitle("Routing settings");
  QVBoxLayout * layout = new QVBoxLayout(this);

  {
    QFrame * frame = new QFrame(this);
    frame->setFrameShape(QFrame::Box);
    QFormLayout * form = new QFormLayout(frame);

    auto const addCoordLine = [&](char const * title, bool isStart)
    {
      QLineEdit * lineEdit = new QLineEdit(frame);
      if (isStart)
        m_startInput = lineEdit;
      else
        m_finishInput = lineEdit;

      std::string defaultText;
      settings::TryGet(isStart ? kStartCoordsCachedSettings : kFinishCoordsCachedSettings, defaultText);

      lineEdit->setText(defaultText.c_str());
      form->addRow(title, lineEdit);
    };
    addCoordLine("Start coords (lat, lon)", true);
    addCoordLine("Finish coords (lat, lon)", false);

    using namespace routing;
    m_routerType = new QComboBox(frame);
    m_routerType->insertItem(static_cast<int>(RouterType::Vehicle), "car");
    m_routerType->insertItem(static_cast<int>(RouterType::Pedestrian), "pedestrian");
    m_routerType->insertItem(static_cast<int>(RouterType::Bicycle), "bicycle");
    m_routerType->insertItem(static_cast<int>(RouterType::Transit), "transit");
    m_routerType->insertItem(static_cast<int>(RouterType::Ruler), "ruler");
    form->addRow("Choose router:", m_routerType);

    m_showTurnsCheckbox = new QCheckBox({}, frame);
    form->addRow("Show turns:", m_showTurnsCheckbox);

    m_useDebugGuideCheckbox = new QCheckBox({}, frame);
    form->addRow("Use debug guide track:", m_useDebugGuideCheckbox);

    layout->addWidget(frame);
  }

  {
    m_saveSessionCheckbox = new QCheckBox("Save these settings for the next app session", this);
    layout->addWidget(m_saveSessionCheckbox);
  }

  {
    auto * buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    layout->addWidget(buttonBox);
  }

  LoadSettings();
}

void RoutingSettings::ShowMessage(std::string const & message)
{
  QMessageBox msgBox;
  msgBox.setText(message.c_str());
  msgBox.exec();
}

bool RoutingSettings::ValidateAndSaveCoordsFromInput()
{
  std::string startText = m_startInput->text().toStdString();
  strings::Trim(startText);
  std::string finishText = m_finishInput->text().toStdString();
  strings::Trim(finishText);

  if (!startText.empty() && !GetCoordsFromString(startText))
    return false;

  if (!finishText.empty() && !GetCoordsFromString(finishText))
    return false;

  settings::Set(kStartCoordsCachedSettings, startText);
  settings::Set(kFinishCoordsCachedSettings, finishText);
  return true;
}

bool RoutingSettings::SaveSettings()
{
  settings::Set(kShowTurnsSettings, m_showTurnsCheckbox->isChecked());
  settings::Set(kUseDebugGuideSettings, m_useDebugGuideCheckbox->isChecked());
  settings::Set(kUseCachedRoutingSettings, m_saveSessionCheckbox->isChecked());
  settings::Set(kRouterTypeCachedSettings, m_routerType->currentIndex());
  return ValidateAndSaveCoordsFromInput();
}

void RoutingSettings::LoadSettings()
{
  std::string startCoordsText;
  settings::TryGet(kStartCoordsCachedSettings, startCoordsText);
  m_startInput->setText(startCoordsText.c_str());

  std::string finishCoordsText;
  settings::TryGet(kFinishCoordsCachedSettings, finishCoordsText);
  m_finishInput->setText(finishCoordsText.c_str());

  int routerType = DefaultRouterIndex();
  settings::TryGet(kRouterTypeCachedSettings, routerType);
  m_routerType->setCurrentIndex(routerType);

  bool showTurns = false;
  settings::TryGet(kShowTurnsSettings, showTurns);
  m_showTurnsCheckbox->setChecked(showTurns);

  bool useGuides = false;
  settings::TryGet(kUseDebugGuideSettings, useGuides);
  m_useDebugGuideCheckbox->setChecked(useGuides);

  bool saveSession = false;
  settings::TryGet(kUseCachedRoutingSettings, saveSession);
  m_saveSessionCheckbox->setChecked(saveSession);
}

void RoutingSettings::ShowModal()
{
  if (exec() != QDialog::Accepted)
    return;

  if (!SaveSettings())
  {
    ShowMessage("Bad start or finish coords input.");
    ShowModal();
    return;
  }

  int const routerType = m_routerType->currentIndex();
  m_framework.GetRoutingManager().SetRouter(static_cast<routing::RouterType>(routerType));
}
}  // namespace qt
