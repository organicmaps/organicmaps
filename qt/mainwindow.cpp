#include "qt/mainwindow.hpp"
#include "qt/about.hpp"
#include "qt/bookmark_dialog.hpp"
#include "qt/dev_search_panel.hpp"
#include "qt/draw_widget.hpp"
#include "qt/mwms_borders_selection.hpp"
#include "qt/osm_auth_dialog.hpp"
#include "qt/popup_menu_holder.hpp"
#include "qt/preferences_dialog.hpp"
#include "qt/qt_common/helpers.hpp"
#include "qt/qt_common/scale_slider.hpp"
#include "qt/routing_settings_dialog.hpp"
#include "qt/screenshoter.hpp"
#include "qt/user_search_panel/user_search_panel.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "base/assert.hpp"

#include <functional>
#include <sstream>

#include "std/target_os.hpp"

#ifdef BUILD_DESIGNER
#include "build_style/build_common.h"
#include "build_style/build_phone_pack.h"
#include "build_style/build_statistics.h"
#include "build_style/build_style.h"
#include "build_style/run_tests.h"

#include "drape_frontend/debug_rect_renderer.hpp"

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#endif  // BUILD_DESIGNER

#include <QtGui/QCloseEvent>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>

#ifndef NO_DOWNLOADER
#include "qt/info_dialog.hpp"
#include "qt/update_dialog.hpp"
#endif  // NO_DOWNLOADER

namespace qt
{
namespace
{
void FormatMapSize(uint64_t sizeInBytes, std::string & units, size_t & sizeToDownload)
{
  int constexpr mbInBytes = 1024 * 1024;
  int constexpr kbInBytes = 1024;
  if (sizeInBytes > mbInBytes)
  {
    sizeToDownload = (sizeInBytes + mbInBytes - 1) / mbInBytes;
    units = "MB";
  }
  else if (sizeInBytes > kbInBytes)
  {
    sizeToDownload = (sizeInBytes + kbInBytes - 1) / kbInBytes;
    units = "KB";
  }
  else
  {
    sizeToDownload = sizeInBytes;
    units = "B";
  }
}

template <class T>
T * CreateBlackControl(QString const & name)
{
  T * p = new T(name);
  p->setStyleSheet("color: black;");
  return p;
}

}  // namespace

// Defined in osm_auth_dialog.cpp.
extern char const * kOauthTokenSetting;

MainWindow::MainWindow(Framework & framework, std::unique_ptr<ScreenshotParams> && screenshotParams,
                       QRect const & screenGeometry
#ifdef BUILD_DESIGNER
                       ,
                       QString const & mapcssFilePath
#endif
                       )
  : m_locationService(CreateDesktopLocationService(*this))
  , m_screenshotMode(screenshotParams != nullptr)
#ifdef BUILD_DESIGNER
  , m_mapcssFilePath(mapcssFilePath)
#endif
{
  setGeometry(screenGeometry);

  // Context Menus of QMainWindow allow showing/hiding toolbars. By hiding the main toolbar, the
  // user could lock themselves out of the UI.
  setContextMenuPolicy(Qt::NoContextMenu);

  if (m_screenshotMode)
  {
    screenshotParams->m_statusChangedFn = [this](std::string const & state, bool finished)
    {
      statusBar()->showMessage(QString::fromStdString(state));
      if (finished)
        QCoreApplication::quit();
    };
  }

  int const width = m_screenshotMode ? static_cast<int>(screenshotParams->m_width) : 0;
  int const height = m_screenshotMode ? static_cast<int>(screenshotParams->m_height) : 0;
  m_pDrawWidget = new DrawWidget(framework, std::move(screenshotParams), this);

  setCentralWidget(m_pDrawWidget);

  if (m_screenshotMode)
  {
    m_pDrawWidget->setFixedSize(width, height);
    setFixedSize(width, height + statusBar()->height());
  }

  connect(m_pDrawWidget, SIGNAL(BeforeEngineCreation()), this, SLOT(OnBeforeEngineCreation()));

  BindHotkeys();
  CreateCountryStatusControls();
  CreateMainToolBar();
  CreateDevToolBar();
  CreateUserSearchPanel();
  CreateDevSearchPanel();

  connect(qApp, &QApplication::focusChanged, this, &MainWindow::OnFocusChange);
  connect(m_userSearchDockWidget, &QDockWidget::visibilityChanged, this,
          &MainWindow::OnSearchDockWidgetVisibilityChange);

  QString caption = QCoreApplication::applicationName();

#ifdef BUILD_DESIGNER
  if (!m_mapcssFilePath.isEmpty())
    caption += QString(" - ") + m_mapcssFilePath;
#endif

  setWindowTitle(caption);
  // TODO (@zagto): set on QApplication?
  setWindowIcon(QIcon(":/ui/logo.png"));

  // TODO (@zagto): use the system menu bar on macOS
  QMenu * mainMenu = new QMenu(this);
#ifndef NO_DOWNLOADER
  // TODO (@zagto): consistent style regarding hotkey and icons
  mainMenu->addAction(QIcon::fromTheme("download"), tr("Download Maps"), this, SLOT(ShowUpdateDialog()));
#endif  // NO_DOWNLOADER
  mainMenu->addAction(tr("OpenStreetMap Login"), QKeySequence(Qt::CTRL | Qt::Key_O), this, SLOT(OnLoginMenuItem()));
  mainMenu->addAction(tr("Upload Edits"), QKeySequence(Qt::CTRL | Qt::Key_U), this, SLOT(OnUploadEditsMenuItem()));
  mainMenu->addAction(tr("Preferences"), QKeySequence(Qt::CTRL | Qt::Key_P), this, SLOT(OnPreferences()));
  mainMenu->addAction(tr("About"), QKeySequence(Qt::Key_F1), this, SLOT(OnAbout()));
  mainMenu->addAction(tr("Exit"), QKeySequence(Qt::CTRL | Qt::Key_Q), this, SLOT(close()));
  m_mainMenuButton->setMenu(mainMenu);

  // Always show on full screen.
  showMaximized();

#ifndef NO_DOWNLOADER
  // Show intro dialog if necessary
  bool bShow = true;
  std::string const showWelcome = "ShowWelcome";
  settings::TryGet(showWelcome, bShow);

  if (bShow)
  {
    bool bShowUpdateDialog = true;

    std::string text;
    try
    {
      ReaderPtr<Reader> reader = GetPlatform().GetReader("welcome.html");
      reader.ReadAsString(text);
    }
    catch (...)
    {}

    if (!text.empty())
    {
      InfoDialog welcomeDlg(QString("Welcome to ") + caption, text.c_str(), this, QStringList(tr("Download Maps")));
      if (welcomeDlg.exec() == QDialog::Rejected)
        bShowUpdateDialog = false;
    }
    settings::Set("ShowWelcome", false);

    if (bShowUpdateDialog)
      ShowUpdateDialog();
  }
#endif  // NO_DOWNLOADER

  m_pDrawWidget->UpdateAfterSettingsChanged();

  bool developerMode = false;
  OnDeveloperModeChanged(settings::Get(settings::kDeveloperMode, developerMode) && developerMode);
  OnSearchDockWidgetWidthChange(UserSearchPanel::kDefaultWidth);

  RoutingSettings::LoadSession(m_pDrawWidget->GetFramework());
}

void MainWindow::LocationStateModeChanged(location::EMyPositionMode mode)
{
  if (mode == location::PendingPosition)
  {
    m_locationService->Start();
    m_pMyPositionAction->setIcon(QIcon(":/navig64/location-search.png"));
    m_pMyPositionAction->setToolTip(tr("Looking for position..."));
    return;
  }

  m_pMyPositionAction->setIcon(QIcon(":/navig64/location.png"));
  m_pMyPositionAction->setToolTip(tr("My Position"));
}

void MainWindow::BindHotkeys()
{
  // Add navigation hot keys.
  m_pDrawWidget->BindHotkeys(*this);

  qt::common::Hotkey const hotkeys[] = {{Qt::Key_A, SLOT(ShowAll())},
                                        // Use CMD+n (New Item hotkey) to activate Create Feature mode.
                                        {Qt::Key_Escape, SLOT(ChoosePositionModeDisable())}};
  for (auto const & hotkey : hotkeys)
  {
    QAction * pAct = new QAction(this);
    pAct->setShortcut(QKeySequence(hotkey.m_key));
    connect(pAct, SIGNAL(triggered()), m_pDrawWidget, hotkey.m_slot);
    addAction(pAct);
  }
}

void MainWindow::CreateMainToolBar()
{
  auto mainToolBar = new QToolBar;
  mainToolBar->setMovable(false);

  m_userSearchQueryEdit = new QLineEdit;
  m_userSearchQueryEdit->addAction(QIcon(":/navig64/search.png"), QLineEdit::LeadingPosition);
  m_userSearchCloseAction = m_userSearchQueryEdit->addAction(QIcon(":/ui/x.png"), QLineEdit::TrailingPosition);
  m_userSearchQueryEdit->setPlaceholderText(tr("Search"));
  m_userSearchCloseAction->setVisible(false);
  mainToolBar->addWidget(m_userSearchQueryEdit);
  m_userSearchCloseAction->setVisible(false);

  qt::common::ScaleSlider::Embed(Qt::Horizontal, *mainToolBar, *m_pDrawWidget);

  // add my position button with "checked" behavior
  m_pMyPositionAction =
      mainToolBar->addAction(QIcon(":/navig64/location.png"), tr("My Position"), this, &MainWindow::OnMyPosition);
  m_pMyPositionAction->setCheckable(true);

  QWidget * spacer = new QWidget();
  spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  mainToolBar->addWidget(spacer);

  auto bookmarksButton = new QToolButton(mainToolBar);
  bookmarksButton->setIcon(QIcon(":/navig64/bookmark.png"));
  bookmarksButton->setText(tr("Bookmarks"));
  bookmarksButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  connect(bookmarksButton, &QToolButton::clicked, this, &MainWindow::OnBookmarksAction);
  mainToolBar->addWidget(bookmarksButton);

  m_layers = new PopupMenuHolder(this);

  /// @todo Uncomment when we will integrate a traffic provider.
  // m_layers->addAction(QIcon(":/navig64/traffic.png"), tr("Traffic"),
  //                     std::bind(&MainWindow::OnLayerEnabled, this, TRAFFIC), true);
  // m_layers->setChecked(TRAFFIC, Framework::LoadTrafficEnabled());

  m_layers->addAction(QIcon(":/navig64/subway.png"), tr("Public transport"),
                      std::bind(&MainWindow::OnLayerEnabled, this, TRANSIT), true);
  m_layers->setChecked(TRANSIT, Framework::LoadTransitSchemeEnabled());

  m_layers->addAction(QIcon(":/navig64/isolines.png"), tr("Isolines"),
                      std::bind(&MainWindow::OnLayerEnabled, this, ISOLINES), true);
  m_layers->setChecked(ISOLINES, Framework::LoadIsolinesEnabled());
  // TODO(AB): Are icons drawable? Fix and make different icons for different layers.
  m_layers->addAction(QIcon(":/navig64/isolines.png"), tr("Outdoors"),
                      std::bind(&MainWindow::OnLayerEnabled, this, OUTDOORS), true);
  m_layers->setChecked(OUTDOORS, Framework::LoadOutdoorsEnabled());

  m_layers->addAction(QIcon(":/navig64/isolines.png"), tr("Hiking"),
                      std::bind(&MainWindow::OnLayerEnabled, this, HIKING), true);
  m_layers->setChecked(HIKING, Framework::IsHikingEnabled());

  m_layers->addAction(QIcon(":/navig64/isolines.png"), tr("Cycling"),
                      std::bind(&MainWindow::OnLayerEnabled, this, CYCLING), true);
  m_layers->setChecked(CYCLING, Framework::IsCyclingEnabled());

  auto layersButton = m_layers->create();
  m_layers->setMainIcon(QIcon(":/navig64/layers.png"));
  layersButton->setText(tr("Layers"));
  layersButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  mainToolBar->addWidget(layersButton);

  m_mainMenuButton = new QToolButton;
  m_mainMenuButton->setIcon(QIcon(":/navig64/hamburger.png"));
  m_mainMenuButton->setStyleSheet("QToolButton::menu-indicator { image: none; }");
  connect(m_mainMenuButton, &QToolButton::clicked, m_mainMenuButton, &QToolButton::showMenu);
  mainToolBar->addWidget(m_mainMenuButton);

  addToolBar(mainToolBar);
  if (m_screenshotMode)
    mainToolBar->setVisible(false);
}

void MainWindow::CreateDevToolBar()
{
  m_devToolBar = new QToolBar(this);
  m_devToolBar->setIconSize(QSize(32, 32));

#ifndef BUILD_DESIGNER
  m_routing = new PopupMenuHolder(this);

  // The order should be the same as in "enum class RouteMarkType".
  m_routing->addAction(QIcon(":/navig64/point-start.png"), tr("Start point"),
                       std::bind(&MainWindow::OnRoutePointSelected, this, RouteMarkType::Start), false);
  m_routing->addAction(QIcon(":/navig64/point-intermediate.png"), tr("Intermediate point"),
                       std::bind(&MainWindow::OnRoutePointSelected, this, RouteMarkType::Intermediate), false);
  m_routing->addAction(QIcon(":/navig64/point-finish.png"), tr("Finish point"),
                       std::bind(&MainWindow::OnRoutePointSelected, this, RouteMarkType::Finish), false);

  QToolButton * toolBtn = m_routing->create();
  toolBtn->setToolTip(tr("Select mode and use SHIFT + LMB to set point"));
  m_devToolBar->addWidget(toolBtn);
  m_routing->setCurrent(m_pDrawWidget->GetRoutePointAddMode());

  QAction * act =
      m_devToolBar->addAction(QIcon(":/navig64/routing.png"), tr("Follow route"), this, SLOT(OnFollowRoute()));
  act->setToolTip(tr("Build route and use ALT + LMB to emulate current position"));
  m_devToolBar->addAction(QIcon(":/navig64/clear-route.png"), tr("Clear route"), this, SLOT(OnClearRoute()));
  m_devToolBar->addAction(QIcon(":/navig64/settings-routing.png"), tr("Routing settings"), this,
                          SLOT(OnRoutingSettings()));

  m_devToolBar->addSeparator();

  m_pCreateFeatureAction = m_devToolBar->addAction(QIcon(":/navig64/select.png"), tr("Create Feature"), this,
                                                   SLOT(OnCreateFeatureClicked()));
  m_pCreateFeatureAction->setCheckable(true);
  m_pCreateFeatureAction->setToolTip(tr("Push to select position, next push to create Feature"));
  m_pCreateFeatureAction->setShortcut(QKeySequence::New);

  m_devToolBar->addSeparator();

  m_selection = new PopupMenuHolder(this);

  // The order should be the same as in "enum class SelectionMode".
  m_selection->addAction(QIcon(":/navig64/selectmode.png"), tr("Roads selection mode"),
                         std::bind(&MainWindow::OnSwitchSelectionMode, this, SelectionMode::Features), true);
  m_selection->addAction(QIcon(":/navig64/city_boundaries.png"), tr("City boundaries selection mode"),
                         std::bind(&MainWindow::OnSwitchSelectionMode, this, SelectionMode::CityBoundaries), true);
  m_selection->addAction(QIcon(":/navig64/city_roads.png"), tr("City roads selection mode"),
                         std::bind(&MainWindow::OnSwitchSelectionMode, this, SelectionMode::CityRoads), true);
  m_selection->addAction(QIcon(":/navig64/test.png"), tr("Cross MWM segments selection mode"),
                         std::bind(&MainWindow::OnSwitchSelectionMode, this, SelectionMode::CrossMwmSegments), true);
  m_selection->addAction(QIcon(":/navig64/borders_selection.png"), tr("MWMs borders selection mode"), this,
                         SLOT(OnSwitchMwmsBordersSelectionMode()), true);

  toolBtn = m_selection->create();
  toolBtn->setToolTip(tr("Select mode and use RMB to define selection box"));
  m_devToolBar->addWidget(toolBtn);

  m_devToolBar->addAction(QIcon(":/navig64/clear.png"), tr("Clear selection"), this, SLOT(OnClearSelection()));

  m_devToolBar->addSeparator();

#endif  // NOT BUILD_DESIGNER

  // Add search button with "checked" behavior.
  m_devSearchAction =
      m_devToolBar->addAction(QIcon(":/navig64/search.png"), tr("Search"), this, SLOT(OnSearchButtonClicked()));
  m_devSearchAction->setCheckable(true);
  m_devSearchAction->setShortcut(QKeySequence::Find);
  // TODO (@zagto): show new search panel from this shortcut

  m_rulerAction = m_devToolBar->addAction(QIcon(":/navig64/ruler.png"), tr("Ruler"), this, SLOT(OnRulerEnabled()));
  m_rulerAction->setToolTip(tr("Check this button and use ALT + LMB to set points"));
  m_rulerAction->setCheckable(true);
  m_rulerAction->setChecked(false);

  m_devToolBar->addSeparator();

#ifdef BUILD_DESIGNER
  // Add "Build style" button
  if (!m_mapcssFilePath.isEmpty())
  {
    m_pBuildStyleAction =
        m_devToolBar->addAction(QIcon(":/navig64/run.png"), tr("Build style"), this, SLOT(OnBuildStyle()));
    m_pBuildStyleAction->setCheckable(false);
    m_pBuildStyleAction->setToolTip(tr("Build style"));

    m_pRecalculateGeomIndex = m_devToolBar->addAction(QIcon(":/navig64/geom.png"), tr("Recalculate geometry index"),
                                                      this, SLOT(OnRecalculateGeomIndex()));
    m_pRecalculateGeomIndex->setCheckable(false);
    m_pRecalculateGeomIndex->setToolTip(tr("Recalculate geometry index"));
  }

  // Add "Debug style" button
  m_pDrawDebugRectAction =
      m_devToolBar->addAction(QIcon(":/navig64/bug.png"), tr("Debug style"), this, SLOT(OnDebugStyle()));
  m_pDrawDebugRectAction->setCheckable(true);
  m_pDrawDebugRectAction->setChecked(false);
  m_pDrawDebugRectAction->setToolTip(tr("Debug style"));
  m_pDrawWidget->GetFramework().EnableDebugRectRendering(false);

  // Add "Get statistics" button
  m_pGetStatisticsAction =
      m_devToolBar->addAction(QIcon(":/navig64/chart.png"), tr("Get statistics"), this, SLOT(OnGetStatistics()));
  m_pGetStatisticsAction->setCheckable(false);
  m_pGetStatisticsAction->setToolTip(tr("Get statistics"));

  // Add "Run tests" button
  m_pRunTestsAction = m_devToolBar->addAction(QIcon(":/navig64/test.png"), tr("Run tests"), this, SLOT(OnRunTests()));
  m_pRunTestsAction->setCheckable(false);
  m_pRunTestsAction->setToolTip(tr("Run tests"));

  // Add "Build phone package" button
  m_pBuildPhonePackAction = m_devToolBar->addAction(QIcon(":/navig64/phonepack.png"), tr("Build phone package"), this,
                                                    SLOT(OnBuildPhonePackage()));
  m_pBuildPhonePackAction->setCheckable(false);
  m_pBuildPhonePackAction->setToolTip(tr("Build phone package"));
#endif  // BUILD_DESIGNER

  addToolBar(Qt::RightToolBarArea, m_devToolBar);
  if (m_screenshotMode)
    m_devToolBar->setVisible(false);
}

Framework & MainWindow::GetFramework() const
{
  return m_pDrawWidget->GetFramework();
}

void MainWindow::CreateCountryStatusControls()
{
  QHBoxLayout * mainLayout = new QHBoxLayout();
  m_downloadButton = CreateBlackControl<QPushButton>("Download");
  mainLayout->addWidget(m_downloadButton, 0, Qt::AlignHCenter);
  m_downloadButton->setVisible(false);
  connect(m_downloadButton, &QAbstractButton::released, this, &MainWindow::OnDownloadClicked);

  m_retryButton = CreateBlackControl<QPushButton>("Retry downloading");
  mainLayout->addWidget(m_retryButton, 0, Qt::AlignHCenter);
  m_retryButton->setVisible(false);
  connect(m_retryButton, &QAbstractButton::released, this, &MainWindow::OnRetryDownloadClicked);

  m_downloadingStatusLabel = CreateBlackControl<QLabel>("Downloading");
  mainLayout->addWidget(m_downloadingStatusLabel, 0, Qt::AlignHCenter);
  m_downloadingStatusLabel->setVisible(false);

  m_pDrawWidget->setLayout(mainLayout);

  auto const OnCountryChanged = [this](storage::CountryId const & countryId)
  {
    m_downloadButton->setVisible(false);
    m_retryButton->setVisible(false);
    m_downloadingStatusLabel->setVisible(false);

    m_lastCountry = countryId;
    if (!storage::IsCountryIdValid(countryId))
      return;

    auto const & storage = GetFramework().GetStorage();
    auto status = storage.CountryStatusEx(countryId);
    auto const & countryName = countryId;

    if (status == storage::Status::NotDownloaded)
    {
      m_downloadButton->setVisible(true);

      std::string units;
      size_t sizeToDownload = 0;
      FormatMapSize(storage.CountrySizeInBytes(countryId).second, units, sizeToDownload);
      std::stringstream str;
      str << "Download (" << countryName << ") " << sizeToDownload << units;
      m_downloadButton->setText(str.str().c_str());
    }
    else if (status == storage::Status::Downloading)
    {
      m_downloadingStatusLabel->setVisible(true);
    }
    else if (status == storage::Status::InQueue)
    {
      m_downloadingStatusLabel->setVisible(true);

      std::stringstream str;
      str << countryName << " is waiting for downloading";
      m_downloadingStatusLabel->setText(str.str().c_str());
    }
    else if (status != storage::Status::OnDisk && status != storage::Status::OnDiskOutOfDate)
    {
      m_retryButton->setVisible(true);

      std::stringstream str;
      str << "Retry to download " << countryName;
      m_retryButton->setText(str.str().c_str());
    }
  };

  GetFramework().SetCurrentCountryChangedListener(OnCountryChanged);

  GetFramework().GetStorage().Subscribe(
      [this, onChanged = std::move(OnCountryChanged)](storage::CountryId const & countryId)
  {
    // Storage also calls notifications for parents, but we are interested in leafs only.
    if (GetFramework().GetStorage().IsLeaf(countryId))
      onChanged(countryId);
  }, [this](storage::CountryId const & countryId, downloader::Progress const & progress)
  {
    std::stringstream str;
    str << "Downloading (" << countryId << ") " << progress.m_bytesDownloaded * 100 / progress.m_bytesTotal << "%";
    m_downloadingStatusLabel->setText(str.str().c_str());
  });
}

void MainWindow::OnAbout()
{
  AboutDialog dlg(this);
  dlg.exec();
}

void MainWindow::OnLocationError(location::TLocationError errorCode)
{
  switch (errorCode)
  {
  case location::EDenied: [[fallthrough]];
  case location::ETimeout: [[fallthrough]];
  case location::EUnknown:
  {
    if (m_pDrawWidget && m_pMyPositionAction)
      m_pMyPositionAction->setEnabled(false);
    break;
  }

  default: ASSERT(false, ("Not handled location notification:", errorCode)); break;
  }

  if (m_pDrawWidget != nullptr)
    m_pDrawWidget->GetFramework().OnLocationError(errorCode);
}

void MainWindow::OnLocationUpdated(location::GpsInfo const & info)
{
  m_pDrawWidget->GetFramework().OnLocationUpdate(info);
}

void MainWindow::OnMyPosition()
{
  if (m_pMyPositionAction->isEnabled())
    m_pDrawWidget->GetFramework().SwitchMyPositionNextMode();
}

void MainWindow::OnCreateFeatureClicked()
{
  if (m_pCreateFeatureAction->isChecked())
  {
    m_pDrawWidget->ChoosePositionModeEnable();
  }
  else
  {
    m_pDrawWidget->ChoosePositionModeDisable();
    m_pDrawWidget->CreateFeature();
  }
}

void MainWindow::OnSwitchSelectionMode(SelectionMode mode)
{
  if (m_selection->isChecked(mode))
  {
    m_selection->setCurrent(mode);
    m_pDrawWidget->SetSelectionMode(mode);
  }
  else
    OnClearSelection();
}

void MainWindow::OnSwitchMwmsBordersSelectionMode()
{
  MwmsBordersSelection dlg(this);
  auto const response = dlg.ShowModal();
  if (response == SelectionMode::Cancelled)
  {
    m_pDrawWidget->DropSelectionIfMWMBordersMode();
    return;
  }

  m_selection->setCurrent(SelectionMode::MWMBorders);
  m_pDrawWidget->SetSelectionMode(response);
}

void MainWindow::OnClearSelection()
{
  m_pDrawWidget->GetFramework().GetDrapeApi().Clear();
  m_pDrawWidget->SetSelectionMode({});

  m_selection->setMainIcon({});
}

void MainWindow::OnSearchButtonClicked()
{
  if (m_devSearchAction->isChecked())
    m_devSearchDockWidget->show();
  else
    m_devSearchDockWidget->hide();
}

void MainWindow::OnLoginMenuItem()
{
  OsmAuthDialog dlg(this);
  dlg.exec();
}

void MainWindow::OnUploadEditsMenuItem()
{
  std::string token;
  if (!settings::Get(kOauthTokenSetting, token) || token.empty())
  {
    OnLoginMenuItem();
  }
  else
  {
    auto & editor = osm::Editor::Instance();
    if (editor.HaveMapEditsOrNotesToUpload())
      editor.UploadChanges(token, {{"created_by", "Organic Maps " OMIM_OS_NAME}});
  }
}

void MainWindow::OnBeforeEngineCreation()
{
  m_pDrawWidget->GetFramework().SetMyPositionModeListener([this](location::EMyPositionMode mode, bool /*routingActive*/)
  { LocationStateModeChanged(mode); });
}

void MainWindow::OnFocusChange(QWidget * previous, QWidget * current)
{
  if (current == m_userSearchQueryEdit)
    m_userSearchDockWidget->show();
}

void MainWindow::OnSearchDockWidgetVisibilityChange(bool visible)
{
  if (!visible)
  {
    if (m_userSearchQueryEdit->hasFocus())
      m_pDrawWidget->setFocus();
    m_userSearchQueryEdit->setText("");
  }
  m_userSearchCloseAction->setVisible(visible);
}

void MainWindow::OnSearchDockWidgetWidthChange(int width)
{
  // TODO (@zagto): handle RTL layout
  int const offset = m_userSearchQueryEdit->x();
  m_userSearchQueryEdit->setFixedWidth(width - offset * 2);
}

void MainWindow::OnPreferences()
{
  Framework & framework = m_pDrawWidget->GetFramework();
  PreferencesDialog dlg(this, framework);
  connect(&dlg, &PreferencesDialog::DeveloperModeChanged, this, &MainWindow::OnDeveloperModeChanged);
  dlg.exec();

  framework.EnterForeground();
}

#ifdef BUILD_DESIGNER
void MainWindow::OnBuildStyle()
{
  try
  {
    build_style::BuildAndApply(m_mapcssFilePath);
    m_pDrawWidget->RefreshDrawingRules();

    bool enabled;
    if (!settings::Get(kEnabledAutoRegenGeomIndex, enabled))
      enabled = false;

    if (enabled)
    {
      build_style::NeedRecalculate = true;
      QMainWindow::close();
    }
  }
  catch (std::exception & e)
  {
    QMessageBox msgBox;
    msgBox.setWindowTitle("Error");
    msgBox.setText(e.what());
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
  }
}

void MainWindow::OnRecalculateGeomIndex()
{
  try
  {
    QMessageBox msgBox;
    msgBox.setWindowTitle("Warning");
    msgBox.setText("Geometry index will be regenerated. It can take a while.\nApplication may be closed and reopened!");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    if (msgBox.exec() == QMessageBox::Yes)
    {
      build_style::NeedRecalculate = true;
      QMainWindow::close();
    }
  }
  catch (std::exception & e)
  {
    QMessageBox msgBox;
    msgBox.setWindowTitle("Error");
    msgBox.setText(e.what());
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
  }
}

void MainWindow::OnDebugStyle()
{
  bool const checked = m_pDrawDebugRectAction->isChecked();
  m_pDrawWidget->GetFramework().EnableDebugRectRendering(checked);
  m_pDrawWidget->RefreshDrawingRules();
}

void MainWindow::OnGetStatistics()
{
  try
  {
    QString text = build_style::GetCurrentStyleStatistics();
    InfoDialog dlg(QString("Style statistics"), text, NULL);
    dlg.exec();
  }
  catch (std::exception & e)
  {
    QMessageBox msgBox;
    msgBox.setWindowTitle("Error");
    msgBox.setText(e.what());
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
  }
}

void MainWindow::OnRunTests()
{
  try
  {
    std::pair<bool, QString> res = build_style::RunCurrentStyleTests();
    InfoDialog dlg(QString("Style tests: ") + (res.first ? "OK" : "FAILED"), res.second, NULL);
    dlg.exec();
  }
  catch (std::exception & e)
  {
    QMessageBox msgBox;
    msgBox.setWindowTitle("Error");
    msgBox.setText(e.what());
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
  }
}

void MainWindow::OnBuildPhonePackage()
{
  try
  {
    char const * const kStylesFolder = "styles";
    char const * const kClearStyleFolder = "clear";

    QString const targetDir = QFileDialog::getExistingDirectory(nullptr, "Choose output directory");
    if (targetDir.isEmpty())
      return;
    auto outDir = QDir(JoinPathQt({targetDir, kStylesFolder}));
    if (outDir.exists())
    {
      QMessageBox msgBox;
      msgBox.setWindowTitle("Warning");
      msgBox.setText(QString("Folder ") + outDir.absolutePath() + " will be deleted?");
      msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
      msgBox.setDefaultButton(QMessageBox::No);
      auto result = msgBox.exec();
      if (result == QMessageBox::No)
        throw std::runtime_error(std::string("Target directory exists: ") + outDir.absolutePath().toStdString());
    }

    QString const stylesDir = JoinPathQt({m_mapcssFilePath, "..", "..", ".."});
    if (!QDir(JoinPathQt({stylesDir, kClearStyleFolder})).exists())
      throw std::runtime_error(std::string("Styles folder is not found in ") + stylesDir.toStdString());

    QString text = build_style::RunBuildingPhonePack(stylesDir, targetDir);
    text.append("\nMobile device style package is in the directory: ");
    text.append(JoinPathQt({targetDir, kStylesFolder}));
    text.append(". Copy it to your mobile device.\n");
    InfoDialog dlg(QString("Building phone pack"), text, nullptr);
    dlg.exec();
  }
  catch (std::exception & e)
  {
    QMessageBox msgBox;
    msgBox.setWindowTitle("Error");
    msgBox.setText(e.what());
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
  }
}
#endif  // BUILD_DESIGNER

#ifndef NO_DOWNLOADER
void MainWindow::ShowUpdateDialog()
{
  UpdateDialog dlg(this, m_pDrawWidget->GetFramework());
  dlg.ShowModal();
  m_pDrawWidget->update();
}

#endif  // NO_DOWNLOADER

void MainWindow::CreateUserSearchPanel()
{
  m_userSearchDockWidget = new QDockWidget(tr("Search"));

  m_userSearchPanel = new UserSearchPanel(GetFramework());
  addDockWidget(Qt::LeftDockWidgetArea, m_userSearchDockWidget);
  m_userSearchDockWidget->setWidget(m_userSearchPanel);
  m_userSearchDockWidget->setTitleBarWidget(new QWidget(m_userSearchDockWidget));
  m_userSearchDockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
  m_userSearchDockWidget->hide();

  connect(m_userSearchQueryEdit, &QLineEdit::textChanged, m_userSearchPanel, &UserSearchPanel::OnSearchQueryChanged);
  connect(m_userSearchCloseAction, &QAction::triggered, m_userSearchDockWidget, &QDockWidget::hide);
  connect(m_userSearchPanel, &UserSearchPanel::WidthChanged, this, &MainWindow::OnSearchDockWidgetWidthChange);
  connect(m_userSearchPanel, &UserSearchPanel::SuggestionAccepted, m_userSearchQueryEdit, &QLineEdit::setText);

  // TODO (@zagto): tab focus order should be m_userSearchQueryEdit -> first search result
}

void MainWindow::CreateDevSearchPanel()
{
  m_devSearchDockWidget = new QDockWidget(tr("Search"));
  addDockWidget(Qt::RightDockWidgetArea, m_devSearchDockWidget);
  m_devSearchDockWidget->hide();
  m_devSearchDockWidget->setWidget(new DevSearchPanel(m_pDrawWidget, m_devSearchDockWidget));
}

void MainWindow::closeEvent(QCloseEvent * e)
{
  m_pDrawWidget->PrepareShutdown();
  e->accept();
}

void MainWindow::OnDownloadClicked()
{
  GetFramework().GetStorage().DownloadNode(m_lastCountry);
}

void MainWindow::OnRetryDownloadClicked()
{
  GetFramework().GetStorage().RetryDownloadNode(m_lastCountry);
}

void MainWindow::SetLayerEnabled(LayerType type, bool enable)
{
  auto & frm = m_pDrawWidget->GetFramework();
  switch (type)
  {
  // @todo Uncomment when we will integrate a traffic provider.
  // case TRAFFIC:
  //   frm.GetTrafficManager().SetEnabled(enable);
  //   frm.SaveTrafficEnabled(enable);
  //   break;
  case TRANSIT:
    frm.GetTransitManager().EnableTransitSchemeMode(enable);
    frm.SaveTransitSchemeEnabled(enable);
    break;
  case ISOLINES:
    frm.GetIsolinesManager().SetEnabled(enable);
    frm.SaveIsolinesEnabled(enable);
    break;
  case OUTDOORS:
    frm.SaveOutdoorsEnabled(enable);
    if (enable)
      m_pDrawWidget->SetMapStyleToOutdoors();
    else
      m_pDrawWidget->SetMapStyleToDefault();
    break;
  case HIKING: frm.SetHikingEnabled(enable); break;
  case CYCLING: frm.SetCyclingEnabled(enable); break;
  }
}

void MainWindow::OnLayerEnabled(LayerType layer)
{
  SetLayerEnabled(layer, m_layers->isChecked(layer));
}

void MainWindow::OnRulerEnabled()
{
  m_pDrawWidget->SetRuler(m_rulerAction->isChecked());
}

void MainWindow::OnRoutePointSelected(RouteMarkType type)
{
  m_routing->setCurrent(type);
  m_pDrawWidget->SetRoutePointAddMode(type);
}

void MainWindow::OnFollowRoute()
{
  m_pDrawWidget->FollowRoute();
}

void MainWindow::OnClearRoute()
{
  m_pDrawWidget->ClearRoute();
}

void MainWindow::OnRoutingSettings()
{
  RoutingSettings dlg(this, m_pDrawWidget->GetFramework());
  dlg.ShowModal();
}

void MainWindow::OnBookmarksAction()
{
  BookmarkDialog dlg(this, m_pDrawWidget->GetFramework());
  dlg.ShowModal();
  m_pDrawWidget->update();
}

void MainWindow::OnDeveloperModeChanged(bool enable)
{
  m_devToolBar->setVisible(enable);
  m_userSearchPanel->OnDeveloperModeChanged(enable);
}

}  // namespace qt
