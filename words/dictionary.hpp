#pragma once
#include "../base/base.hpp"
#include "../std/string.hpp"
#include "../base/base.hpp"
#include "../base/exception.hpp"
#include "../std/utility.hpp"

namespace sl
{

class Dictionary
{
public:
  DECLARE_EXCEPTION(Exception, RootException);
  DECLARE_EXCEPTION(OpenDictionaryException, Exception);
  DECLARE_EXCEPTION(OpenDictionaryNewerVersionException, Exception);
  DECLARE_EXCEPTION(BrokenDictionaryException, Exception);

  // Key id, which is 0-based index of key in sorted order.
  typedef uint32_t Id;

  virtual ~Dictionary() {}

  // Number of keys in the dictionary.
  virtual Id KeyCount() const = 0;

  // Get key by id.
  virtual void KeyById(Id id, string & key) const = 0;

  // Get article by id.
  virtual void ArticleById(Id id, string & article) const = 0;
};

}
