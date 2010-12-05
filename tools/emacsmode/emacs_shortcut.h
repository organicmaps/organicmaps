#ifndef EMACS_SHORTCUT_H
#define EMACS_SHORTCUT_H

#include <boost/function.hpp>
#include <list>
#include <vector>
#include <Qt>
#include <QKeyEvent>

namespace Emacs
{
  class Shortcut
  {
  private:

    Qt::KeyboardModifiers m_mods;
    std::vector<int> m_keys;

    typedef std::list<boost::function<void ()> > TFnList;
    TFnList m_fnList;

  public:

    Shortcut();
    Shortcut(QString const & s);
    Shortcut(Qt::KeyboardModifiers, std::vector<int> const & , TFnList const & );

    Shortcut & addFn(boost::function<void ()> fn);

    void exec() const;

    bool isEmpty() const;
    bool isAccepted(QKeyEvent * kev) const;
    bool hasFollower(QKeyEvent * kev) const;
    Shortcut const getFollower(QKeyEvent * kev) const;
  };
}

#endif // EMACS_SHORTCUT_H
