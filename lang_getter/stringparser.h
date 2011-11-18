#pragma once

#include <QDomDocument>


class Logging;

class ContentParser
{
  Logging & m_log;

  QDomDocument m_doc;
  QDomElement m_node;

public:
  ContentParser(Logging & log) : m_log(log) {}

  bool InitSubDOM(QString const & xml, QString const & entry, QString const & tag);
  QDomElement Root() const { return m_node; }
};

template <class ToDo>
void TokenizeString(QString const & s, QString const & delim, ToDo toDo)
{
  int beg = 0;
  int i = 0;
  for (; i < s.length(); ++i)
  {
    if (delim.indexOf(s[i]) != -1)
    {
      if (i > beg)
        toDo(s.mid(beg, i-beg));
      beg = i+1;
    }
  }

  if (i > beg)
    toDo(s.mid(beg, i-beg));
}
