#pragma once
#include "../coding/file_reader.hpp"
#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/map.hpp"

struct XMLElement
{
  string name;
  map<string, string> attrs;
  vector<XMLElement> childs;
  XMLElement * parent;

  void Clear()
  {
    name.clear();
    attrs.clear();
    childs.clear();
    parent = 0;
  }
};

class BaseOSMParser
{
  XMLElement m_element;
  XMLElement * m_current;

  size_t m_depth;

  vector<string> m_tags;
  bool is_our_tag(string const & name);

public:
  BaseOSMParser() : m_current(0), m_depth(0) {}

  template <size_t N> void SetTags(char const * (&arr)[N]) { m_tags.assign(&arr[0], &arr[N]); }

  bool Push(string const & name);
  void AddAttr(string const & name, string const & value);
  void Pop(string const &);

protected:
  virtual void EmitElement(XMLElement * p) = 0;
};

void ParseXMLFromStdIn(BaseOSMParser & parser);

void ParseXMLFromFile(FileReader const & reader, BaseOSMParser & parser);
