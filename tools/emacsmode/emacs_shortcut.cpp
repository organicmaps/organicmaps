#include "emacs_shortcut.h"
#include <QString>
#include <QStringList>

namespace Emacs
{
  Shortcut::Shortcut(Qt::KeyboardModifiers mods, std::vector<int> const & keys, TFnList const & fnList)
    : m_mods(mods), m_keys(keys), m_fnList(fnList)
  {}

  Shortcut::Shortcut(QString const & s)
  {
    QStringList l = s.split("|");
    QString keys = l.at(l.size() - 1).toLower();

    for (int i = 0; i < l.size(); ++i)
    {
      QString key = l.at(i).toUpper();
      if (key == "<CONTROL>")
        m_mods |= Qt::ControlModifier;
      else if (key == "<META>")
        m_mods |= Qt::MetaModifier;
      else if (key == "<SHIFT>")
        m_mods |= Qt::ShiftModifier;
      else if (key == "<ALT>")
        m_mods |= Qt::AltModifier;
      else if (key == "<TAB>")
        m_keys.push_back(Qt::Key_Tab);
      else if (key == "<SPACE>")
        m_keys.push_back(Qt::Key_Space);
      else if (key == "<UNDERSCORE>")
        m_keys.push_back(Qt::Key_Underscore);
      else if (key == "<ESC>")
        m_keys.push_back(Qt::Key_Escape);
      else
          m_keys.push_back(key.at(0).toAscii() - 'A' + Qt::Key_A);
    }
  }

  Shortcut::Shortcut()
  {}


  bool Shortcut::isEmpty() const
  {
    return m_keys.size() != 0;
  }

  bool Shortcut::isAccepted(QKeyEvent * kev) const
  {
    int key = kev->key();
    Qt::KeyboardModifiers mods = kev->modifiers();
    return ((mods == m_mods) && (key == m_keys.front()));
  }

  bool Shortcut::hasFollower(QKeyEvent * kev) const
  {
    return (isAccepted(kev) && (m_keys.size() > 1));
  }

  Shortcut const Shortcut::getFollower(QKeyEvent * kev) const
  {
    if (hasFollower(kev))
    {
      std::vector<int> keys;
      std::copy(++m_keys.begin(), m_keys.end(), std::back_inserter(keys));
      return Shortcut(m_mods, keys, m_fnList);
    }
    return Shortcut();
  }

  Shortcut & Shortcut::addFn(boost::function<void()> fn)
  {
    m_fnList.push_back(fn);
    return *this;
  }

  void Shortcut::exec() const
  {
    for (TFnList::const_iterator it = m_fnList.begin(); it != m_fnList.end(); ++it)
      (*it)();
  }

}
