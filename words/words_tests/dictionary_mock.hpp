#pragma once
#include "../dictionary.hpp"
#include "../../base/assert.hpp"
#include "../../std/string.hpp"
#include "../../std/vector.hpp"

class DictionaryMock : public sl::Dictionary
{
public:
  void Add(string const & key, string const & article)
  {
    m_Keys.push_back(key);
    m_Articles.push_back(article);
  }

  Id KeyCount() const
  {
    ASSERT_EQUAL(m_Keys.size(), m_Articles.size(), ());
    return m_Keys.size();
  }

  void KeyById(Id id, string & key) const { key = m_Keys[id]; }
  void ArticleById(Id id, string & article) const { article = m_Articles[id]; }

private:
  vector<string> m_Keys, m_Articles;
};
