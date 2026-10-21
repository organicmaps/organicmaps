#include "testing/testing.hpp"

#include "qt/qml/forwarding_quickwidget.hpp"

#include <QtTest/qtestspontaneevent.h>
#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>
#include <QtCore/QUrl>
#include <QtGui/QMouseEvent>
#include <QtGui/QPointingDevice>
#include <QtGui/QWheelEvent>
#include <QtGui/QWindow>
#include <QtQuick/QQuickItem>
#include <QtTest/QTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

#include <utility>
#include <vector>

namespace forwarding_quickwidget_tests
{
using Positions = std::vector<std::pair<int, int>>;
using Buttons = std::vector<Qt::MouseButton>;
// TouchBegin, TouchUpdate and TouchEnd counts.
using TouchCounts = std::vector<int>;

char constexpr kEmptyQml[] = "import QtQuick; Item {}";
char constexpr kAcceptingQml[] =
    "import QtQuick; Item { MouseArea { anchors.fill: parent; acceptedButtons: Qt.AllButtons } }";
char constexpr kWrappedMouseAreaQml[] =
    "import QtQuick; Item { Item { x: 40; y: 40; MouseArea { objectName: 'area'; x: 20; y: 20; width: 60; height: 60; "
    "property int presses: 0; onPressed: presses++ } } }";
char constexpr kFullSizeRectangleQml[] = "import QtQuick; Item { Rectangle { anchors.fill: parent } }";
char constexpr kDisabledMouseAreaQml[] =
    "import QtQuick; Item { MouseArea { enabled: false; width: 100; height: 100 } }";
char constexpr kClippedMouseAreaQml[] =
    "import QtQuick; Item { Item { clip: true; MouseArea { width: 100; height: 100 } } }";
char constexpr kOpenPopupQml[] =
    "import QtQuick; import QtQuick.Controls; "
    "Item { Popup { width: 100; height: 100; visible: true; MouseArea { anchors.fill: parent } } }";
char constexpr kTapHandlerQml[] =
    "import QtQuick; Item { Rectangle { width: 100; height: 100; "
    "TapHandler { objectName: 'tap'; property int taps: 0; onTapped: taps++ } } }";
char constexpr kModalPopupQml[] =
    "import QtQuick; import QtQuick.Controls; Item { Popup { objectName: 'popup'; width: 100; height: 100; "
    "modal: true; closePolicy: Popup.NoAutoClose; visible: true } }";
char constexpr kDismissableModalPopupQml[] =
    "import QtQuick; import QtQuick.Controls; "
    "Item { Popup { objectName: 'popup'; width: 100; height: 100; modal: true; visible: true } }";
char constexpr kFlickableQml[] =
    "import QtQuick; Item { Flickable { objectName: 'flick'; width: 100; height: 100; contentWidth: 300; "
    "contentHeight: 300; Rectangle { width: 300; height: 300 } } }";
char constexpr kRightAcceptingQml[] =
    "import QtQuick; Item { MouseArea { anchors.fill: parent; acceptedButtons: Qt.RightButton } }";
char constexpr kLeftAcceptingQml[] =
    "import QtQuick; Item { MouseArea { anchors.fill: parent; acceptedButtons: Qt.LeftButton } }";
char constexpr kLeftHalfAcceptingQml[] =
    "import QtQuick; Item { MouseArea { width: parent.width / 2; height: parent.height } }";

void EnsureApp()
{
  if (!QApplication::instance())
  {
    static int argc = 0;
    // Never destroyed: ~QApplication at exit() runs Quick Controls' post routines after Qt's statics are gone.
    new QApplication(argc, nullptr);
  }
}

class CountingParent : public QWidget
{
public:
  CountingParent() { setAttribute(Qt::WA_AcceptTouchEvents); }

  Positions m_presses;
  Positions m_releases;
  Buttons m_pressButtons;
  Buttons m_releaseButtons;
  Positions m_moves;
  std::vector<int> m_moveButtons;
  int m_doubleClicks = 0;
  int m_wheels = 0;
  TouchCounts m_touches = TouchCounts(3);

protected:
  bool event(QEvent * e) override
  {
    switch (e->type())
    {
    case QEvent::TouchBegin: ++m_touches[0]; break;
    case QEvent::TouchUpdate: ++m_touches[1]; break;
    case QEvent::TouchEnd: ++m_touches[2]; break;
    default: return QWidget::event(e);
    }
    // Like the map, take the whole touch sequence once its TouchBegin arrives.
    e->accept();
    return true;
  }

  void mousePressEvent(QMouseEvent * e) override
  {
    m_presses.emplace_back(e->position().toPoint().x(), e->position().toPoint().y());
    m_pressButtons.push_back(e->button());
    QWidget::mousePressEvent(e);
  }

  void mouseReleaseEvent(QMouseEvent * e) override
  {
    m_releases.emplace_back(e->position().toPoint().x(), e->position().toPoint().y());
    m_releaseButtons.push_back(e->button());
    QWidget::mouseReleaseEvent(e);
  }

  void mouseDoubleClickEvent(QMouseEvent * e) override
  {
    ++m_doubleClicks;
    QWidget::mouseDoubleClickEvent(e);
  }

  void mouseMoveEvent(QMouseEvent * e) override
  {
    m_moves.emplace_back(e->position().toPoint().x(), e->position().toPoint().y());
    m_moveButtons.push_back(e->buttons().toInt());
    QWidget::mouseMoveEvent(e);
  }

  void wheelEvent(QWheelEvent * e) override
  {
    ++m_wheels;
    QWidget::wheelEvent(e);
  }
};

class Fixture
{
public:
  Fixture(char const * qml, QPoint const & childPos)
  {
    TEST(m_dir.isValid(), ());
    QString const qmlPath = m_dir.filePath("test.qml");
    QFile file(qmlPath);
    TEST(file.open(QIODevice::WriteOnly), ());
    file.write(qml);
    file.close();

    m_parent.resize(400, 300);
    m_child = new qt::ForwardingQuickWidget();
    m_child->setParent(&m_parent);
    m_child->setGeometry(QRect(childPos, QSize(200, 200)));
    m_child->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_child->setSource(QUrl::fromLocalFile(qmlPath));
    TEST_EQUAL(m_child->status(), QQuickWidget::Ready, ());
  }

  void Send(QEvent::Type type, Qt::MouseButton button, QPoint const & childPos)
  {
    Send(type, button, type == QEvent::MouseButtonPress ? Qt::MouseButtons(button) : Qt::NoButton, childPos);
  }

  void Send(QEvent::Type type, Qt::MouseButton button, Qt::MouseButtons buttons, QPoint const & childPos)
  {
    QMouseEvent e(type, QPointF(childPos), QPointF(m_child->mapToGlobal(childPos)), button, buttons, Qt::NoModifier);
    QApplication::sendEvent(m_child, &e);
  }

  void SendWheel(QPoint const & childPos)
  {
    QWheelEvent e(QPointF(childPos), QPointF(m_child->mapToGlobal(childPos)), QPoint(), QPoint(0, 120), Qt::NoButton,
                  Qt::NoModifier, Qt::NoScrollPhase, false);
    QSpontaneKeyEvent::setSpontaneous(&e);
    // sendEvent() would clear the spontaneous flag.
    QApplication::instance()->notify(m_child, &e);
  }

  void Show()
  {
    if (m_parent.isVisible())
      return;
    m_parent.show();
    TEST(QTest::qWaitForWindowExposed(&m_parent), ());
  }

  void ClickOnWindow(QPoint const & childPos)
  {
    Show();
    QTest::mouseClick(m_parent.windowHandle(), Qt::LeftButton, Qt::NoModifier, m_child->mapTo(&m_parent, childPos));
  }

  void DoubleClickOnWindow(QPoint const & childPos)
  {
    Show();
    QTest::mouseDClick(m_parent.windowHandle(), Qt::LeftButton, Qt::NoModifier, m_child->mapTo(&m_parent, childPos));
  }

  void DragOnWindow(QPoint const & childPos, QPoint const & step, int steps)
  {
    Show();
    QWindow * window = m_parent.windowHandle();
    QPoint pos = m_child->mapTo(&m_parent, childPos);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, pos);
    for (int i = 0; i < steps; ++i)
      QTest::mouseMove(window, pos += step);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, pos);
  }

  void SendTouch(QPoint const & childPos)
  {
    static QPointingDevice * const touchScreen = QTest::createTouchDevice();
    Show();
    QWindow * window = m_parent.windowHandle();
    QPoint const pos = m_child->mapTo(&m_parent, childPos);
    QTest::touchEvent(window, touchScreen).press(0, pos, window);
    QTest::touchEvent(window, touchScreen).move(0, pos + QPoint(5, 5), window);
    QTest::touchEvent(window, touchScreen).release(0, pos + QPoint(5, 5), window);
  }

  int QmlProperty(char const * objectName, char const * name) const
  {
    return m_child->rootObject()->findChild<QObject *>(objectName)->property(name).toInt();
  }

  QTemporaryDir m_dir;
  CountingParent m_parent;
  qt::ForwardingQuickWidget * m_child = nullptr;
};

UNIT_TEST(ForwardingQuickWidget_IgnoredPressReachesParentOnce)
{
  EnsureApp();
  Fixture f(kEmptyQml, QPoint(0, 0));
  f.Send(QEvent::MouseButtonPress, Qt::LeftButton, QPoint(50, 50));
  TEST_EQUAL(f.m_parent.m_presses.size(), 1, (f.m_parent.m_presses));
}

UNIT_TEST(ForwardingQuickWidget_ReleaseAfterIgnoredPressReachesParentOnceInParentCoords)
{
  EnsureApp();
  Fixture f(kEmptyQml, QPoint(100, 0));
  f.Send(QEvent::MouseButtonPress, Qt::LeftButton, QPoint(50, 50));
  f.Send(QEvent::MouseButtonRelease, Qt::LeftButton, QPoint(60, 60));
  TEST_EQUAL(f.m_parent.m_releases, Positions({{160, 60}}), ());
}

UNIT_TEST(ForwardingQuickWidget_AcceptedPressKeepsReleaseInQml)
{
  EnsureApp();
  Fixture f(kAcceptingQml, QPoint(0, 0));
  f.Send(QEvent::MouseButtonPress, Qt::LeftButton, QPoint(50, 50));
  f.Send(QEvent::MouseButtonRelease, Qt::LeftButton, QPoint(50, 50));
  TEST_EQUAL(f.m_parent.m_presses.size(), 0, (f.m_parent.m_presses));
  TEST_EQUAL(f.m_parent.m_releases.size(), 0, (f.m_parent.m_releases));
}

UNIT_TEST(ForwardingQuickWidget_RightButtonReleaseReachesParent)
{
  EnsureApp();
  Fixture f(kEmptyQml, QPoint(0, 0));
  f.Send(QEvent::MouseButtonPress, Qt::RightButton, QPoint(50, 50));
  f.Send(QEvent::MouseButtonRelease, Qt::RightButton, QPoint(50, 50));
  TEST_EQUAL(f.m_parent.m_presses.size(), 1, (f.m_parent.m_presses));
  TEST_EQUAL(f.m_parent.m_releases.size(), 1, (f.m_parent.m_releases));
}

UNIT_TEST(ForwardingQuickWidget_DragMovesReachParent)
{
  EnsureApp();
  Fixture f(kEmptyQml, QPoint(100, 0));
  f.Send(QEvent::MouseButtonPress, Qt::LeftButton, QPoint(50, 50));
  for (int x : {60, 70, 80})
    f.Send(QEvent::MouseMove, Qt::NoButton, Qt::LeftButton, QPoint(x, 50));
  f.Send(QEvent::MouseButtonRelease, Qt::LeftButton, QPoint(80, 50));
  TEST_EQUAL(f.m_parent.m_moves, Positions({{160, 50}, {170, 50}, {180, 50}}), ());
  TEST_EQUAL(f.m_parent.m_moveButtons, std::vector<int>(3, Qt::LeftButton), ());
}

UNIT_TEST(ForwardingQuickWidget_WheelOutsideQmlItemsReachesParent)
{
  EnsureApp();
  Fixture f(kEmptyQml, QPoint(0, 0));
  f.SendWheel(QPoint(50, 50));
  TEST_EQUAL(f.m_parent.m_wheels, 1, ());
}

UNIT_TEST(ForwardingQuickWidget_TouchOnQmlInputItemStaysInQml)
{
  EnsureApp();
  Fixture f(kWrappedMouseAreaQml, QPoint(0, 0));
  f.SendTouch(QPoint(90, 90));
  TEST_EQUAL(f.m_parent.m_touches, TouchCounts({0, 0, 0}), ());
  TEST_EQUAL(f.QmlProperty("area", "presses"), 1, ());
  f.SendTouch(QPoint(150, 150));
  TEST_EQUAL(f.m_parent.m_touches, TouchCounts({1, 1, 1}), ());
  f.SendTouch(QPoint(90, 90));
  TEST_EQUAL(f.m_parent.m_touches, TouchCounts({1, 1, 1}), ());
  TEST_EQUAL(f.QmlProperty("area", "presses"), 2, ());
}

UNIT_TEST(ForwardingQuickWidget_TouchOnNonInputItemReachesParent)
{
  EnsureApp();
  Fixture f(kFullSizeRectangleQml, QPoint(0, 0));
  f.SendTouch(QPoint(50, 50));
  TEST_EQUAL(f.m_parent.m_touches, TouchCounts({1, 1, 1}), ());
}

UNIT_TEST(ForwardingQuickWidget_TouchBeginOutsideQmlItemsReachesParent)
{
  EnsureApp();
  Fixture f(kEmptyQml, QPoint(0, 0));
  f.SendTouch(QPoint(50, 50));
  TEST_EQUAL(f.m_parent.m_touches, TouchCounts({1, 1, 1}), ());
}

UNIT_TEST(ForwardingQuickWidget_TouchOnDisabledMouseAreaReachesParent)
{
  EnsureApp();
  Fixture f(kDisabledMouseAreaQml, QPoint(0, 0));
  f.SendTouch(QPoint(50, 50));
  TEST_EQUAL(f.m_parent.m_touches, TouchCounts({1, 1, 1}), ());
}

UNIT_TEST(ForwardingQuickWidget_TouchOnClippedMouseAreaReachesParent)
{
  EnsureApp();
  Fixture f(kClippedMouseAreaQml, QPoint(0, 0));
  f.SendTouch(QPoint(50, 50));
  TEST_EQUAL(f.m_parent.m_touches, TouchCounts({1, 1, 1}), ());
}

UNIT_TEST(ForwardingQuickWidget_TouchOnOpenPopupStaysInQml)
{
  EnsureApp();
  Fixture f(kOpenPopupQml, QPoint(0, 0));
  f.SendTouch(QPoint(50, 50));
  TEST_EQUAL(f.m_parent.m_touches, TouchCounts({0, 0, 0}), ());
}

UNIT_TEST(ForwardingQuickWidget_ChordedButtonsKeepPerButtonDelivery)
{
  EnsureApp();
  {
    Fixture f(kRightAcceptingQml, QPoint(0, 0));
    f.Send(QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton, QPoint(50, 50));
    f.Send(QEvent::MouseButtonPress, Qt::RightButton, Qt::LeftButton | Qt::RightButton, QPoint(50, 50));
    f.Send(QEvent::MouseButtonRelease, Qt::RightButton, Qt::LeftButton, QPoint(50, 50));
    f.Send(QEvent::MouseButtonRelease, Qt::LeftButton, Qt::NoButton, QPoint(50, 50));
    TEST_EQUAL(f.m_parent.m_pressButtons, Buttons({Qt::LeftButton}), ());
    TEST_EQUAL(f.m_parent.m_releaseButtons, Buttons({Qt::LeftButton}), ());
  }
  {
    Fixture f(kLeftAcceptingQml, QPoint(0, 0));
    f.Send(QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton, QPoint(50, 50));
    f.Send(QEvent::MouseButtonPress, Qt::RightButton, Qt::LeftButton | Qt::RightButton, QPoint(50, 50));
    f.Send(QEvent::MouseButtonRelease, Qt::LeftButton, Qt::RightButton, QPoint(50, 50));
    f.Send(QEvent::MouseButtonRelease, Qt::RightButton, Qt::NoButton, QPoint(50, 50));
    TEST_EQUAL(f.m_parent.m_pressButtons, Buttons({Qt::RightButton}), ());
    TEST_EQUAL(f.m_parent.m_releaseButtons, Buttons({Qt::RightButton}), ());
  }
}

UNIT_TEST(ForwardingQuickWidget_IgnoredStateResetsPerPress)
{
  EnsureApp();
  Fixture f(kLeftHalfAcceptingQml, QPoint(0, 0));
  f.Send(QEvent::MouseButtonPress, Qt::LeftButton, QPoint(150, 50));
  f.Send(QEvent::MouseButtonRelease, Qt::LeftButton, QPoint(150, 50));
  f.Send(QEvent::MouseButtonPress, Qt::LeftButton, QPoint(50, 50));
  f.Send(QEvent::MouseButtonRelease, Qt::LeftButton, QPoint(50, 50));
  TEST_EQUAL(f.m_parent.m_presses.size(), 1, (f.m_parent.m_presses));
  TEST_EQUAL(f.m_parent.m_releases.size(), 1, (f.m_parent.m_releases));
}

UNIT_TEST(ForwardingQuickWidget_ReleaseAfterDoubleClickReachesParent)
{
  EnsureApp();
  Fixture f(kEmptyQml, QPoint(0, 0));
  f.Send(QEvent::MouseButtonPress, Qt::LeftButton, QPoint(50, 50));
  f.Send(QEvent::MouseButtonRelease, Qt::LeftButton, QPoint(50, 50));
  f.Send(QEvent::MouseButtonDblClick, Qt::LeftButton, Qt::LeftButton, QPoint(50, 50));
  f.Send(QEvent::MouseButtonRelease, Qt::LeftButton, QPoint(50, 50));
  TEST_EQUAL(f.m_parent.m_releases.size(), 2, (f.m_parent.m_releases));
  TEST_EQUAL(f.m_parent.m_doubleClicks, 1, ());
}

UNIT_TEST(ForwardingQuickWidget_DoubleClickFollowsQmlPress)
{
  EnsureApp();
  Fixture f(kAcceptingQml, QPoint(0, 0));
  f.Send(QEvent::MouseButtonPress, Qt::LeftButton, QPoint(50, 50));
  f.Send(QEvent::MouseButtonRelease, Qt::LeftButton, QPoint(50, 50));
  f.Send(QEvent::MouseButtonDblClick, Qt::LeftButton, Qt::LeftButton, QPoint(50, 50));
  f.Send(QEvent::MouseButtonRelease, Qt::LeftButton, QPoint(50, 50));
  TEST_EQUAL(f.m_parent.m_presses.size(), 0, (f.m_parent.m_presses));
  TEST_EQUAL(f.m_parent.m_doubleClicks, 0, ());
  TEST_EQUAL(f.m_parent.m_releases.size(), 0, (f.m_parent.m_releases));
}

UNIT_TEST(ForwardingQuickWidget_MouseOnTapHandlerStaysInQml)
{
  EnsureApp();
  Fixture f(kTapHandlerQml, QPoint(0, 0));
  f.ClickOnWindow(QPoint(50, 50));
  TEST_EQUAL(f.m_parent.m_presses.size(), 0, (f.m_parent.m_presses));
  TEST_EQUAL(f.m_parent.m_releases.size(), 0, (f.m_parent.m_releases));
  TEST_EQUAL(f.QmlProperty("tap", "taps"), 1, ());
  f.DoubleClickOnWindow(QPoint(50, 50));
  TEST_EQUAL(f.m_parent.m_presses.size(), 0, (f.m_parent.m_presses));
  TEST_EQUAL(f.m_parent.m_doubleClicks, 0, ());
  TEST_EQUAL(f.m_parent.m_releases.size(), 0, (f.m_parent.m_releases));
  f.ClickOnWindow(QPoint(150, 150));
  TEST_EQUAL(f.m_parent.m_presses.size(), 1, (f.m_parent.m_presses));
  TEST_EQUAL(f.m_parent.m_releases.size(), 1, (f.m_parent.m_releases));
}

UNIT_TEST(ForwardingQuickWidget_TouchOnTapHandlerStaysInQml)
{
  EnsureApp();
  Fixture f(kTapHandlerQml, QPoint(0, 0));
  f.SendTouch(QPoint(50, 50));
  TEST_EQUAL(f.m_parent.m_touches, TouchCounts({0, 0, 0}), ());
  TEST_EQUAL(f.QmlProperty("tap", "taps"), 1, ());
}

UNIT_TEST(ForwardingQuickWidget_ModalPopupBlocksMap)
{
  EnsureApp();
  Fixture f(kModalPopupQml, QPoint(0, 0));
  f.ClickOnWindow(QPoint(150, 150));
  TEST_EQUAL(f.m_parent.m_presses.size(), 0, (f.m_parent.m_presses));
  TEST_EQUAL(f.m_parent.m_releases.size(), 0, (f.m_parent.m_releases));
  f.SendTouch(QPoint(150, 150));
  TEST_EQUAL(f.m_parent.m_touches, TouchCounts({0, 0, 0}), ());
  TEST_EQUAL(f.QmlProperty("popup", "visible"), 1, ());
}

UNIT_TEST(ForwardingQuickWidget_ModalDismissKeepsReleaseInQml)
{
  EnsureApp();
  Fixture f(kDismissableModalPopupQml, QPoint(0, 0));
  f.ClickOnWindow(QPoint(150, 150));
  TEST_EQUAL(f.QmlProperty("popup", "visible"), 0, ());
  TEST_EQUAL(f.m_parent.m_presses.size(), 0, (f.m_parent.m_presses));
  TEST_EQUAL(f.m_parent.m_releases.size(), 0, (f.m_parent.m_releases));
}

UNIT_TEST(ForwardingQuickWidget_FlickableDragStaysInQml)
{
  EnsureApp();
  Fixture f(kFlickableQml, QPoint(0, 0));
  f.DragOnWindow(QPoint(50, 50), QPoint(0, -20), 3);
  TEST_EQUAL(f.m_parent.m_presses.size(), 0, (f.m_parent.m_presses));
  TEST_EQUAL(f.m_parent.m_moves.size(), 0, (f.m_parent.m_moves));
  TEST_EQUAL(f.m_parent.m_releases.size(), 0, (f.m_parent.m_releases));
  TEST(f.QmlProperty("flick", "contentX") != 0 || f.QmlProperty("flick", "contentY") != 0, ());
}
}  // namespace forwarding_quickwidget_tests
