#include "openlr/openlr_match_quality/openlr_assessment_tool/trafficmodeinitdlg.h"
#include "ui_trafficmodeinitdlg.h"

#include "platform/settings.hpp"

#include <QtWidgets/QFileDialog>

#include <string>

namespace
{
std::string const kDataFilePath = "LastOpenlrAssessmentDataFilePath";
}  // namespace

namespace openlr
{
TrafficModeInitDlg::TrafficModeInitDlg(QWidget * parent) : QDialog(parent), m_ui(new Ui::TrafficModeInitDlg)
{
  m_ui->setupUi(this);

  std::string lastDataFilePath;
  if (settings::Get(kDataFilePath, lastDataFilePath))
    m_ui->dataFileName->setText(QString::fromStdString(lastDataFilePath));

  connect(m_ui->chooseDataFileButton, &QPushButton::clicked,
          [this](bool) { SetFilePathViaDialog(*m_ui->dataFileName, tr("Choose data file"), "*.xml"); });
}

TrafficModeInitDlg::~TrafficModeInitDlg()
{
  delete m_ui;
}

void TrafficModeInitDlg::accept()
{
  m_dataFileName = m_ui->dataFileName->text().trimmed().toStdString();
  settings::Set(kDataFilePath, m_dataFileName);
  QDialog::accept();
}

void TrafficModeInitDlg::SetFilePathViaDialog(QLineEdit & dest, QString const & title, QString const & filter)
{
  QFileDialog openFileDlg(nullptr, title, {} /* directory */, filter);
  openFileDlg.exec();
  if (openFileDlg.result() != QDialog::DialogCode::Accepted)
    return;

  dest.setText(openFileDlg.selectedFiles().first());
}
}  // namespace openlr
