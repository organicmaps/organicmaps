#pragma once

#include <string>

#include <QtWidgets/QDialog>

class QLineEdit;

namespace Ui {
class TrafficModeInitDlg;
}

namespace openlr
{
class TrafficModeInitDlg : public QDialog
{
  Q_OBJECT

public:
  explicit TrafficModeInitDlg(QWidget * parent = nullptr);
  ~TrafficModeInitDlg();

  std::string GetDataFilePath() const { return m_dataFileName; }

private:
  void SetFilePathViaDialog(QLineEdit & dest, QString const & title,
                            QString const & filter = {});
public slots:
  void accept() override;

private:
  Ui::TrafficModeInitDlg * m_ui;

  std::string m_dataFileName;
};
}  // namespace openlr
