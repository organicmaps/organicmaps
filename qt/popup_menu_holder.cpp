#include "popup_menu_holder.hpp"

#include "base/assert.hpp"

#include <QtWidgets/QMenu>

namespace qt
{

PopupMenuHolder::PopupMenuHolder(QObject * parent) : QObject(parent) {}

QAction * PopupMenuHolder::addActionImpl(QIcon const & icon, QString const & text, bool checkable)
{
  QAction * p = new QAction(icon, text, this);
  p->setCheckable(checkable);
  m_actions.push_back(p);
  return p;
}

QToolButton * PopupMenuHolder::create()
{
  QMenu * menu = new QMenu();
  for (auto * p : m_actions)
    menu->addAction(p);

  m_toolButton = new QToolButton();
  m_toolButton->setPopupMode(QToolButton::MenuButtonPopup);
  m_toolButton->setMenu(menu);
  return m_toolButton;
}

void PopupMenuHolder::setMainIcon(QIcon const & icon)
{
  m_toolButton->setIcon(icon);
}

void PopupMenuHolder::setCurrent(size_t idx)
{
  CHECK_LESS(idx, m_actions.size(), ());
  m_toolButton->setIcon(m_actions[idx]->icon());
}

void PopupMenuHolder::setChecked(size_t idx, bool checked)
{
  CHECK_LESS(idx, m_actions.size(), ());
  ASSERT(m_actions[idx]->isCheckable(), ());
  m_actions[idx]->setChecked(checked);
}

bool PopupMenuHolder::isChecked(size_t idx)
{
  CHECK_LESS(idx, m_actions.size(), ());
  ASSERT(m_actions[idx]->isCheckable(), ());
  return m_actions[idx]->isChecked();
}

}  // namespace qt
