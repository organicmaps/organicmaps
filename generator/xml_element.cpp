#include "xml_element.hpp"

#include "../coding/parse_xml.hpp"

#include "../std/cstdio.hpp"
#include "../std/algorithm.hpp"


void XMLElement::Clear()
{
  name.clear();
  attrs.clear();
  childs.clear();
  parent = 0;
}

void XMLElement::AddKV(string const & k, string const & v)
{
  childs.push_back(XMLElement());
  XMLElement & e = childs.back();

  e.name = "tag";
  e.attrs["k"] = k;
  e.attrs["v"] = v;
  e.parent = this;
}

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

namespace
{
  struct StdinReader
  {
    uint64_t Read(char * buffer, uint64_t bufferSize)
    {
      return fread(buffer, sizeof(char), bufferSize, stdin);
    }
  };

  struct FileReader
  {
    FILE * m_file;

    FileReader(string const & filename)
    {
      m_file = fopen(filename.c_str(), "rb");
    }

    ~FileReader()
    {
      fclose(m_file);
    }

    uint64_t Read(char * buffer, uint64_t bufferSize)
    {
      return fread(buffer, sizeof(char), bufferSize, m_file);
    }
  };
}

void ParseXMLFromStdIn(BaseOSMParser & parser)
{
  StdinReader reader;
  (void)ParseXMLSequence(reader, parser);
}

void ParseXMLFromFile(BaseOSMParser & parser, string const & osmFileName)
{
  FileReader reader(osmFileName);
  (void)ParseXMLSequence(reader, parser);
}
