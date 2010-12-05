#include "main_tester.hpp"

#include <QtGui/QApplication>
#include <QtGui/QFrame>
#include <QtGui/QBoxLayout>

#include "../base/start_mem_debug.hpp"


namespace tst
{
  class MainWindow : public QFrame
  {
  public:
    MainWindow(QWidget * pWidget)
    {
      QHBoxLayout * pMainLayout = new QHBoxLayout();
      pMainLayout->setContentsMargins(0, 0, 0, 0);

      pMainLayout->addWidget(pWidget);

      setLayout(pMainLayout);

      setWindowTitle(tr("Testing Framework Form"));

      pWidget->setFocus();

      resize(640, 480);
    }
  };

int BaseTester::Run(char const * name, function<QWidget * (void)> const & fn)
{
  char * argv[] = { const_cast<char *>(name) };
  int argc = 1;

  QApplication app(argc, argv);

  MainWindow wnd(fn());
  wnd.show();

  return app.exec();
}

}
