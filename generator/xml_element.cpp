#include "xml_element.hpp"

#include "../coding/parse_xml.hpp"
#include "../coding/reader.hpp"

#include "../std/cstdio.hpp"
#include "../std/algorithm.hpp"
#include "../std/limits.hpp"


bool BaseOSMParser::is_our_tag(string const & name)
{
  return (find(m_tags.begin(), m_tags.end(), name) != m_tags.end());
}

bool BaseOSMParser::Push(string const & name)
{
  if (!is_our_tag(name) && (m_depth != 2))
    return false;

  ++m_depth;

  if (m_depth == 1)
  {
    m_current = 0;
  }
  else if (m_depth == 2)
  {
    m_current = &m_element;
    m_current->parent = 0;
  }
  else
  {
    m_current->childs.push_back(XMLElement());
    m_current->childs.back().parent = m_current;
    m_current = &m_current->childs.back();
  }

  if (m_depth >= 2)
    m_current->name = name;
  return true;
}

void BaseOSMParser::AddAttr(string const & name, string const & value)
{
  if (m_current)
    m_current->attrs[name] = value;
}

void BaseOSMParser::Pop(string const &)
{
  --m_depth;

  if (m_depth >= 2)
    m_current = m_current->parent;

  else if (m_depth == 1)
  {
    EmitElement(m_current);
    m_current->Clear();
  }
}


struct StdinReader
{
  uint64_t Read(char * buffer, uint64_t bufferSize)
  {
    return fread(buffer, sizeof(char), bufferSize, stdin);
  }
};


void ParseXMLFromStdIn(BaseOSMParser & parser)
{
  StdinReader reader;
  (void)ParseXMLSequence(reader, parser);
}

void ParseXMLFromFile(FileReader const & reader, BaseOSMParser & parser)
{
  ReaderSource<FileReader> src(reader);
  CHECK(ParseXML(src, parser), ());
}
