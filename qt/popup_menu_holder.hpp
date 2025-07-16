#pragma once

#include <QtGui/QAction>
#include <QtWidgets/QToolButton>

#include <type_traits>
#include <vector>

namespace qt
{
/// Inherit from QObject to assign parents and don't worry about deleting.
class PopupMenuHolder : public QObject
{
  Q_OBJECT

  QToolButton * m_toolButton;

  std::vector<QAction *> m_actions;

  QAction * addActionImpl(QIcon const & icon, QString const & text, bool checkable);

public:
  explicit PopupMenuHolder(QObject * parent = nullptr);

  QAction * addAction(QIcon const & icon, QString const & text, QObject const * receiver, char const * member,
                      bool checkable)
  {
    QAction * p = addActionImpl(icon, text, checkable);
    connect(p, SIGNAL(triggered()), receiver, member);
    return p;
  }

  template <class SlotT>
  QAction * addAction(QIcon const & icon, QString const & text, SlotT slot, bool checkable)
  {
    QAction * p = addActionImpl(icon, text, checkable);
    connect(p, &QAction::triggered, std::move(slot));
    return p;
  }

  QToolButton * create();
  void setMainIcon(QIcon const & icon);

  void setCurrent(size_t idx);
  template <class T>
  typename std::enable_if<std::is_enum<T>::value, void>::type setCurrent(T idx)
  {
    setCurrent(static_cast<size_t>(idx));
  }

  void setChecked(size_t idx, bool checked);

  bool isChecked(size_t idx);
  template <class T>
  typename std::enable_if<std::is_enum<T>::value, bool>::type isChecked(T idx)
  {
    return isChecked(static_cast<size_t>(idx));
  }
};
}  // namespace qt
