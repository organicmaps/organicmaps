#pragma once
#include "../words/dictionary.hpp"
#include "../base/base.hpp"
#include "../std/map.hpp"
#include "../std/scoped_ptr.hpp"
#include "../std/string.hpp"

class Reader;

namespace sl
{

class AardDictionary : public Dictionary
{
public:
  explicit AardDictionary(Reader const & reader);
  ~AardDictionary();
  Id KeyCount() const;
  void KeyById(Id id, string & key) const;
  void ArticleById(Id id, string & article) const;
private:
  Reader const & m_Reader;
  struct AardDictionaryHeader;
  scoped_ptr<AardDictionaryHeader> m_pHeader;
  map<string, Id> m_KeyToIdMap;
};

}
