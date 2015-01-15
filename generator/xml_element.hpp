#pragma once
#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/map.hpp"

#include "../std/iostream.hpp"

struct XMLElement
{
  enum ETag {
    ET_UNKNOWN = 0,
    ET_OSM = 'so',
    ET_NODE = 'on',
    ET_WAY = 'aw',
    ET_RELATION = 'er',
    ET_TAG = 'at',
    ET_ND = 'dn',
    ET_MEMBER = 'em'
  };

  ETag tagKey = ET_UNKNOWN;
  uint64_t id = 0;
  double lng = 0;
  double lat = 0;
  uint64_t ref = 0;
  string k;
  string v;
  string type;
  string role;
  
  XMLElement * parent = nullptr;
  vector<XMLElement> childs;

  void AddKV(string const & k, string const & v);
};

class BaseOSMParser
{
  XMLElement m_element;

  size_t m_depth;

  typedef struct { char const * tagName; XMLElement::ETag tagKey; bool allowed;} TagT;

protected:
  XMLElement * m_current;

public:
  BaseOSMParser() : m_current(0), m_depth(0) {}

  void AddAttr(string const & key, string const & value);
  bool Push(string const & tagName);
  void Pop(string const &);
  void CharData(string const &) {}

protected:
  bool MatchTag(string const & tagName, XMLElement::ETag & tagKey);
  virtual void EmitElement(XMLElement * p) = 0;
};

void ParseXMLFromStdIn(BaseOSMParser & parser);
void ParseXMLFromFile(BaseOSMParser & parser, string const & osmFileName);
