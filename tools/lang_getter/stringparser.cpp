#include "stringparser.h"
#include "logging.h"


bool ContentParser::InitSubDOM(QString const & xml, QString const & entry, QString const & tag)
{
  int const i = xml.indexOf(entry);
  if (i == -1)
  {
    m_log.Print(Logging::INFO, QString("Can't find entry: ") + entry);
    return false;
  }

  int const beg = xml.indexOf(QString("<") + tag, i);
  if (beg == -1 || beg < i)
  {
    m_log.Print(Logging::INFO, QString("Can't find tag: ") + tag);
    return false;
  }

  QString last = QString("/") + tag + QString(">");
  int const end = xml.indexOf(last, beg);
  Q_ASSERT ( end != -1 && beg < end );

  if (!m_doc.setContent(xml.mid(beg, end - beg + last.length())))
  {
    m_log.Print(Logging::ERROR, QString("QDomDocument::setContent error"));
    return false;
  }

  m_node = m_doc.documentElement();
  Q_ASSERT ( !m_node.isNull() );
  Q_ASSERT ( m_node.tagName() == tag );

  return true;
}
