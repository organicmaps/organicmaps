#pragma once
#include "dictionary.hpp"
#include "slof.hpp"
#include "../std/function.hpp"
#include "../std/scoped_ptr.hpp"

class Reader;

namespace sl
{

class SlofDictionary : public Dictionary
{
public:
  // Takes ownership of pReader and deletes it, even if exception is thrown.
  explicit SlofDictionary(Reader const * pReader);
  // Takes ownership of pReader and deletes it, even if exception is thrown.
  SlofDictionary(Reader const * pReader,
                 function<void (char const *, size_t, char *, size_t)> decompressor);
  ~SlofDictionary();
  Id KeyCount() const;
  void KeyById(Id id, string & key) const;
  void ArticleById(Id id, string & article) const;
private:
  void Init();
  void ReadKeyData(Id id, string & data) const;
  scoped_ptr<Reader const> m_pReader;
  function<void (char const *, size_t, char *, size_t)> m_Decompressor;
  SlofHeader m_Header;
};

}
