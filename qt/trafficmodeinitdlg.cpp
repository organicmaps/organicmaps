#include "qt/trafficmodeinitdlg.h"
#include "ui_trafficmodeinitdlg.h"

#include "platform/settings.hpp"

#include <QtWidgets/QFileDialog>

namespace
{
string const kDataFilePath = "LastTrafficDataFilePath";
string const kSampleFilePath = "LastTrafficSampleFilePath";
}  // namespace

TrafficModeInitDlg::TrafficModeInitDlg(QWidget * parent) :
  QDialog(parent),
  m_ui(new Ui::TrafficModeInitDlg)
{
  m_ui->setupUi(this);

  string lastDataFilePath;
  string lastSampleFilePath;
  if (settings::Get(kDataFilePath, lastDataFilePath))
      m_ui->dataFileName->setText(QString::fromStdString(lastDataFilePath));
  if (settings::Get(kSampleFilePath, lastSampleFilePath))
    m_ui->sampleFileName->setText(QString::fromStdString(lastSampleFilePath));

  connect(m_ui->chooseDataFileButton, &QPushButton::clicked, [this](bool)
  {
    SetFilePathViaDialog(*m_ui->dataFileName, tr("Choose traffic data file"), "*.xml");
  });
  connect(m_ui->chooseSampleFileButton, &QPushButton::clicked, [this](bool)
  {
    SetFilePathViaDialog(*m_ui->sampleFileName, tr("Choose traffic sample file"));
  });
}

TrafficModeInitDlg::~TrafficModeInitDlg()
{
  delete m_ui;
}

void TrafficModeInitDlg::accept()
{
  m_dataFileName = m_ui->dataFileName->text().trimmed().toStdString();
  m_sampleFileName = m_ui->sampleFileName->text().trimmed().toStdString();

  settings::Set(kDataFilePath, m_dataFileName);
  settings::Set(kSampleFilePath, m_sampleFileName);

  QDialog::accept();
}

void TrafficModeInitDlg::SetFilePathViaDialog(QLineEdit & dest, QString const & title,
                                              QString const & filter)
{
  QFileDialog openFileDlg(nullptr, title, {} /* directory */, filter);
  openFileDlg.exec();
  if (openFileDlg.result() != QDialog::DialogCode::Accepted)
    return;

  dest.setText(openFileDlg.selectedFiles().first());
}
