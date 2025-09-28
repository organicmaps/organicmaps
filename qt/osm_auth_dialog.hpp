#pragma once

#include <thread>

#include <QtWidgets/QDialog>

namespace qt
{

class OsmAuthDialog : public QDialog
{
  Q_OBJECT

public:
  explicit OsmAuthDialog(QWidget * parent);
  ~OsmAuthDialog() override;

private slots:
  void OnAction();

private:
  std::thread m_authThread;
};

}  // namespace qt
