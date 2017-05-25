#include "openlr/openlr_match_quality/openlr_assessment_tool/mainwindow.hpp"

#include "openlr/openlr_match_quality/openlr_assessment_tool/traffic_panel.hpp"
#include "openlr/openlr_match_quality/openlr_assessment_tool/trafficmodeinitdlg.h"

#include "qt/qt_common/map_widget.hpp"

#include "map/framework.hpp"

#include "drape_frontend/drape_api.hpp"

#include <QDockWidget>
#include <QFileDialog>
#include <QLayout>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStandardPaths>

#include <cerrno>
#include <cstring>

namespace
{
// TODO(mgsergio): Consider getting rid of this class: just put everything
// in TrafficMode.
class TrafficDrawerDelegate : public TrafficDrawerDelegateBase
{
public:
  TrafficDrawerDelegate(Framework & framework)
  : m_framework(framework)
  , m_drapeApi(m_framework.GetDrapeApi())
  {
  }

  void SetViewportCenter(m2::PointD const & center) override
  {
    m_framework.SetViewportCenter(center);
  }

  void DrawDecodedSegments(DecodedSample const & sample, int const sampleIndex) override
  {
    CHECK(!sample.GetItems().empty(), ("Sample must not be empty."));
    auto const & points = sample.GetPoints(sampleIndex);

    LOG(LINFO, ("Decoded segment", points));
    m_drapeApi.AddLine(NextLineId(),
                       df::DrapeApiLineData(points, dp::Color(0, 0, 255, 255))
                       .Width(3.0f).ShowPoints(true /* markPoints */));
  }

  void DrawEncodedSegment(openlr::LinearSegment const & segment) override
  {
    auto const & points = segment.GetMercatorPoints();

    LOG(LINFO, ("Encoded segment", points));
    m_drapeApi.AddLine(NextLineId(),
                       df::DrapeApiLineData(points, dp::Color(255, 0, 0, 255))
                       .Width(3.0f).ShowPoints(true /* markPoints */));
  }

  void Clear() override
  {
    m_drapeApi.Clear();
  }

private:
  string NextLineId() { return strings::to_string(m_lineId++); }

  uint32_t m_lineId = 0;

  Framework & m_framework;
  df::DrapeApi & m_drapeApi;
};
}  // namespace


MainWindow::MainWindow(Framework & framework)
  : m_framework(framework)
{
  auto * mapWidget = new qt::common::MapWidget(
      m_framework, false /* apiOpenGLES3 */, this /* parent */
  );
  setCentralWidget(mapWidget);

  // setWindowTitle(tr("MAPS.ME"));
  // setWindowIcon(QIcon(":/ui/logo.png"));

  QMenu * fileMenu = new QMenu("File", this);
  menuBar()->addMenu(fileMenu);

  fileMenu->addAction("Open sample", this, &MainWindow::OnOpenTrafficSample);

  m_closeTrafficSampleAction = fileMenu->addAction(
      "Close sample", this, &MainWindow::OnCloseTrafficSample
  );
  m_closeTrafficSampleAction->setEnabled(false /* enabled */);

  m_saveTrafficSampleAction = fileMenu->addAction(
      "Save sample", this, &MainWindow::OnSaveTrafficSample
  );
  m_saveTrafficSampleAction->setEnabled(false /* enabled */);
}

void MainWindow::CreateTrafficPanel(string const & dataFilePath, string const & sampleFilePath)
{
  m_docWidget = new QDockWidget(tr("Routes"), this);
  addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, m_docWidget);

  m_trafficMode = new TrafficMode(dataFilePath, sampleFilePath,
                                  m_framework.GetIndex(),
                                  make_unique<TrafficDrawerDelegate>(m_framework));
  m_docWidget->setWidget(new TrafficPanel(m_trafficMode, m_docWidget));

  m_docWidget->adjustSize();
  m_docWidget->show();
}

void MainWindow::DestroyTrafficPanel()
{
  removeDockWidget(m_docWidget);
  delete m_docWidget;
  m_docWidget = nullptr;

  delete m_trafficMode;
  m_trafficMode = nullptr;
}

void MainWindow::OnOpenTrafficSample()
{
  TrafficModeInitDlg dlg;
  dlg.exec();
  if (dlg.result() != QDialog::DialogCode::Accepted)
    return;

  CreateTrafficPanel(dlg.GetDataFilePath(), dlg.GetSampleFilePath());
  m_closeTrafficSampleAction->setEnabled(true /* enabled */);
  m_saveTrafficSampleAction->setEnabled(true /* enabled */);
}

void MainWindow::OnCloseTrafficSample()
{
  // TODO(mgsergio):
  // If not saved, ask a user if he/she wants to save.
  // OnSaveTrafficSample()

  m_saveTrafficSampleAction->setEnabled(false /* enabled */);
  m_closeTrafficSampleAction->setEnabled(false /* enabled */);
  DestroyTrafficPanel();
}

void MainWindow::OnSaveTrafficSample()
{
  // TODO(mgsergio): Add default filename.
  auto const & fileName = QFileDialog::getSaveFileName(this, "Save sample");
  if (fileName.isEmpty())
    return;

  if (!m_trafficMode->SaveSampleAs(fileName.toStdString()))
  {
    QMessageBox::critical(
        this, "Saving error",
        QString("Can't save file: ") + strerror(errno));
  }
}
